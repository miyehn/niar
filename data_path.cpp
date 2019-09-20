#include "data_path.hpp"

#include <iostream>
#include <vector>
#include <sstream>

#if defined(_WIN32)
#include <windows.h>
#include <Knownfolders.h>
#include <Shlobj.h>
#include <direct.h>
#include <io.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#include <sys/stat.h>
#endif //WINDOWS


//This function gets the path to the current executable in various os-specific ways:
static std::string get_exe_path() {
#if defined(_WIN32)
  //See: https://stackoverflow.com/questions/1023306/finding-current-executables-path-without-proc-self-exe
	TCHAR buffer[MAX_PATH];
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::string ret = buffer;
	ret = ret.substr(0, ret.rfind('\\'));
	return ret;
#elif defined(__linux__)
  //From: https://stackoverflow.com/questions/933850/how-do-i-find-the-location-of-the-executable-in-c
	std::vector< char > buffer(1000);
	while (1) {
		ssize_t got = readlink("/proc/self/exe", &buffer[0], buffer.size());
		if (got <= 0) {
			return "";
		} else if (got < (ssize_t)buffer.size()) {
			std::string ret = std::string(buffer.begin(), buffer.begin() + got);
			return ret.substr(0, ret.rfind('/'));
		}
		buffer.resize(buffer.size() + 4000);
	}
#elif defined(__APPLE__)
  //From: https://stackoverflow.com/questions/799679/programmatically-retrieving-the-absolute-path-of-an-os-x-command-line-app/1024933
	uint32_t bufsize = 0;
	std::vector< char > buffer;
	_NSGetExecutablePath(&buffer[0], &bufsize);
	buffer.resize(bufsize, '\0');
	bufsize = buffer.size();
	if (_NSGetExecutablePath(&buffer[0], &bufsize) != 0) {
		throw std::runtime_error("Call to _NSGetExecutablePath failed for mysterious reasons.");
	}
	std::string ret = std::string(&buffer[0]);
	return ret.substr(0, ret.rfind('/'));
#else
#error "No idea what the OS is."
#endif
}

std::string data_path(std::string const &suffix) {
  static std::string path = get_exe_path(); //cache result of get_exe_path()
  return path + "/" + suffix;
}

/* From Rktcr; to be used eventually!
static std::string make_user_dir(std::string const &app_name) {
	std::string ret = "";
	#if defined(_WIN32)
	PWSTR path = NULL;
	if (S_OK == SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &path)) {
		int needed = WideCharToMultiByte(CP_UTF8, 0, path, -1, NULL, 0, NULL, NULL);
		if (needed != 0) {
			std::unique_ptr< char[] > temp(new char[needed]);
			memset(temp.get(), 0, needed);
			if (WideCharToMultiByte(CP_UTF8, 0, path, -1, temp.get(), needed, NULL, NULL) != 0) {
				if (temp.get()[needed-1] != '\0') {
					temp.get()[needed-1] = '\0'; //"fix it"
					LOG_ERROR("!!!! Woah, missing '\\0' terminator in converted string: " << temp.get());
				} else {
					ret = temp.get();
				}
			}
		}
		CoTaskMemFree(path);
		path = NULL;
	} else {
		LOG_WARNING("Unable to locate FOLDERID_Documents.");
		ret = ".";
	}
	if (ret.empty() || ret[ret.size()-1] != '/') {
		ret += '/';
	}
	ret = ret + app_name;
	#elif defined(__APPLE__) || defined(__linux__)
	char *var = getenv("HOME");
	if (var == NULL) {
		std::cerr << "Environment variable 'HOME' is not set; using current directory as data directory" << std::endl;
		ret = ".";
	} else {
		ret = var;
	}
	if (ret.empty() || ret[ret.size()-1] != '/') {
		ret += '/';
	}
	ret = ret + "." + app_name;
	#else
	#error "Unsure of the what OS we're on; so can't generate home-directory code"
	#endif
	//Make sure directory exists... or at least try to!
	#ifdef WINDOWS
	_mkdir(ret.c_str());
	#elif defined(MINGW)
	mkdir(ret.c_str());
	#else
	mkdir(ret.c_str(), 0755);
	#endif
	return ret;
}
std::string user_path(std::string const &suffix) {
	static std::string path = make_user_dir( ... TODO ... );
	return path + '/' + suffix;
}
*/
