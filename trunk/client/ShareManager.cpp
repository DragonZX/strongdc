/* 
 * Copyright (C) 2001-2003 Jacek Sieka, j_s@telia.com
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

#include "ShareManager.h"

#include "CryptoManager.h"
#include "ClientManager.h"
#include "LogManager.h"
#include "HashManager.h"

#include "SimpleXML.h"
#include "StringTokenizer.h"
#include "File.h"
#include "FilteredFile.h"
#include "BZUtils.h"

#ifndef _WIN32
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <limits>

ShareManager::ShareManager() : hits(0), listLen(0), bzXmlListLen(0),
	xmlDirty(false), nmdcDirty(false), refreshDirs(false), update(false), listN(0), lFile(NULL), 
	xFile(NULL), lastXmlUpdate(0), lastNmdcUpdate(0), lastFullUpdate(GET_TICK()), bloom(1<<20) 
{ 
	SettingsManager::getInstance()->addListener(this);
	TimerManager::getInstance()->addListener(this);
	DownloadManager::getInstance()->addListener(this);
	HashManager::getInstance()->addListener(this);
	/* Common search words used to make search more efficient, should be more dynamic */
	words.push_back("avi");
	words.push_back("mp3");
	words.push_back("bin");
	words.push_back("zip");
	words.push_back("jpg");
	words.push_back("mpeg");
	words.push_back("mpg");
	words.push_back("rar");
	words.push_back("ace");
	words.push_back("bin");
	words.push_back("iso");
	words.push_back("dev");
	words.push_back("flt");
	words.push_back("ccd");
	words.push_back("txt");
	words.push_back("sub");
	words.push_back("nfo");
	words.push_back("wav");
	words.push_back("exe");
	words.push_back("ccd");

};

ShareManager::~ShareManager() {
	SettingsManager::getInstance()->removeListener(this);
	TimerManager::getInstance()->removeListener(this);
	DownloadManager::getInstance()->removeListener(this);

	join();

	delete lFile;
	lFile = NULL;
	delete xFile;
	xFile = NULL;

	for(int i = 0; i <= listN; ++i) {
		File::deleteFile(Util::getAppPath() + "MyList" + Util::toString(i) + ".DcLst");
		File::deleteFile(Util::getAppPath() + "files" + Util::toString(i) + ".xml.bz2");
	}

	File::deleteFile(Util::getAppPath() + "files.xml.bz2");

	for(Directory::MapIter j = directories.begin(); j != directories.end(); ++j) {
		delete j->second;
	}
}

ShareManager::Directory::~Directory() {
	for(MapIter i = directories.begin(); i != directories.end(); ++i)
		delete i->second;
	for(File::Iter i = files.begin(); i != files.end(); ++i) {
		dcassert(i->getTTH() != NULL);
		ShareManager::getInstance()->removeTTH(i->getTTH(), i);
	}
}


string ShareManager::translateFileName(const string& aFile, bool adc, bool utf8) throw(ShareException) {
	RLock l(cs);
	if(aFile == "MyList.DcLst") {
		//generateNmdcList();
		return getListFile();
	} else if(aFile == "files.xml.bz2") {
		//generateXmlList();
		return getBZXmlFile();
	} else {
		string file;

		if(adc) {
			// Check for tth root identifier
			if(aFile.compare(0, 4, "TTH/") == 0) {
				TTHValue v(aFile.substr(4));
				HashFileIter i = tthIndex.find(&v);
				if(i != tthIndex.end()) {			
					file = i->second->getADCPath();
				} else {
					throw ShareException("File Not Available");
				}
			} else if(aFile.compare(0, 1, "/") == 0) {
				if(utf8) {
					file = Util::toAcp(aFile, file);
				} else {
					file = aFile;
				}
			} else {
				throw ShareException("File Not Available");
			}
			// Remove initial '/'
			file.erase(0, 1);

			// Change to NMDC path separators
			for(string::size_type i = 0; i < file.length(); ++i) {
				if(file[i] == '/') {
					file[i] = '\\';
				}
			}
			// Ok, we now should have an adc equivalent name
		} else if(utf8) {
			file = Util::toAcp(aFile, file);
		} else {
			file = aFile;
		}

		string::size_type i = file.find('\\');
		if(i == string::npos)
			throw ShareException("File Not Available");
		
		string aDir = file.substr(0, i);

		RLock l(cs);
		StringPairIter j = lookupVirtual(aDir);
		if(j == virtualMap.end()) {
			throw ShareException("File Not Available");
		}
		
		file = file.substr(i + 1);
		
		if(!checkFile(j->second, file)) {
			throw ShareException("File Not Available");
		}
		
		return j->second + file;
	}
}

StringPairIter ShareManager::findVirtual(const string& name) {
	for(StringPairIter i = virtualMap.begin(); i != virtualMap.	end(); ++i) {
		if(Util::stricmp(name, i->second) == 0)
			return i;
	}
	return virtualMap.end();
}

StringPairIter ShareManager::lookupVirtual(const string& name) {
	for(StringPairIter i = virtualMap.begin(); i != virtualMap.	end(); ++i) {
		if(Util::stricmp(name, i->first) == 0)
			return i;
	}
	return virtualMap.end();
}

bool ShareManager::checkFile(const string& dir, const string& aFile) {

	Directory::MapIter mi = directories.find(dir);
	if(mi == directories.end())
		return false;
	Directory* d = mi->second;

	string::size_type i;
	string::size_type j = 0;
	while( (i = aFile.find(PATH_SEPARATOR, j)) != string::npos) {
		mi = d->directories.find(aFile.substr(j, i-j));
		j = i + 1;
		if(mi == d->directories.end())
			return false;
		d = mi->second;
	}
	
	if(find_if(d->files.begin(), d->files.end(), Directory::File::StringComp(aFile.substr(j))) == d->files.end())
		return false;

	return true;
}

void ShareManager::load(SimpleXML* aXml) {
	WLock l(cs);

	if(aXml->findChild("Share")) {
		aXml->stepIn();
		while(aXml->findChild("Directory")) {
			const string& virt = aXml->getChildAttrib("Virtual");
			string d(aXml->getChildData()), newVirt;

				if(d[d.length() - 1] != PATH_SEPARATOR)
					d += PATH_SEPARATOR;
			if(!virt.empty()) {
				newVirt = virt;
				if(newVirt[newVirt.length() - 1] == PATH_SEPARATOR) {
					newVirt.erase(newVirt.length() -1, 1);
				}
			} else {
				newVirt = Util::getLastDir(d);
			}
			Directory* dp = new Directory(newVirt);
			directories[d] = dp;
			virtualMap.push_back(make_pair(newVirt, d));
		}
		aXml->stepOut();
	}
	if(aXml->findChild("NoShare")) {
		aXml->stepIn();
		while(aXml->findChild("Directory"))
			notShared.push_back(aXml->getChildData());
	
		aXml->stepOut();
	}
}

void ShareManager::save(SimpleXML* aXml) {
	RLock l(cs);
	
	aXml->addTag("Share");
	aXml->stepIn();
	for(StringPairIter i = virtualMap.begin(); i != virtualMap.end(); ++i) {
		aXml->addTag("Directory", i->second);
		aXml->addChildAttrib("Virtual", i->first);
	}
	aXml->stepOut();
	aXml->addTag("NoShare");
	aXml->stepIn();
	for(StringIter j = notShared.begin(); j != notShared.end(); ++j) {
		aXml->addTag("Directory", *j);
	}
	aXml->stepOut();
}

void ShareManager::addDirectory(const string& aDirectory, const string& aName) throw(ShareException) {
	if(!Util::fileExists(aDirectory))
		return;

	if(aDirectory.empty() || aName.empty()) {
		throw ShareException(STRING(NO_DIRECTORY_SPECIFIED));
	}

	if(Util::stricmp(SETTING(TEMP_DOWNLOAD_DIRECTORY), aDirectory) == 0) {
		throw ShareException(STRING(DONT_SHARE_TEMP_DIRECTORY));
	}

	string d(aDirectory);

	if(d[d.length() - 1] != PATH_SEPARATOR)
		d += PATH_SEPARATOR;

	Directory* dp = NULL;
	{
		RLock l(cs);
		
		Directory::Map a = directories;

		for(Directory::MapIter i = a.begin(); i != a.end(); ++i) {
			if(Util::strnicmp(d, i->first, i->first.length()) == 0) {
				// Trying to share an already shared directory
				removeDirectory1(i->first);				
			} else if(Util::strnicmp(d, i->first, d.length()) == 0) {
				// Trying to share a parent directory
				removeDirectory1(i->first);
			}
		}

		if(lookupVirtual(aName) != virtualMap.end()) {
			throw ShareException(STRING(VIRTUAL_NAME_EXISTS));
		}
		
		dp = buildTree(d, NULL);
		dp->setName(aName);
		}
	{
		WLock l(cs);
		addTree(d, dp);
		
		directories[d] = dp;
		virtualMap.push_back(make_pair(aName, d));
		setDirty();
	}
}

void ShareManager::removeDirectory(const string& aDirectory) {
	WLock l(cs);

	string d(aDirectory);

	if(d[d.length() - 1] != PATH_SEPARATOR)
		d += PATH_SEPARATOR;

	Directory::MapIter i = directories.find(d);
	if(i != directories.end()) {
		delete i->second;
		directories.erase(i);
	}

	for(StringPairIter j = virtualMap.begin(); j != virtualMap.end(); ++j) {
		if(Util::stricmp(j->second.c_str(), d.c_str()) == 0) {
			virtualMap.erase(j);
			break;
		}
	}
	setDirty();
}

void ShareManager::removeDirectory1(const string& aDirectory) {

	string d(aDirectory);

	if(d[d.length() - 1] != PATH_SEPARATOR)
		d += PATH_SEPARATOR;

	Directory::MapIter i = directories.find(d);
	if(i != directories.end()) {
		delete i->second;
		directories.erase(i);
	}

	for(StringPairIter j = virtualMap.begin(); j != virtualMap.end(); ++j) {
		if(Util::stricmp(j->second.c_str(), d.c_str()) == 0) {
			virtualMap.erase(j);
			break;
		}
	}
	setDirty();
}

int64_t ShareManager::getShareSize(const string& aDir) throw() {
	RLock l(cs);
	dcassert(aDir.size()>0);
	Directory::MapIter i = directories.find(aDir);

	if(i != directories.end()) {
		return i->second->getSize();
	}

	return -1;
}

int64_t ShareManager::getShareSize() throw() {
	RLock l(cs);
	int64_t tmp = 0;
	for(Directory::MapIter i = directories.begin(); i != directories.end(); ++i) {
		tmp += i->second->getSize();
	}
	return tmp;
}


string ShareManager::Directory::getADCPath() const throw() {
	if(parent == NULL)
		return '/' + name + '/';
	return parent->getADCPath() + name + '/';
}
string ShareManager::Directory::getFullName() const throw() {
	if(parent == NULL)
		return getName() + '\\';
	return parent->getFullName() + getName() + '\\';
}

void ShareManager::Directory::addType(u_int32_t type) throw() {
	if(!hasType(type)) {
		fileTypes |= (1 << type);
		if(getParent() != NULL)
			getParent()->addType(type);
	}
}
void ShareManager::Directory::addSearchType(u_int32_t mask) throw() {
	if(!hasSearchType(mask)) {
		searchTypes |= mask;
		if(getParent() != NULL)
			getParent()->addSearchType(mask);
	}
}

class FileFindIter {
#ifdef _WIN32
public:
	/** End iterator constructor */
	FileFindIter() : handle(INVALID_HANDLE_VALUE) { }
	/** Begin iterator constructor, path in utf-8 */
	FileFindIter(const string& path) : handle(INVALID_HANDLE_VALUE) { 
		handle = ::FindFirstFile(path.c_str(), &data);
	}

	~FileFindIter() {
		if(handle != INVALID_HANDLE_VALUE) {
			::FindClose(handle);
		}
	}

	FileFindIter& operator++() {
		if(!::FindNextFile(handle, &data)) {
			::FindClose(handle);
			handle = INVALID_HANDLE_VALUE;
		}
		return *this;
	}

	struct DirData : public WIN32_FIND_DATA {
		string getFileName() {
			return cFileName;
		}

		bool isDirectory() {
			return (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0;
		}

		bool isHidden() {
			return ((dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) || (cFileName[0] == L'.'));
		}

		int64_t getSize() {
			return (int64_t)nFileSizeLow | ((int64_t)nFileSizeHigh)<<32;
		}

		u_int32_t getLastWriteTime() {
			return File::convertTime(&ftLastWriteTime);
		}
	};

	DirData& operator*() { return data; }
	DirData* operator->() { return &data; }

	bool operator!=(const FileFindIter& rhs) const { return handle != rhs.handle; }
private:
	DirData data;
	HANDLE handle;
#else
public:
	// TODO...
	FileFindIter() { }
	FileFindIter(const string&) { }
	void operator++(int) { }
	struct DirData {
		string getFileName() { return Util::emptyString; }
		bool isDirectory() { return false; }
		bool isHidden() { return false; }
		int64_t getSize() { return 0; }
		u_int32_t getLastWriteTime { return 0; }
	};
#warn FIXME
#endif
};

ShareManager::Directory* ShareManager::buildTree(const string& aName, Directory* aParent) {
	Directory* dir = new Directory(Util::getLastDir(aName), aParent);
	dir->addType(SearchManager::TYPE_DIRECTORY); // needed since we match our own name in directory searches
	dir->addSearchType(getMask(dir->getName()));

	Directory::File::Iter lastFileIter = dir->files.begin();
	
	FileFindIter end;
	for(FileFindIter i(aName + "*"); i != end; ++i) {
		string name = i->getFileName();

		if(name == "." || name == "..")
				continue;
			if(name.find('$') != string::npos) {
				LogManager::getInstance()->message(STRING(FORBIDDEN_DOLLAR_FILE) + name + " (" + STRING(SIZE) + ": " + Util::toString(File::getSize(name)) + " " + STRING(B) + ") (" + STRING(DIRECTORY) + ": \"" + aName + "\")", true);
				continue;
			}
			if(BOOLSETTING(REMOVE_FORBIDDEN)) {
			//check for forbidden file patterns
				string::size_type nameLen = name.size();
				if ((nameLen >= 28 && name.find("download") == 0 && name.rfind(".dat") == nameLen - 4 && Util::toInt64(name.substr(8, 16))) ||		//kazaa temps
					name.find("__INCOMPLETE__") == 0 ||		//winmx
					name.find("__incomplete__") == 0 ||		//winmx
					(nameLen > 9 && name.rfind(".GetRight") == nameLen - 9) ||
					(nameLen > 9 && name.rfind(".tdc") == nameLen - 4) ||
					(nameLen > 9 && name.rfind(".temp") == nameLen - 5) ||
					(nameLen > 9 && name.rfind("part.met") == nameLen - 8) ||
					(nameLen > 9 && name.rfind(".antifrag") == nameLen - 9) ||
					(nameLen > (39 + 2 + 5) && name.rfind(".dctmp") == nameLen - 6)) {	// dc++ temp files
					LogManager::getInstance()->message("Forbidden file will not be shared: " + name + " (" + STRING(SIZE) + ": " + Util::toString(File::getSize(name)) + " " + STRING(B) + ") (" + STRING(DIRECTORY) + ": \"" + aName + "\")", true);
						//size-=j->getSize();
						//++j;
						continue;
				}
			}
			if(!BOOLSETTING(SHARE_HIDDEN) && i->isHidden() )
				continue;

		if(i->isDirectory()) {
			string newName = aName + name + PATH_SEPARATOR;
			if((Util::stricmp(newName + PATH_SEPARATOR, SETTING(TEMP_DOWNLOAD_DIRECTORY)) != 0) && shareFolder(newName)) {
					dir->directories[name] = buildTree(newName, dir);
					dir->addSearchType(dir->directories[name]->getSearchTypes()); 
				}
			} else {
				// Not a directory, assume it's a file...make sure we're not sharing the settings file...
				if( (Util::stricmp(name.c_str(), "DCPlusPlus.xml") != 0) && 
					(Util::stricmp(name.c_str(), "Favorites.xml") != 0)) {

				int64_t size = i->getSize();

				HashManager::getInstance()->checkTTH(aName + name, size, i->getLastWriteTime());
				lastFileIter = dir->files.insert(lastFileIter, Directory::File(name, size, dir, NULL));

					}
				}
			}

	return dir;
}

void ShareManager::addTree(const string& fullName, Directory* dir) {
	bloom.add(Util::toLower(dir->getName()));

	for(Directory::MapIter i = dir->directories.begin(); i != dir->directories.end(); ++i) {
		Directory* d = i->second;
		addTree(fullName + d->getName() + PATH_SEPARATOR, d);
	}

	for(Directory::File::Iter i = dir->files.begin(); i != dir->files.end(); ) {
		const Directory::File& f2 = *i;

		// We're not changing anything cruical...
		Directory::File& f = const_cast<Directory::File&>(f2);
		string fileName = fullName + f.getName();

		f.setTTH(HashManager::getInstance()->getTTH(fileName));

		if(f.getTTH() != NULL) {
			addFile(dir, i++);
		} else {
			dir->files.erase(i++);
		}
	}
}

void ShareManager::addFile(Directory* dir, Directory::File::Iter i) {
	const Directory::File& f = *i;

	HashFileIter j = tthIndex.find(f.getTTH());
	if(j == tthIndex.end()) {
		dir->size+=f.getSize();
	} else {
		if(!SETTING(LIST_DUPES)) {
			LogManager::getInstance()->message(STRING(DUPLICATE_FILE_NOT_SHARED) + dir->getFullName() + f.getName() + " (" + STRING(SIZE) + ": " + Util::toString(f.getSize()) + " " + STRING(B) + ") " + STRING(DUPLICATE_MATCH) + j->second->getParent()->getFullName() + j->second->getName(), true);
			dir->files.erase(i);
			return;
		}
	}

	dir->addSearchType(getMask(f.getName()));
	dir->addType(getType(f.getName()));

	tthIndex.insert(make_pair(f.getTTH(), i));
	bloom.add(Util::toLower(f.getName()));
}

void ShareManager::removeTTH(TTHValue* tth, const Directory::File::Iter& iter) {
	pair<HashFileIter, HashFileIter> range = tthIndex.equal_range(tth);
	for(HashFileIter j = range.first; j != range.second; ++j) {
		if(j->second == iter) {
			tthIndex.erase(j);
			break;
		}
	}
}

void ShareManager::refresh(bool dirs /* = false */, bool aUpdate /* = true */, bool block /* = false */) throw(ShareException) {
	update = aUpdate;
	refreshDirs = dirs;
	join();
	start();
	if(block) {
		join();
	} else {
		setThreadPriority(Thread::LOW);
	}
}

int ShareManager::run() {
	LogManager::getInstance()->message(STRING(FILE_LIST_REFRESH_INITIATED), true);
	{
		if(refreshDirs) {
			lastFullUpdate = GET_TICK();
			StringPairList dirs;
			Directory::Map newDirs;
			{
				RLock l(cs);
				for(Directory::MapIter i = directories.begin(); i != directories.end(); ++i) {
					Directory* dp = buildTree(i->first, NULL);
					dp->setName(findVirtual(i->first)->first);
					newDirs.insert(make_pair(i->first, dp));
				}
			}
			{
				WLock l(cs);
				StringPairList dirs = virtualMap;
				for(StringPairIter i = dirs.begin(); i != dirs.end(); ++i) {
					removeDirectory(i->second);
				}
				bloom.clear();
		
				virtualMap = dirs;

				for(Directory::MapIter i = newDirs.begin(); i != newDirs.end(); ++i) {
					addTree(i->first, i->second);
					directories.insert(*i);
				}
			}
			refreshDirs = false;
		}

		listN++;
		generateXmlList();
		generateNmdcList();

	}

	LogManager::getInstance()->message(STRING(FILE_LIST_REFRESH_FINISHED), true);
	if(update) {
		ClientManager::getInstance()->infoUpdated(false);
	}
	return 0;
}
		
void ShareManager::generateXmlList() {
	if(xmlDirty/* && lastXmlUpdate + 15 * 60 * 1000 < GET_TICK()*/) {
		//listN++;

		try {
			string tmp2;
			string indent;

			string newXmlName = Util::getAppPath() + "files" + Util::toString(listN) + ".xml.bz2";
			{
				FilteredOutputStream<BZFilter, true> newXmlFile(new File(newXmlName, File::WRITE, File::TRUNCATE | File::CREATE));
				newXmlFile.write(SimpleXML::utf8Header);
				newXmlFile.write("<FileListing Version=\"1\" Generator=\"DC++ " DCVERSIONSTRING "\">\r\n");
				for(Directory::MapIter i = directories.begin(); i != directories.end(); ++i) {
					i->second->toXml(newXmlFile, indent, tmp2);
				}
				newXmlFile.write("</FileListing>");
				newXmlFile.flush();
			}

			if(xFile != NULL) {
				delete xFile;
				xFile = NULL;
				File::deleteFile(getBZXmlFile());
			}
			try {
				File::copyFile(newXmlName, Util::getAppPath() + "files.xml.bz2");
			} catch(const FileException&) {
				// Ignore, this is for caching only...
			}
			xFile = new File(newXmlName, File::READ, File::OPEN);
			setBZXmlFile(newXmlName);
			bzXmlListLen = File::getSize(newXmlName);
		} catch(const Exception&) {
			// No new file lists...
		}

		xmlDirty = false;
		lastXmlUpdate = GET_TICK();
	}
}

void ShareManager::generateNmdcList() {
	if(nmdcDirty/* && lastNmdcUpdate + 15 * 60 * 1000 < GET_TICK()*/) {
		//listN++;

		try {
			string tmp;
			string tmp2;
			string indent;

			for(Directory::MapIter i = directories.begin(); i != directories.end(); ++i) {
				i->second->toNmdc(tmp, indent, tmp2);
			}

			string newName = Util::getAppPath() + "MyList" + Util::toString(listN) + ".DcLst";
			tmp2.clear();
			CryptoManager::getInstance()->encodeHuffman(tmp, tmp2);
			File(newName, File::WRITE, File::CREATE | File::TRUNCATE).write(tmp2);

			if(lFile != NULL) {
				delete lFile;
				lFile = NULL;
				File::deleteFile(getListFile());
			}
			lFile = new File(newName, File::READ, File::OPEN);
			setListFile(newName);
			listLen = File::getSize(newName);
		} catch(const Exception&) {
			// No new file lists...
		}
		
		nmdcDirty = false;
		lastNmdcUpdate = GET_TICK();
	}
}

static const string& escaper(const string& n, string& tmp) {
	if(SimpleXML::needsEscape(n, false, false)) {
		tmp.clear();
		tmp.append(n);
		return SimpleXML::escape(tmp, false, false);
	}
	return n;
}

#define LITERAL(n) n, sizeof(n)-1
void ShareManager::Directory::toNmdc(string& nmdc, string& indent, string& tmp2) {
	tmp2.clear();
	nmdc.append(indent);
	nmdc.append(name);
	nmdc.append(LITERAL("\r\n"));

	indent += '\t';
	for(MapIter i = directories.begin(); i != directories.end(); ++i) {
		i->second->toNmdc(nmdc, indent, tmp2);
	}
	
	Directory::File::Iter j = files.begin();
	for(Directory::File::Iter i = files.begin(); i != files.end(); ++i) {
		const Directory::File& f = *i;
		nmdc.append(indent);
		tmp2.clear();		
		nmdc.append(f.getName());
		nmdc.append(LITERAL("|"));
		nmdc.append(Util::toString(f.getSize()));
		nmdc.append(LITERAL("\r\n"));
		}
	indent.erase(indent.length()-1);
}

void ShareManager::Directory::toXml(OutputStream& xmlFile, string& indent, string& tmp2) {
	xmlFile.write(indent);
	xmlFile.write(LITERAL("<Directory Name=\""));
	xmlFile.write(escaper(Util::toUtf8(name, tmp2), tmp2));
	xmlFile.write(LITERAL("\">\r\n"));

	indent += '\t';
	for(MapIter i = directories.begin(); i != directories.end(); ++i) {
		i->second->toXml(xmlFile, indent, tmp2);
		}

	for(Directory::File::Iter i = files.begin(); i != files.end(); ++i) {
		const Directory::File& f = *i;

		xmlFile.write(indent);
		xmlFile.write(LITERAL("<File Name=\""));
		xmlFile.write(escaper(Util::toUtf8(f.getName(), tmp2), tmp2));
		xmlFile.write(LITERAL("\" Size=\""));
		xmlFile.write(Util::toString(f.getSize()));
		if(f.getTTH()) {
				tmp2.clear();
			xmlFile.write(LITERAL("\" TTH=\""));
			xmlFile.write(f.getTTH()->toBase32(tmp2));
		}
		xmlFile.write(LITERAL("\"/>\r\n"));
	}
	indent.erase(indent.length()-1);
	xmlFile.write(indent);
	xmlFile.write(LITERAL("</Directory>\r\n"));
}


// These ones we can look up as ints (4 bytes...)...

static const char* typeAudio[] = { ".mp3", ".mp2", ".mid", ".wav", ".ogg", ".wma", ".669", ".aac", ".aif", ".amf", ".ams", ".ape", ".dbm", ".dmf", ".dsm", ".far", ".mdl", ".med", ".mod", ".mol", ".mp1", ".mp4", ".mpa", ".mpc", ".mpp", ".mtm", ".nst", ".okt", ".psm", ".ptm", ".rmi", ".s3m", ".stm", ".ult", ".umx", ".wow" };
static const char* typeCompressed[] = { ".zip", ".ace", ".rar", ".arj", ".hqx", ".lha", ".sea", ".tar", ".tgz", ".uc2" };
static const char* typeDocument[] = { ".htm", ".doc", ".txt", ".nfo", ".pdf", ".chm" };
static const char* typeExecutable[] = { ".exe", ".com" };
static const char* typePicture[] = { ".jpg", ".gif", ".png", ".eps", ".img", ".pct", ".psp", ".pic", ".tif", ".rle", ".bmp", ".pcx", ".jpe", ".dcx", ".emf", ".ico", ".psd", ".tga", ".wmf", ".xif" };
static const char* typeVideo[] = { ".mpg", ".mov", ".asf", ".avi", ".pxp", ".wmv", ".ogm", ".mkv", ".m1v", ".m2v", ".mpe", ".mps", ".mpv", ".ram", ".vob" };

static const string type2Audio[] = { ".au", ".it", ".ra", ".xm", ".aiff", ".flac", ".midi", };
static const string type2Compressed[] = { ".gz" };
static const string type2Picture[] = { ".ai", ".ps", ".pict", ".jpeg", ".tiff" };
static const string type2Video[] = { ".rm", ".divx", ".mpeg", ".mp1v", ".mp2v", ".mpv1", ".mpv2", ".qt", ".rv", ".vivo" };

#define IS_TYPE(x) ( type == (*((u_int32_t*)x)) )
#define IS_TYPE2(x) (Util::stricmp(aString.c_str() + aString.length() - x.length(), x.c_str()) == 0)

static bool checkType(const string& aString, int aType) {
	if(aType == SearchManager::TYPE_ANY)
		return true;

	if(aString.length() < 5)
		return false;
	
	const char* c = aString.c_str() + aString.length() - 3;
	u_int32_t type = '.' | (Util::toLower(c[0]) << 8) | (Util::toLower(c[1]) << 16) | (((u_int32_t)Util::toLower(c[2])) << 24);

	switch(aType) {
	case SearchManager::TYPE_AUDIO:
		{
			for(size_t i = 0; i < (sizeof(typeAudio) / sizeof(typeAudio[0])); i++) {
				if(IS_TYPE(typeAudio[i])) {
					return true;
				}
			}
			for(size_t i = 0; i < (sizeof(type2Audio) / sizeof(type2Audio[0])); i++) {
				if(IS_TYPE2(type2Audio[i])) {
					return true;
				}
			}
		}
		break;
	case SearchManager::TYPE_COMPRESSED:
		{
			for(size_t i = 0; i < (sizeof(typeCompressed) / sizeof(typeCompressed[0])); i++) {
				if(IS_TYPE(typeCompressed[i])) {
					return true;
				}
			}
			if( IS_TYPE2(type2Compressed[0]) ) {
				return true;
			}
		}
		break;
	case SearchManager::TYPE_DOCUMENT:
			for(size_t i = 0; i < (sizeof(typeDocument) / sizeof(typeDocument[0])); i++) {
				if(IS_TYPE(typeDocument[i])) {
					return true;
				}
			}
		break;
	case SearchManager::TYPE_EXECUTABLE:
		if(IS_TYPE(typeExecutable[0]) || IS_TYPE(typeExecutable[1])) {
			return true;
		}
		break;
	case SearchManager::TYPE_PICTURE:
		{
			for(size_t i = 0; i < (sizeof(typePicture) / sizeof(typePicture[0])); i++) {
				if(IS_TYPE(typePicture[i])) {
					return true;
				}
			}
			for(size_t i = 0; i < (sizeof(type2Picture) / sizeof(type2Picture[0])); i++) {
				if(IS_TYPE2(type2Picture[i])) {
					return true;
				}
			}
		}
		break;
	case SearchManager::TYPE_VIDEO:
		{
			for(size_t i = 0; i < (sizeof(typeVideo) / sizeof(typeVideo[0])); i++) {
				if(IS_TYPE(typeVideo[i])) {
					return true;
				}
			}
			for(size_t i = 0; i < (sizeof(type2Video) / sizeof(type2Video[0])); i++) {
				if(IS_TYPE2(type2Video[i])) {
					return true;
				}
			}
		}
		break;
	default:
		dcasserta(0);
		break;
	}
	return false;
}

SearchManager::TypeModes ShareManager::getType(const string& aFileName) {
	if(aFileName[aFileName.length() - 1] == PATH_SEPARATOR) {
		return SearchManager::TYPE_DIRECTORY;
	}

	if(checkType(aFileName, SearchManager::TYPE_VIDEO))
		return SearchManager::TYPE_VIDEO;
	else if(checkType(aFileName, SearchManager::TYPE_AUDIO))
		return SearchManager::TYPE_AUDIO;
	else if(checkType(aFileName, SearchManager::TYPE_COMPRESSED))
		return SearchManager::TYPE_COMPRESSED;
	else if(checkType(aFileName, SearchManager::TYPE_DOCUMENT))
		return SearchManager::TYPE_DOCUMENT;
	else if(checkType(aFileName, SearchManager::TYPE_EXECUTABLE))
		return SearchManager::TYPE_EXECUTABLE;
	else if(checkType(aFileName, SearchManager::TYPE_PICTURE))
		return SearchManager::TYPE_PICTURE;

	return SearchManager::TYPE_ANY;
}

/**
 * The mask is a set of bits that say which words a file matches. Each bit means
 * that a fileName matches the word at position n-1 in the words list where n is
 * the bit number. bit 0 is only set when no words match.
 */
u_int32_t ShareManager::getMask(const string& fileName) {
	u_int32_t mask = 0;
	int n = 1;
	for(StringIter i = words.begin(); i != words.end(); ++i, n++) {
		if(Util::findSubString(fileName, *i) != string::npos) {
			mask |= (1 << n);
		}
	}
	return (mask == 0) ? 1 : mask;
}

u_int32_t ShareManager::getMask(StringList& l) {
	u_int32_t mask = 0;
	int n = 1;

	for(StringIter i = words.begin(); i != words.end(); ++i, n++) {
		for(StringIter j = l.begin(); j != l.end(); ++j) {
			if(Util::findSubString(*j, *i) != string::npos) {
				mask |= (1 << n);
			}
		}
	}
	return (mask == 0) ? 1 : mask;	
}

u_int32_t ShareManager::getMask(StringSearch::List& l) {
	u_int32_t mask = 0;
	int n = 1;

	for(StringIter i = words.begin(); i != words.end(); ++i, n++) {
		for(StringSearch::Iter j = l.begin(); j != l.end(); ++j) {
			if(Util::findSubString(j->getPattern(), *i) != string::npos) {
				mask |= (1 << n);
			}
		}
	}
	return (mask == 0) ? 1 : mask;	
}

/**
 * Alright, the main point here is that when searching, a search string is most often found in 
 * the filename, not directory name, so we want to make that case faster. Also, we want to
 * avoid changing StringLists unless we absolutely have to --> this should only be done if a string
 * has been matched in the directory name. This new stringlist should also be used in all descendants,
 * but not the parents...
 */
void ShareManager::Directory::search(SearchResult::List& aResults, StringSearch::List& aStrings, int aSearchType, int64_t aSize, int aFileType, Client* aClient, StringList::size_type maxResults, u_int32_t mask) throw() {
	// Skip everything if there's nothing to find here (doh! =)
	if(!hasType(aFileType))
		return;

	if(!hasSearchType(mask))
		return;

	StringSearch::List* cur = &aStrings;
	auto_ptr<StringSearch::List> newStr;

	// Find any matches in the directory name
	for(StringSearch::Iter k = aStrings.begin(); k != aStrings.end(); ++k) {
		if(k->match(name)) {
			if(!newStr.get()) {
				newStr = auto_ptr<StringSearch::List>(new StringSearch::List(aStrings));
			}
			dcassert(find(newStr->begin(), newStr->end(), *k) != newStr->end());
			newStr->erase(find(newStr->begin(), newStr->end(), *k));
			u_int32_t xmask = ShareManager::getInstance()->getMask(k->getPattern());
			if(xmask != 1) {
				mask &= ~xmask;
			}
		}
	}

	if(newStr.get() != 0) {
		cur = newStr.get();
	}

	bool sizeOk = (aSearchType != SearchManager::SIZE_ATLEAST) || (aSize == 0);
	if( (cur->empty()) && 
		(((aFileType == SearchManager::TYPE_ANY) && sizeOk) || (aFileType == SearchManager::TYPE_DIRECTORY)) ) {
		// We satisfied all the search words! Add the directory...
		SearchResult* sr = new SearchResult(aClient, SearchResult::TYPE_DIRECTORY, 
			0, getFullName(), NULL);
		aResults.push_back(sr);
		ShareManager::getInstance()->setHits(ShareManager::getInstance()->getHits()+1);
	}

	if(aFileType != SearchManager::TYPE_DIRECTORY) {
		for(File::Iter i = files.begin(); i != files.end(); ++i) {
			
			if(aSearchType == SearchManager::SIZE_ATLEAST && aSize > i->getSize()) {
				continue;
			} else if(aSearchType == SearchManager::SIZE_ATMOST && aSize < i->getSize()) {
				continue;
			}	
			StringSearch::Iter j = cur->begin();
			for(; j != cur->end() && j->match(i->getName()); ++j) 
				;	// Empty
			
			if(j != cur->end())
				continue;
			
			// Check file type...
			if(checkType(i->getName(), aFileType)) {
				
				SearchResult* sr = new SearchResult(aClient, SearchResult::TYPE_FILE, 
					i->getSize(), getFullName() + i->getName(), i->getTTH());
				aResults.push_back(sr);
				ShareManager::getInstance()->setHits(ShareManager::getInstance()->getHits()+1);
				if(aResults.size() >= maxResults) {
					break;
				}
			}
		}
	}

	for(Directory::MapIter l = directories.begin(); (l != directories.end()) && (aResults.size() < maxResults); ++l) {
		l->second->search(aResults, *cur, aSearchType, aSize, aFileType, aClient, maxResults, mask);
	}
}

void ShareManager::search(SearchResult::List& results, const string& aString, int aSearchType, int64_t aSize, int aFileType, Client* aClient, StringList::size_type maxResults) {
	RLock l(cs);
	if(aFileType == SearchManager::TYPE_HASH) {
		if(aString.compare(0, 4, "TTH:") == 0) {
			TTHValue tth(aString.substr(4));
			HashFileIter i = tthIndex.find(&tth);
			if(i != tthIndex.end()) {
				dcassert(i->second->getTTH() != NULL);

				SearchResult* sr = new SearchResult(aClient, SearchResult::TYPE_FILE, 
					i->second->getSize(), i->second->getParent()->getFullName() + i->second->getName(), 
					i->second->getTTH());

				results.push_back(sr);
				ShareManager::getInstance()->setHits(ShareManager::getInstance()->getHits()+1);
			}
		}
		return;
	}
	StringTokenizer t(Util::toLower(aString), '$');
	StringList& sl = t.getTokens();
	if(!bloom.match(sl))
		return;

	StringSearch::List ssl;
	for(StringList::iterator i = sl.begin(); i != sl.end(); ++i) {
		if(!i->empty()) {
			ssl.push_back(StringSearch(*i));
		}
	}
	if(ssl.empty())
		return;
	u_int32_t mask = getMask(sl);

	for(Directory::MapIter j = directories.begin(); (j != directories.end()) && (results.size() < maxResults); ++j) {
		j->second->search(results, ssl, aSearchType, aSize, aFileType, aClient, maxResults, mask);
	}
}
	
namespace {
	inline u_int16_t toCode(char a, char b) { return (u_int16_t)a | ((u_int16_t)b)<<8; }
}

ShareManager::AdcSearch::AdcSearch(const StringList& params) : include(&includeX), gt(0), 
	lt(numeric_limits<int64_t>::max()), hasRoot(false), isDirectory(false)
{
	for(StringIterC i = params.begin(); i != params.end(); ++i) {
		const string& p = *i;
		if(p.length() <= 2)
			continue;

		u_int16_t cmd = toCode(p[0], p[1]);
		if(toCode('T', 'L') == cmd) {
			hasRoot = true;
			root = p.substr(2);
			return;
		} else if(toCode('+', '+') == cmd) {
			includeX.push_back(StringSearch(p.substr(2)));		
		} else if(toCode('-', '-') == cmd) {
			exclude.push_back(StringSearch(p.substr(2)));
		} else if(toCode('E', 'X') == cmd) {
			ext.push_back(p.substr(2));
		} else if(toCode('>', '=') == cmd) {
			gt = Util::toInt64(p.substr(2));
		} else if(toCode('<', '=') == cmd) {
			lt = Util::toInt64(p.substr(2));
		} else if(toCode('=', '=') == cmd) {
			lt = gt = Util::toInt64(p.substr(2));
		} else if(toCode('D', 'O') == cmd) {
			isDirectory = (p[2] != '0');
		}
	}
}

void ShareManager::Directory::search(SearchResult::List& aResults, AdcSearch& aStrings, Client* aClient, StringList::size_type maxResults, u_int32_t mask) throw() {
	if(!hasSearchType(mask))
		return;

	StringSearch::List* cur = aStrings.include;
	StringSearch::List* old = aStrings.include;

	auto_ptr<StringSearch::List> newStr;

	// Find any matches in the directory name
	for(StringSearch::Iter k = cur->begin(); k != cur->end(); ++k) {
		if(k->match(name) && !aStrings.isExcluded(name)) {
			if(!newStr.get()) {
				newStr = auto_ptr<StringSearch::List>(new StringSearch::List(*cur));
			}
			dcassert(find(newStr->begin(), newStr->end(), *k) != newStr->end());
			newStr->erase(find(newStr->begin(), newStr->end(), *k));
			u_int32_t xmask = ShareManager::getInstance()->getMask(k->getPattern());
			if(xmask != 1) {
				mask &= ~xmask;
			}
		}
	}

	if(newStr.get() != 0) {
		cur = newStr.get();
	}

	bool sizeOk = (aStrings.gt == 0);
	if( cur->empty() && aStrings.ext.empty() && sizeOk ) {
		// We satisfied all the search words! Add the directory...
		SearchResult* sr = new SearchResult(aClient, SearchResult::TYPE_DIRECTORY, 
			0, getFullName(), NULL);
		aResults.push_back(sr);
		ShareManager::getInstance()->setHits(ShareManager::getInstance()->getHits()+1);
	}

	if(!aStrings.isDirectory) {
		for(File::Iter i = files.begin(); i != files.end(); ++i) {

			if(!(i->getSize() >= aStrings.gt)) {
				continue;
			} else if(!(i->getSize() <= aStrings.lt)) {
				continue;
			}	

			if(aStrings.isExcluded(i->getName()))
				continue;

			StringSearch::Iter j = cur->begin();
			for(; j != cur->end() && j->match(i->getName()); ++j) 
				;	// Empty

			if(j != cur->end())
				continue;

			// Check file type...
			if(aStrings.hasExt(i->getName())) {

				SearchResult* sr = new SearchResult(aClient, SearchResult::TYPE_FILE, 
					i->getSize(), getFullName() + i->getName(), i->getTTH());
				aResults.push_back(sr);
				ShareManager::getInstance()->addHits(1);
				if(aResults.size() >= maxResults) {
					return;
				}
			}
		}
	}

	for(Directory::MapIter l = directories.begin(); (l != directories.end()) && (aResults.size() < maxResults); ++l) {
		l->second->search(aResults, aStrings, aClient, maxResults, mask);
	}
	aStrings.include = old;
}

void ShareManager::search(SearchResult::List& results, const StringList& params, Client* aClient, StringList::size_type maxResults) {
	AdcSearch srch(params);	

	RLock l(cs);

	if(srch.hasRoot) {
		HashFileIter i = tthIndex.find(&srch.root);
		if(i != tthIndex.end()) {
			dcassert(i->second->getTTH() != NULL);

			SearchResult* sr = new SearchResult(aClient,
				SearchResult::TYPE_FILE, i->second->getSize(), i->second->getParent()->getFullName() + i->second->getName(), 
				i->second->getTTH());
			results.push_back(sr);
			ShareManager::getInstance()->addHits(1);
		}
		return;
	}

	for(StringSearch::Iter i = srch.includeX.begin(); i != srch.includeX.end(); ++i) {
		if(!bloom.match(i->getPattern()))
			return;
	}

	u_int32_t mask = getMask(srch.includeX);

	for(Directory::MapIter j = directories.begin(); (j != directories.end()) && (results.size() < maxResults); ++j) {
		j->second->search(results, srch, aClient, maxResults, mask);
	}
}

ShareManager::Directory* ShareManager::getDirectory(const string& fname) {
	for(Directory::MapIter mi = directories.begin(); mi != directories.end(); ++mi) {
		if(Util::strnicmp(fname, mi->first, mi->first.length()) == 0) {
			Directory* d = mi->second;

			string::size_type i;
			string::size_type j = mi->first.length();
			while( (i = fname.find(PATH_SEPARATOR, j)) != string::npos) {
				mi = d->directories.find(fname.substr(j, i-j));
				j = i + 1;
				if(mi == d->directories.end())
					return NULL;
				d = mi->second;
			}
			return d;
		}
	}
	return NULL;
}

void ShareManager::on(DownloadManagerListener::Complete, Download* d) throw() {
	if(BOOLSETTING(ADD_FINISHED_INSTANTLY)) {
		// Check if finished download is supposed to be shared
		WLock l(cs);
		const string& n = d->getTarget();
		for(Directory::MapIter i = directories.begin(); i != directories.end(); i++) {
			if(Util::strnicmp(i->first.c_str(), n.c_str(), i->first.size()) == 0 && n[i->first.size()] == PATH_SEPARATOR) {
				string s = n.substr(i->first.size()+1);
				try {
					// Schedule for hashing, it'll be added automatically later on...
					HashManager::getInstance()->checkTTH(n, d->getSize(), 0);
				} catch(const Exception&) {
					// Not a vital feature...
				}
				break;
			}
		}
	}
}

void ShareManager::on(HashManagerListener::TTHDone, const string& fname, TTHValue* root) throw() {
		WLock l(cs);
		Directory* d = getDirectory(fname);
		if(d != NULL) {
			Directory::File::Iter i = find_if(d->files.begin(), d->files.end(), Directory::File::StringComp(Util::getFileName(fname)));
			if(i != d->files.end()) {
				if(i->getTTH() != NULL) { // TTH of file updated?				
					dcassert(tthIndex.find(i->getTTH()) != tthIndex.end());
					removeTTH(i->getTTH(), i);
				}
				// Get rid of false constness...
				Directory::File* f = const_cast<Directory::File*>(&(*i));
				f->setTTH(root);
				tthIndex.insert(make_pair(root, i));
			} else {
				string name = Util::getFileName(fname);
				int64_t size = File::getSize(fname);
				Directory::File::Iter it = d->files.insert(Directory::File(name, size, d, root)).first;
				addFile(d, it);
			}

		}
}

void ShareManager::on(HashManagerListener::Finished) {
	setDirty();
	generateXmlList();
	LogManager::getInstance()->message(STRING(HASHING_FINISHED), true);
}

void ShareManager::on(TimerManagerListener::Minute, u_int32_t tick) throw() {
	if(BOOLSETTING(AUTO_UPDATE_LIST)) {
		if(lastFullUpdate + 60 * 60 * 1000 < tick) {
			try {
				setDirty();
				refresh(true, true);
			} catch(const ShareException&) {
			}
		}
	}
}

bool ShareManager::shareFolder(const string& path, bool thoroughCheck /* = false */) {
	if(thoroughCheck)	// check if it's part of the share before checking if it's in the exclusions
	{
		bool result = false;
		for(Directory::MapIter i = directories.begin(); i != directories.end(); ++i)
		{
			// is it a perfect match
			if((path.size() == i->first.size()) && (Util::stricmp(path, i->first) == 0))
				return true;
			else if (path.size() > i->first.size()) // this might be a subfolder of a shared folder
			{
				string temp = path.substr(0, i->first.size());
				// if the left-hand side matches and there is a \ in the remainder then it is a subfolder
				if((Util::stricmp(temp, i->first) == 0) && (path.find('\\', i->first.size()) != string::npos))
				{
					result = true;
					break;
				}
			}
		}

		if(!result)
			return false;
	}

	// check if it's an excluded folder or a sub folder of an excluded folder
	for(StringIter j = notShared.begin(); j != notShared.end(); ++j)
	{		
		if(Util::stricmp(path, *j) == 0)
			return false;

		if(thoroughCheck)
		{
			if(path.size() > (*j).size())
			{
				string temp = path.substr(0, (*j).size());
				if((Util::stricmp(temp, *j) == 0) && (path[(*j).size()] == '\\'))
					return false;
			}
		}
	}
	return true;
}

int64_t ShareManager::addExcludeFolder(const string &path) {
	// make sure this is a sub folder of a shared folder
	bool result = false;
	for(Directory::MapIter i = directories.begin(); i != directories.end(); ++i)
	{
		if(path.size() > i->first.size())
		{
			string temp = path.substr(0, i->first.size());
			if(Util::stricmp(temp, i->first) == 0)
			{
				result = true;
				break;
			}
		}
	}

	if(!result)
		return 0;

	// Make sure this not a subfolder of an already excluded folder
	for(StringIter j = notShared.begin(); j != notShared.end(); ++j)
	{
		if(path.size() >= (*j).size())
		{
			string temp = path.substr(0, (*j).size());
			if(Util::stricmp(temp, *j) == 0)
				return 0;
		}
	}

	// remove all sub folder excludes
	int64_t bytesNotCounted = 0;
	for(StringIter j = notShared.begin(); j != notShared.end(); ++j)
	{
		if(path.size() < (*j).size())
		{
			string temp = (*j).substr(0, path.size());
			if(Util::stricmp(temp, path) == 0)
			{
				bytesNotCounted += Util::getDirSize(*j);
				j = notShared.erase(j);
				j--;
			}
		}
	}

	// add it to the list
	notShared.push_back(path);

	int64_t bytesRemoved = Util::getDirSize(path);

	return (bytesRemoved - bytesNotCounted);
}

int64_t ShareManager::removeExcludeFolder(const string &path, bool returnSize /* = true */) {
	int64_t bytesAdded = 0;
	// remove all sub folder excludes
	for(StringIter j = notShared.begin(); j != notShared.end(); ++j)
	{
		if(path.size() <= (*j).size())
		{
			string temp = (*j).substr(0, path.size());
			if(Util::stricmp(temp, path) == 0)
			{
				if(returnSize) // this needs to be false if the files don't exist anymore
					bytesAdded += Util::getDirSize(*j);
				
				j = notShared.erase(j);
				j--;
			}
		}
	}
	
	return bytesAdded;
}

/**
 * @file
 * $Id$
 */
