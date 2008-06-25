/*
 * Copyright (C) 2001-2008 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_UTIL_H
#define DCPLUSPLUS_DCPP_UTIL_H

#ifndef _WIN32
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#endif

#include "Text.h"

namespace dcpp {

template<typename T, bool flag> struct ReferenceSelector {
	typedef T ResultType;
};
template<typename T> struct ReferenceSelector<T,true> {
	typedef const T& ResultType;
};

template<typename T> class IsOfClassType {
public:
	template<typename U> static char check(int U::*);
	template<typename U> static float check(...);
public:
	enum { Result = sizeof(check<T>(0)) };
};

template<typename T> struct TypeTraits {
	typedef IsOfClassType<T> ClassType;
	typedef ReferenceSelector<T, ((ClassType::Result == 1) || (sizeof(T) > sizeof(char*)) ) > Selector;
	typedef typename Selector::ResultType ParameterType;
};

#define GETSET(type, name, name2) \
private: type name; \
public: TypeTraits<type>::ParameterType get##name2() const { return name; } \
	void set##name2(TypeTraits<type>::ParameterType a##name2) { name = a##name2; }

#define LIT(x) x, (sizeof(x)-1)

/** Evaluates op(pair<T1, T2>.first, compareTo) */
template<class T1, class T2, class op = equal_to<T1> >
class CompareFirst {
public:
	CompareFirst(const T1& compareTo) : a(compareTo) { }
	bool operator()(const pair<T1, T2>& p) { return op()(p.first, a); }
private:
	CompareFirst& operator=(const CompareFirst&);
	const T1& a;
};

/** Evaluates op(pair<T1, T2>.second, compareTo) */
template<class T1, class T2, class op = equal_to<T2> >
class CompareSecond {
public:
	CompareSecond(const T2& compareTo) : a(compareTo) { }
	bool operator()(const pair<T1, T2>& p) { return op()(p.second, a); }
private:
	CompareSecond& operator=(const CompareSecond&);
	const T2& a;
};

/** Evaluates op(pair<T1, T2>.second, compareTo) */
template<class T1, class T2, class T3, class op = equal_to<T2> >
class CompareSecondFirst {
public:
	CompareSecondFirst(const T2& compareTo) : a(compareTo) { }
	bool operator()(const pair<T1, pair<T2, T3>>& p) { return op()(p.second.first, a); }
private:
	CompareSecondFirst& operator=(const CompareSecondFirst&);
	const T2& a;
};

/** 
 * Compares two values
 * @return -1 if v1 < v2, 0 if v1 == v2 and 1 if v1 > v2
 */
template<typename T1>
inline int compare(const T1& v1, const T1& v2) { return (v1 < v2) ? -1 : ((v1 == v2) ? 0 : 1); }

class Util  
{
public:
	static tstring emptyStringT;
	static string emptyString;
	static wstring emptyStringW;

	static void initialize();

	/** Path of temporary storage */
	static string getTempPath() {
#ifdef _WIN32
		TCHAR buf[MAX_PATH + 1];
		DWORD x = GetTempPath(MAX_PATH, buf);
		return Text::fromT(tstring(buf, x));
#else
		return "/tmp/";
#endif
	}

	/** Path of configuration files */
	static const string& getConfigPath() { return configPath; }
	static const string& getDataPath() { return dataPath; }
	static const string& getSystemPath() { return systemPath; }

	/** Path of file lists */
	static string getListPath() { return getConfigPath() + "FileLists" PATH_SEPARATOR_STR; }
	/** Path of hub lists */
	static string getHubListsPath() { return getConfigPath() + "HubLists" PATH_SEPARATOR_STR; }
	/** Notepad filename */
	static string getNotepadFile() { return getConfigPath() + "Notepad.txt"; }

	static string translateError(int aError) {
#ifdef _WIN32
		LPVOID lpMsgBuf;
		DWORD chars = FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			aError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
			);
		if(chars == 0) {
			return string();
		}
		string tmp = Text::fromT((LPCTSTR)lpMsgBuf);
		// Free the buffer.
		LocalFree( lpMsgBuf );
		string::size_type i = 0;

		while( (i = tmp.find_first_of("\r\n", i)) != string::npos) {
			tmp.erase(i, 1);
		}
		return tmp;
#else // _WIN32
		return Text::toUtf8(strerror(aError));
#endif // _WIN32
	}

	static long getUptime() { return mUptimeSeconds; }
	static void increaseUptime() { mUptimeSeconds++; }

	static string getFilePath(const string& path) {
		string::size_type i = path.rfind(PATH_SEPARATOR);
		return (i != string::npos) ? path.substr(0, i + 1) : path;
	}
	static string getFileName(const string& path) {
		string::size_type i = path.rfind(PATH_SEPARATOR);
		return (i != string::npos) ? path.substr(i + 1) : path;
	}
	static string getFileExt(const string& path) {
		string::size_type i = path.rfind('.');
		return (i != string::npos) ? path.substr(i) : Util::emptyString;
	}
	static string getLastDir(const string& path) {
		string::size_type i = path.rfind(PATH_SEPARATOR);
		if(i == string::npos)
			return Util::emptyString;
		string::size_type j = path.rfind(PATH_SEPARATOR, i-1);
		return (j != string::npos) ? path.substr(j+1, i-j-1) : path;
	}

	static wstring getFilePath(const wstring& path) {
		wstring::size_type i = path.rfind(PATH_SEPARATOR);
		return (i != wstring::npos) ? path.substr(0, i + 1) : path;
	}
	static wstring getFileName(const wstring& path) {
		wstring::size_type i = path.rfind(PATH_SEPARATOR);
		return (i != wstring::npos) ? path.substr(i + 1) : path;
	}
	static wstring getFileExt(const wstring& path) {
		wstring::size_type i = path.rfind('.');
		return (i != wstring::npos) ? path.substr(i) : Util::emptyStringW;
	}
	static wstring getLastDir(const wstring& path) {
		wstring::size_type i = path.rfind(PATH_SEPARATOR);
		if(i == wstring::npos)
			return Util::emptyStringW;
		wstring::size_type j = path.rfind(PATH_SEPARATOR, i-1);
		return (j != wstring::npos) ? path.substr(j+1, i-j-1) : path;
	}

	static void decodeUrl(const string& aUrl, string& aServer, uint16_t& aPort, string& aFile);
	static string validateFileName(string aFile);
	static string cleanPathChars(string aNick);
	static string formatStatus(int iStatus) {
		switch(iStatus) {
			case 1: return "Normal (1)";
			case 2: return "Away (2)";
			case 3: return "Away (3)";
			case 4: return "Fileserver (4)";
			case 5: return "Fileserver (5)";
			case 6: return "Fileserver Away (6)";
			case 7: return "Fileserver Away (7)";
			case 8: return "Fireball (8)";
			case 9: return "Fireball (9)";
			case 10: return "Fireball Away (10)";
			case 11: return "Fireball Away (11)";
			default: return "Unknown (" + toString(iStatus) + ")";
		}
	}
	
	static string formatBytes(const string& aString) { return formatBytes(toInt64(aString)); }
	static string formatMessage(const string& nick, const string& message, bool thirdPerson);

	static string getShortTimeString();

	static string getTimeString();
	static string toAdcFile(const string& file);
	static string toNmdcFile(const string& file);
	
	static string formatBytes(int64_t aBytes);
	static wstring formatBytesW(int64_t aBytes);

	static wstring formatExactSize(int64_t aBytes);

	static wstring formatSeconds(int64_t aSec, bool supressHours = false) {
		wchar_t buf[64];
		if (!supressHours)
			snwprintf(buf, sizeof(buf), L"%01lu:%02d:%02d", (unsigned long)(aSec / (60*60)), (int)((aSec / 60) % 60), (int)(aSec % 60));
		else
			snwprintf(buf, sizeof(buf), L"%02d:%02d", (int)(aSec / 60), (int)(aSec % 60));	
		return buf;
	}

	static string formatParams(const string& msg, StringMap& params, bool filter);
	static string formatTime(const string &msg, const time_t t);
	static string formatRegExp(const string& msg, StringMap& params);

	static int64_t toInt64(const string& aString) {
#ifdef _WIN32
		return _atoi64(aString.c_str());
#else
		return strtoll(aString.c_str(), (char **)NULL, 10);
#endif
	}

	static int toInt(const string& aString) {
		return atoi(aString.c_str());
	}
	static uint32_t toUInt32(const string& str) {
		return toUInt32(str.c_str());
	}
	static uint32_t toUInt32(const char* c) {
#ifdef _MSC_VER
		/*
		* MSVC's atoi returns INT_MIN/INT_MAX if out-of-range; hence, a number
		* between INT_MAX and UINT_MAX can't be converted back to uint32_t.
		*/
		uint32_t ret = atoi(c);
		if(errno == ERANGE)
			return (uint32_t)_atoi64(c);
		return ret;
#else
		return (uint32_t)atoi(c);
#endif
	}

	static double toDouble(const string& aString) {
		// Work-around for atof and locales...
		lconv* lv = localeconv();
		string::size_type i = aString.find_last_of(".,");
		if(i != string::npos && aString[i] != lv->decimal_point[0]) {
			string tmp(aString);
			tmp[i] = lv->decimal_point[0];
			return atof(tmp.c_str());
		}
		return atof(aString.c_str());
	}

	static float toFloat(const string& aString) {
		return (float)toDouble(aString.c_str());
	}

	static string toString(short val) {
		char buf[8];
		snprintf(buf, sizeof(buf), "%d", (int)val);
		return buf;
	}
	static string toString(unsigned short val) {
		char buf[8];
		snprintf(buf, sizeof(buf), "%u", (unsigned int)val);
		return buf;
	}
	static string toString(int val) {
		char buf[16];
		snprintf(buf, sizeof(buf), "%d", val);
		return buf;
	}
	static string toString(unsigned int val) {
		char buf[16];
		snprintf(buf, sizeof(buf), "%u", val);
		return buf;
	}
	static string toString(long val) {
		char buf[32];
		snprintf(buf, sizeof(buf), "%ld", val);
		return buf;
	}
	static string toString(unsigned long val) {
		char buf[32];
		snprintf(buf, sizeof(buf), "%lu", val);
		return buf;
	}
	static string toString(long long val) {
		char buf[32];
		snprintf(buf, sizeof(buf), I64_FMT, val);
		return buf;
	}
	static string toString(unsigned long long val) {
		char buf[32];
		snprintf(buf, sizeof(buf), U64_FMT, val);
		return buf;
	}
	static string toString(double val) {
		char buf[16];
		snprintf(buf, sizeof(buf), "%0.2f", val);
		return buf;
	}

	static string toString(const StringList& lst) {
		if(lst.size() == 1)
			return lst[0];
		string tmp("[");
		for(StringList::const_iterator i = lst.begin(); i != lst.end(); ++i) {
			tmp += *i + ',';
		}
		if(tmp.length() == 1)
			tmp.push_back(']');
		else
			tmp[tmp.length()-1] = ']';
		return tmp;
	}

	static wstring toStringW( long val ) {
		wchar_t buf[32];
		snwprintf(buf, sizeof(buf), L"%ld", val);
		return buf;
	}

	static wstring toStringW( int64_t val ) {
		wchar_t buf[32];
		snwprintf(buf, sizeof(buf), _T(I64_FMT), val);
		return buf;
	}

	static wstring toStringW( DWORD val ) {
		wchar_t buf[16];
		snwprintf(buf, sizeof(buf), L"%d", val);
		return buf;
	}

	static wstring toStringW( int val ) {
		wchar_t buf[16];
		snwprintf(buf, sizeof(buf), L"%d", val);
		return buf;
	}

	static wstring toStringW( size_t val ) {
		wchar_t buf[16];
		snwprintf(buf, sizeof(buf), L"%d", val);
		return buf;
	}

	static wstring toStringW( double val ) {
		wchar_t buf[32];
		snwprintf(buf, sizeof(buf), L"%0.2f", val);
		return buf;
	}

	static string toHexEscape(char val) {
		char buf[sizeof(int)*2+1+1];
		snprintf(buf, sizeof(buf), "%%%X", val&0x0FF);
		return buf;
	}
	static char fromHexEscape(const string aString) {
		unsigned int res = 0;
		sscanf(aString.c_str(), "%X", &res);
		return static_cast<char>(res);
	}

	template<typename T>
	static T& intersect(T& t1, const T& t2) {
		for(typename T::iterator i = t1.begin(); i != t1.end();) {
			if(find_if(t2.begin(), t2.end(), bind1st(equal_to<typename T::value_type>(), *i)) == t2.end())
				i = t1.erase(i);
			else
				++i;
		}
		return t1;
	}

	static string encodeURI(const string& /*aString*/, bool reverse = false);
	static string getLocalIp();
	static bool isPrivateIp(string const& ip);
	/**
	 * Case insensitive substring search.
	 * @return First position found or string::npos
	 */
	static string::size_type findSubString(const string& aString, const string& aSubString, string::size_type start = 0) throw();
	static wstring::size_type findSubString(const wstring& aString, const wstring& aSubString, wstring::size_type start = 0) throw();

	/* Utf-8 versions of strnicmp and stricmp, unicode char code order (!) */
	static int stricmp(const char* a, const char* b);
	static int strnicmp(const char* a, const char* b, size_t n);

	static int stricmp(const wchar_t* a, const wchar_t* b) {
		while(*a && Text::toLower(*a) == Text::toLower(*b))
			++a, ++b;
		return ((int)Text::toLower(*a)) - ((int)Text::toLower(*b));
	}
	static int strnicmp(const wchar_t* a, const wchar_t* b, size_t n) {
		while(n && *a && Text::toLower(*a) == Text::toLower(*b))
			--n, ++a, ++b;

		return n == 0 ? 0 : ((int)Text::toLower(*a)) - ((int)Text::toLower(*b));
	}

	static int stricmp(const string& a, const string& b) { return stricmp(a.c_str(), b.c_str()); }
	static int strnicmp(const string& a, const string& b, size_t n) { return strnicmp(a.c_str(), b.c_str(), n); }
	static int stricmp(const wstring& a, const wstring& b) { return stricmp(a.c_str(), b.c_str()); }
	static int strnicmp(const wstring& a, const wstring& b, size_t n) { return strnicmp(a.c_str(), b.c_str(), n); }

	static string getIpCountry (const string& IP);

	static bool getAway() { return away; }
	static void setAway(bool aAway);
	static string getAwayMessage();
	static void setAwayMessage(const string& aMsg) { awayMsg = aMsg; }

	static uint64_t getDirSize(const string &sFullPath);
	static bool validatePath(const string &sPath);
	static bool fileExists(const string &aFile);

	static uint32_t rand();
	static uint32_t rand(uint32_t high) { return rand() % high; }
	static uint32_t rand(uint32_t low, uint32_t high) { return rand(high-low) + low; }
	static double randd() { return ((double)rand()) / ((double)0xffffffff); }

	static TCHAR* strstr(const TCHAR *str1, const TCHAR *str2, int *pnIdxFound);
	static int getNetLimiterLimit();

private:
	/** Per-user configuration */
	static string configPath;
	/** Global configuration */
	static string systemPath;
	/** Various resources (help files etc) */
	static string dataPath;

	static bool away;
	static string awayMsg;
	static time_t awayTime;
	
	typedef map<uint32_t, uint16_t> CountryList;
	typedef CountryList::iterator CountryIter;

	static CountryList countries;
	
	static long mUptimeSeconds;
};
	
/** Case insensitive hash function for strings */
struct noCaseStringHash {
	size_t operator()(const string* s) const {
		return operator()(*s);
	}

	size_t operator()(const string& s) const {
		size_t x = 0;
		const char* end = s.data() + s.size();
		for(const char* str = s.data(); str < end; ) {
			wchar_t c = 0;
			int n = Text::utf8ToWc(str, c);
			if(n < 0) {
				x = x*32 - x + '_';
				str += abs(n);
			} else {
				x = x*32 - x + (size_t)Text::toLower(c);
				str += n;
			}
		}
		return x;
	}

	size_t operator()(const wstring* s) const {
		return operator()(*s);
	}
	size_t operator()(const wstring& s) const {
		size_t x = 0;
		const wchar_t* y = s.data();
		wstring::size_type j = s.size();
		for(wstring::size_type i = 0; i < j; ++i) {
			x = x*31 + (size_t)Text::toLower(y[i]);
		}
		return x;
	}

	bool operator()(const string* a, const string* b) const {
		return Util::stricmp(*a, *b) < 0;
	}
	bool operator()(const string& a, const string& b) const {
		return Util::stricmp(a, b) < 0;
	}
	bool operator()(const wstring* a, const wstring* b) const {
		return Util::stricmp(*a, *b) < 0;
	}
	bool operator()(const wstring& a, const wstring& b) const {
		return Util::stricmp(a, b) < 0;
	}
};

/** Case insensitive string comparison */
struct noCaseStringEq {
	bool operator()(const string* a, const string* b) const {
		return a == b || Util::stricmp(*a, *b) == 0;
	}
	bool operator()(const string& a, const string& b) const {
		return Util::stricmp(a, b) == 0;
	}
	bool operator()(const wstring* a, const wstring* b) const {
		return a == b || Util::stricmp(*a, *b) == 0;
	}
	bool operator()(const wstring& a, const wstring& b) const {
		return Util::stricmp(a, b) == 0;
	}
};

} // namespace dcpp

#endif // !defined(UTIL_H)

/**
 * @file
 * $Id$
 */
