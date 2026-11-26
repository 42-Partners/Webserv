#include "Handlers/AutoIndexHandler.hpp"
#include "Handlers/ErrorHandler.hpp"
#include "Logger/Logger.hpp"
#include "StatusCode/StatusCode.hpp"
#include "Utils/Utils.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <sstream>
#include <sys/stat.h>
#include <vector>

static std::string makeAutoIndex( const std::string & requestPath, const std::vector<fileStruct> & files );
static bool compareFiles( const fileStruct & a, const fileStruct & b );

HttpResponse handleAutoindex(const ServerConfig & config, const std::string & dirPath, const std::string & requestPath) {
	DIR * dir = opendir( dirPath.c_str() );
	if ( !dir ) {
		int err = errno;
		Logger::error("AutoIndex", "opendir falhou em " + dirPath + ": " + strerror(err));
		return handleError(config, err == EACCES ? STATUS_FORBIDDEN
								: (err == ENOENT || err == ENOTDIR ) ? STATUS_NOT_FOUND
								: STATUS_INTERNAL_SERVER_ERROR );	
	}
		
	std::vector<fileStruct> files;

	struct dirent * dirEntry;
	while ( (dirEntry = readdir(dir)) != NULL ) {
		std::string name = dirEntry->d_name;
		if ( name == "." ) continue;
		if ( name == ".." && requestPath == "/" ) continue;

		fileStruct file;
		file.name = name;

		std::string entryPath = dirPath + file.name;

		struct stat fileStat;
		if (stat( entryPath.c_str(), &fileStat ) != 0) continue;

		if ( S_ISDIR(fileStat.st_mode) ) file.isDir = true;

		files.push_back(file);
	}

	closedir( dir );

	std::sort(files.begin(), files.end(), compareFiles);
	Logger::info("AutoIndex", "listando " + requestPath);

	HttpResponse response;
	response.setStatus( STATUS_OK );
	response.setBody( makeAutoIndex(requestPath, files) );
	response.setHeader( "Content-Type", "text/html");

	Logger::info("AutoIndex", "Autoindex generated for " + dirPath);

	return response;
}

static std::string percentEncoding( const std::string & text ) {
	std::string result;

	for ( size_t i = 0; i < text.size(); ++i ) {
		if ( (text[i] >= '0' && text[i] <= '9')
			|| (text[i] >= 'a' && text[i] <= 'z') 
			|| (text[i] >= 'A' && text[i] <= 'Z') 
			|| text[i] == '-' || text[i] == '_' 
			|| text[i] == '.' || text[i] == '~' )
			result += text[i];
		else {
			unsigned char c = text[i];
			result += '%';
			result += intToHex( c >> 4 );
			result += intToHex( c & 0x0F );
		}
	}

	return result;
}

static std::string escapeHtml( const std::string & text ) {
	std::string result;

	for ( size_t i = 0; i < text.size(); ++i ) {
		if ( text[i] == '&') result += "&amp;";
		else if ( text[i] == '<') result += "&lt;";
		else if ( text[i] == '>') result += "&gt;";
		else if ( text[i] == '"') result += "&quot;";
		else result += text[i];
	}

	return result;
}

static std::string makeAutoIndex( const std::string & requestPath, const std::vector<fileStruct> & files ) {
	std::string safePath = escapeHtml(requestPath);
	std::string html = "<html><head><title>Index of " + safePath + "</title></head><body><h1>Index of " + safePath + "</h1><hr><pre>";

	for ( size_t i = 0; i < files.size(); ++i ) {
		std::string displayName = files[i].name;
		std::string link = percentEncoding( displayName );

		if ( files[i].isDir) {
			link += "/";
			displayName += "/";
		}

		html += "<a href=\"" + link + "\">" + escapeHtml(displayName) + "</a>\n";
	}

	html += "</pre><hr></body></html>";

	return html;
}

static bool compareFiles( const fileStruct & a, const fileStruct & b ) {
	if ( a.name == ".." )
		return b.name != "..";

	if ( b.name == ".." )
		return false;

	if ( a.isDir && !b.isDir )
		return true;
	
	if ( !a.isDir && b.isDir )
		return false;
	
	return a.name < b.name;
}
