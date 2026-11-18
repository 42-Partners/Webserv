#include "Handlers/StaticFileHandler.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"
#include "config/LocationConfig.hpp"
#include "StatusCode/StatusCode.hpp"
#include "Utils/Utils.hpp"

#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

static std::string resolveFilePath( const LocationConfig & loc, const std::string & requestPath ) {
	const std::string leftover = requestPath.substr( loc.getPath().size() );
	const std::string & root = loc.getRoot();

	if ( leftover.empty() )
		return root;

	bool rootEndsWithSlash = root[root.size() - 1] == '/';
	bool leftoverStartsWithSlash = leftover[0] == '/';

	std::string filePath;
	if ( !rootEndsWithSlash && !leftoverStartsWithSlash )
		filePath = root + "/" + leftover;
	else if ( rootEndsWithSlash && leftoverStartsWithSlash )
		filePath = root + leftover.substr( 1 );
	else 
		filePath = root + leftover;

	return filePath;
}

std::string getMimeType(const std::string& path) {
    size_t dotPos = path.find_last_of(".");
    if (dotPos == std::string::npos) return "application/octet-stream";
    
    std::string ext = toLower(path.substr(dotPos + 1));
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "txt") return "text/plain";
    if (ext == "ico") return "image/x-icon";
    
    return "application/octet-stream";
}

HttpResponse handleGet(const HttpRequest & request, const ServerConfig & config, const LocationConfig & loc) {
    HttpResponse response;
    (void)config;
    if (!loc.isMethodAllowed("GET"))
        return handleError(config, STATUS_METHOD_NOT_ALLOWED); // 405

    std::string requestPath = request.getPath();
    std::string filePath = resolveFilePath(loc, requestPath);

    struct stat fileInfo;
    if (stat(filePath.c_str(), &fileInfo) != 0)
        return handleError(config, STATUS_NOT_FOUND); // 404

    if (S_ISDIR(fileInfo.st_mode) && !loc.getIndex().empty()) {
        if (filePath[filePath.size() - 1] != '/')
            filePath += '/';
        filePath += loc.getIndex();
        if (stat(filePath.c_str(), &fileInfo) != 0)
            return handleError(config, STATUS_NOT_FOUND); // 404
    }

    if (access(filePath.c_str(), R_OK) != 0)
        return handleError(config, STATUS_FORBIDDEN); // 403

    std::ifstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open()) 
        return handleError(config, STATUS_INTERNAL_SERVER_ERROR); // 500

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();

    response.setStatus(STATUS_OK); // 200
    response.setHeader("Content-Type", getMimeType(filePath));
    
    std::ostringstream lenStr;
    lenStr << content.size();
    response.setHeader("Content-Length", lenStr.str());
    
    response.setBody(content);
    return response;
}

HttpResponse handleDelete(const HttpRequest & request, const ServerConfig & config, const LocationConfig & loc) {
    HttpResponse response;
    (void)config;
    if (!loc.isMethodAllowed("DELETE"))
        return handleError(config, STATUS_METHOD_NOT_ALLOWED); // 405

    std::string requestPath = request.getPath();
    std::string filePath = resolveFilePath(loc, requestPath);

    struct stat fileInfo;
    if (stat(filePath.c_str(), &fileInfo) != 0)
        return handleError(config, STATUS_NOT_FOUND); // 404

    if (access(filePath.c_str(), W_OK) != 0)
        return handleError(config, STATUS_FORBIDDEN); // 403

    if (unlink(filePath.c_str()) != 0)
        return handleError(config, STATUS_INTERNAL_SERVER_ERROR); // 500

    response.setStatus(STATUS_NO_CONTENT); // 204
    response.setBody("File deleted successfully.");
    return response;
}
