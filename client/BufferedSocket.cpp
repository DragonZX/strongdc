/* 
 * Copyright (C) 2001-2006 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdinc.h"
#include "DCPlusPlus.h"

#include "BufferedSocket.h"

#include "ResourceManager.h"
#include "TimerManager.h"
#include "SettingsManager.h"

#include "Streams.h"
#include "SSLSocket.h"
#include "CryptoManager.h"

#include "UploadManager.h"
#include "DownloadManager.h"

// Polling is used for tasks...should be fixed...
#define POLL_TIMEOUT 250

BufferedSocket::BufferedSocket(char aSeparator) throw() : 
separator(aSeparator), mode(MODE_LINE), filterIn(NULL),
dataBytes(0), rollback(0), failed(false), sock(0), disconnecting(false)
{
	Thread::safeInc(sockets);
}

volatile long BufferedSocket::sockets = 0;

BufferedSocket::~BufferedSocket() throw() {
	delete sock;
	delete filterIn;
	Thread::safeDec(sockets);
}

void BufferedSocket::setMode (Modes aMode, size_t aRollback) {
	if (mode == aMode) {
		dcdebug ("WARNING: Re-entering mode %d\n", mode);
		return;
	}

	if (mode == MODE_ZPIPE) {
		// should not happen!
		if (filterIn) {
			delete filterIn;
			filterIn = NULL;
		}
	}

	mode = aMode;
	switch (aMode) {
		case MODE_LINE:
			rollback = aRollback;
			break;
		case MODE_ZPIPE:
			filterIn = new UnZFilter;
			break;
		case MODE_DATA:
			break;
	}
}

void BufferedSocket::accept(const Socket& srv, bool secure, bool allowUntrusted) throw(SocketException, ThreadException) {
	dcassert(!sock);
	try {
		dcdebug("BufferedSocket::accept() %p\n", (void*)this);
		sock = secure ? CryptoManager::getInstance()->getServerSocket(allowUntrusted) : new Socket;

		sock->accept(srv);
		if(SETTING(SOCKET_IN_BUFFER) > 1023)
			sock->setSocketOpt(SO_RCVBUF, SETTING(SOCKET_IN_BUFFER));
    	if(SETTING(SOCKET_OUT_BUFFER) > 1023)
			sock->setSocketOpt(SO_SNDBUF, SETTING(SOCKET_OUT_BUFFER));
		sock->setBlocking(false);

		inbuf.resize(sock->getSocketOptInt(SO_RCVBUF));

		// This lock prevents the shutdown task from being added and executed before we're done initializing the socket
		Lock l(cs);
		start();
		addTask(ACCEPTED, 0);
	} catch(...) {
		delete sock;
		sock = 0;
		throw;
	}
}

void BufferedSocket::connect(const string& aAddress, short aPort, bool secure, bool allowUntrusted, bool proxy) throw(SocketException, ThreadException) {
	dcassert(!sock);

	try {
		dcdebug("BufferedSocket::connect() %p\n", (void*)this);
		sock = secure ? CryptoManager::getInstance()->getClientSocket(allowUntrusted) : new Socket;

		sock->create();
		if(SETTING(SOCKET_IN_BUFFER) >= 1024)
			sock->setSocketOpt(SO_RCVBUF, SETTING(SOCKET_IN_BUFFER));
		if(SETTING(SOCKET_OUT_BUFFER) >= 1024)
			sock->setSocketOpt(SO_SNDBUF, SETTING(SOCKET_OUT_BUFFER));
		sock->setBlocking(false);

		inbuf.resize(sock->getSocketOptInt(SO_RCVBUF));

		Lock l(cs);
		start();
		addTask(CONNECT, new ConnectInfo(aAddress, aPort, proxy && (SETTING(OUTGOING_CONNECTIONS) == SettingsManager::OUTGOING_SOCKS5)));
	} catch(...) {
		delete sock;
		sock = 0;
		throw;
	}
}

#define CONNECT_TIMEOUT 30000
void BufferedSocket::threadConnect(const string& aAddr, short aPort, bool proxy) throw(SocketException) {
	dcdebug("threadConnect %s:%d\n", aAddr.c_str(), (int)aPort);
	dcassert(sock);
	if(!sock)
		return;
	fire(BufferedSocketListener::Connecting());

	u_int32_t startTime = GET_TICK();
	if(proxy) {
		sock->socksConnect(aAddr, aPort, CONNECT_TIMEOUT);
	} else {
		sock->connect(aAddr, aPort);
	}

	while(sock->wait(POLL_TIMEOUT, Socket::WAIT_CONNECT) != Socket::WAIT_CONNECT) {
		if(disconnecting)
			return;

		if((startTime + 30000) < GET_TICK()) {
			throw SocketException(STRING(CONNECTION_TIMEOUT));
		}
	}

	fire(BufferedSocketListener::Connected());
}	

void BufferedSocket::threadRead() throw(SocketException) {
	dcassert(sock);
	if(!sock)
		return;

	DownloadManager *dm = DownloadManager::getInstance();
	size_t readsize = inbuf.size();
	bool throttling = false;
	if(mode == MODE_DATA)
	{
		u_int32_t getMaximum;
		throttling = dm->throttle();
		if (throttling)
		{
			getMaximum = dm->throttleGetSlice();
			readsize = (u_int32_t)min((int64_t)inbuf.size(), (int64_t)getMaximum);
			if (readsize <= 0  || readsize > inbuf.size()) { // FIX
				Thread::sleep(dm->throttleCycleTime());
				return;
			}
		}
	}
	int left = sock->read(&inbuf[0], (int)readsize);
	if(left == -1) {
	// EWOULDBLOCK, no data received...
		return;
	} else if(left == 0) {
		// This socket has been closed...
		throw SocketException(STRING(CONNECTION_CLOSED));
	}
    size_t used;
	string::size_type pos = 0;
    // always uncompressed data
	string l;
	int bufpos = 0, total = left;

	while (left > 0) {
		switch (mode) {
			case MODE_ZPIPE:
				if (filterIn != NULL){
				    const int BufSize = 1024;
					// Special to autodetect nmdc connections...
					string::size_type pos = 0;
					AutoArray<u_int8_t> buffer (BufSize);
					size_t in;
					l = line;
					// decompress all input data and store in l.
					while (left) {
						in = BufSize;
						used = left;
						bool ret = (*filterIn) ((void *)(&inbuf[0] + total - left), used, &buffer[0], in);
						left -= used;
						l.append ((const char *)&buffer[0], in);
						// if the stream ends before the data runs out, keep remainder of data in inbuf
						if (!ret) {
							bufpos = total-left;
							setMode (MODE_LINE, rollback);
							break;
						}
					}
					// process all lines
					while ((pos = l.find(separator)) != string::npos) {
                       	if(pos > 0) // check empty (only pipe) command and don't waste cpu with it ;o)
							fire(BufferedSocketListener::Line(), l.substr(0, pos));
						l.erase (0, pos + 1 /* seperator char */);
					}
					// store remainder
					line = l;

					break;
				}
			case MODE_LINE:
				// Special to autodetect nmdc connections...
				if(separator == 0) {
					if(inbuf[0] == '$') {
						separator = '|';
					} else {
						separator = '\n';
					}
				}
				l = line + string ((char*)&inbuf[bufpos], left);
				while ((pos = l.find(separator)) != string::npos) {
	                if(pos > 0) // check empty (only pipe) command and don't waste cpu with it ;o)
						fire(BufferedSocketListener::Line(), l.substr(0, pos));
					l.erase (0, pos + 1 /* separator char */);
					if (l.length() < (size_t)left) left = l.length();
					if (mode != MODE_LINE) {
						// we changed mode; remainder of l is invalid.
						l.clear();
						bufpos = total - left;
						break;
					}
				}
				if (pos == string::npos) 
					left = 0;
				line = l;
				break;
			case MODE_DATA:
				while(left > 0) {
					if(dataBytes == -1) {
						fire(BufferedSocketListener::Data(), &inbuf[bufpos], left);
						bufpos += (left - rollback);
						left = rollback;
						rollback = 0;
					} else {
						int high = (int)min(dataBytes, (int64_t)left);
						fire(BufferedSocketListener::Data(), &inbuf[bufpos], high);
						bufpos += high;
						left -= high;

						dataBytes -= high;
						if(dataBytes == 0) {
							mode = MODE_LINE;
							fire(BufferedSocketListener::ModeChange());
						}
					}
					if (throttling) {
						if (left > 0 && left < (int)readsize) {
							dm->throttleReturnBytes(left - readsize);
						}
						Thread::sleep(dm->throttleCycleTime());
					}
				}
				break;
		}
	}
	
	if(mode == MODE_LINE && line.size() > 16777216) {
		throw SocketException(STRING(COMMAND_TOO_LONG));
	}	
}

void BufferedSocket::threadSendFile(InputStream* file) throw(Exception) {
	dcassert(sock);
	if(!sock)
		return;
	dcassert(file != NULL);
	size_t sockSize = (size_t)sock->getSocketOptInt(SO_SNDBUF);
	size_t bufSize = max(sockSize, (size_t)64*1024);

	vector<u_int8_t> readBuf(bufSize);
	vector<u_int8_t> writeBuf(bufSize);

	size_t readPos = 0;

	bool readDone = false;
	dcdebug("Starting threadSend\n");
	UploadManager *um = UploadManager::getInstance();
	size_t sendMaximum;
	u_int32_t start = 0, current= 0;
	bool throttling;
	while(true) {
		if(disconnecting)
			return;
		throttling = BOOLSETTING(THROTTLE_ENABLE);
		if(!readDone && readBuf.size() > readPos) {
			// Fill read buffer
			size_t bytesRead = readBuf.size() - readPos;
			if(throttling) {
				start = TimerManager::getTick();
				sendMaximum = um->throttleGetSlice();
				if (sendMaximum < 0) {
					throttling = false;
					sendMaximum = bytesRead;
				}
				bytesRead = min(bytesRead, sendMaximum);
			}
			size_t actual = file->read(&readBuf[readPos], bytesRead);

			if(bytesRead > 0) {
				fire(BufferedSocketListener::BytesSent(), bytesRead, 0);
			}

			if(actual == 0) {
				readDone = true;
			} else {
				readPos += actual;
			}
		}

		if(readDone && readPos == 0) {
			fire(BufferedSocketListener::TransmitDone());
			return;
		}

		readBuf.swap(writeBuf);
		readBuf.resize(bufSize);
		writeBuf.resize(readPos);
		readPos = 0;

		size_t writePos = 0;

		while(writePos < writeBuf.size()) {
			if(disconnecting)
				return;
			size_t writeSize = min(sockSize / 2, writeBuf.size() - writePos);
			int written = sock->write(&writeBuf[writePos], writeSize);
			if(written > 0) {
				writePos += written;

				fire(BufferedSocketListener::BytesSent(), 0, written);
			} else if(written == -1) {
				if(!readDone && readPos < readBuf.size()) {
					// Read a little since we're blocking anyway...
					size_t bytesRead = min(readBuf.size() - readPos, readBuf.size() / 2);
					if(throttling) {
						start = TimerManager::getTick();
						sendMaximum = um->throttleGetSlice();
						if (sendMaximum < 0) {
							throttling = false;
							sendMaximum = bytesRead;
						}
						bytesRead = (u_int32_t)min((int64_t)bytesRead, (int64_t)sendMaximum);
					}
					size_t actual = file->read(&readBuf[readPos], bytesRead);

					if(bytesRead > 0) {
						fire(BufferedSocketListener::BytesSent(), bytesRead, 0);
					}

					if(actual == 0) {
						readDone = true;
					} else {
						readPos += actual;
					}
				} else {
					while(!disconnecting) {
						int w = sock->wait(POLL_TIMEOUT, Socket::WAIT_WRITE | Socket::WAIT_READ);
						if(w & Socket::WAIT_READ) {
							threadRead();
						}
						if(w & Socket::WAIT_WRITE) {
							break;
						}
					}
				}	
			}
			if(throttling) {
				u_int32_t cycle_time = um->throttleCycleTime();
				current = TimerManager::getTick();
				u_int32_t sleep_time = cycle_time - (current - start);
				if (sleep_time > 0 && sleep_time <= cycle_time) {
					Thread::sleep(sleep_time);
				}
			}
		}
	}
}

void BufferedSocket::write(const char* aBuf, size_t aLen) throw() {
	dcassert(sock);
	if(!sock)
		return;
	Lock l(cs);
	if(writeBuf.empty())
		addTask(SEND_DATA, 0);

	writeBuf.insert(writeBuf.end(), aBuf, aBuf+aLen);
}

void BufferedSocket::threadSendData() {
	dcassert(sock);
	if(!sock)
		return;
	{
		Lock l(cs);
		if(writeBuf.empty())
			return;

		writeBuf.swap(sendBuf);
	}

	size_t left = sendBuf.size();
	size_t done = 0;
	while(left > 0) {
		if(disconnecting) {
			return;
		}

		int w = sock->wait(POLL_TIMEOUT, Socket::WAIT_READ | Socket::WAIT_WRITE);

		if(w & Socket::WAIT_READ) {
			threadRead();
		}

		if(w & Socket::WAIT_WRITE) {
			int n = sock->write(&sendBuf[done], left);
			if(n > 0) {
				left -= n;
				done += n;
			}
		}
	}
	sendBuf.clear();
}

bool BufferedSocket::checkEvents() {
	while(isConnected() ? taskSem.wait(0) : taskSem.wait()) {
		pair<Tasks, TaskData*> p;
		{
			Lock l(cs);
			dcassert(tasks.size() > 0);
			p = tasks.front();
			tasks.erase(tasks.begin());
		}
		try {
			if(failed && p.first != SHUTDOWN) {
				dcdebug("BufferedSocket: New command when already failed: %d\n", p.first);
				fail(STRING(DISCONNECTED));
				delete p.second;
				continue;
			}

			switch(p.first) {
				case SEND_DATA:
					threadSendData(); break;
				case SEND_FILE:
					threadSendFile(((SendFileInfo*)p.second)->stream); break;
				case CONNECT: 
					{
						ConnectInfo* ci = (ConnectInfo*)p.second;
						threadConnect(ci->addr, ci->port, ci->proxy); 
						break;
					}
				case DISCONNECT:  
					if(isConnected())
						fail(STRING(DISCONNECTED)); 
					break;
				case SHUTDOWN:
					return false;
				case ACCEPTED:
					break;
			}

			delete p.second;
		} catch(const Exception& e) {
			delete p.second;
			fail(e.getError());
		}
	}
	return true;
}

void BufferedSocket::checkSocket() {
	dcassert(sock);
	if(!sock)
		return;

	int waitFor = sock->wait(POLL_TIMEOUT, Socket::WAIT_READ);

	if(waitFor & Socket::WAIT_READ) {
		threadRead();
	}
}

/**
 * Main task dispatcher for the buffered socket abstraction.
 * @todo Fix the polling...
 */
int BufferedSocket::run() {
	dcdebug("BufferedSocket::run() start %p\n", (void*)this);
	while(true) {
		try {
			if(!checkEvents())
				break;
			checkSocket();
		} catch(const Exception& e) {
			fail(e.getError());
		}
	}
	dcdebug("BufferedSocket::run() end %p\n", (void*)this);
	delete this;
	return 0;
}

void BufferedSocket::fail(const string& aError) {
	if(sock) {
		sock->disconnect();
	}
	if(!failed) {
		failed = true;
		fire(BufferedSocketListener::Failed(), aError);
	}
}

void BufferedSocket::shutdown() { 
	if(sock) {
		Lock l(cs); 
		disconnecting = true; 
		addTask(SHUTDOWN, 0); 
	} else {
		// Socket thread not running yet, disconnect...
		delete this;
	}
}

/**
 * @file
 * $Id$
 */
