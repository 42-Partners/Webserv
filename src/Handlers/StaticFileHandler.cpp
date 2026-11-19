#include "Handlers/ErrorHandler.hpp"
#include "Handlers/RedirectHandler.hpp"
#include "Handlers/StaticFileHandler.hpp"
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"
#include "config/LocationConfig.hpp"
#include "StatusCode/StatusCode.hpp"
#include "Utils/Utils.hpp"

#include <cerrno>
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
    size_t slashPos = path.find_last_of('/');
    size_t dotPos = path.find_last_of(".");

    if (dotPos == std::string::npos ||
        (slashPos != std::string::npos && dotPos < slashPos))
        return "application/octet-stream";
    
    std::string ext = toLower(path.substr(dotPos + 1));
    if (ext == "html" || ext == "htm") return "text/html; charset=utf-8";
    if (ext == "css") return "text/css; charset=utf-8";
    if (ext == "js") return "application/javascript; charset=utf-8";
    if (ext == "txt") return "text/plain; charset=utf-8";
    if (ext == "json") return "application/json";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "ico") return "image/x-icon";
    if (ext == "svg") return "image/svg+xml";
    if (ext == "pdf") return "application/pdf";
    
    return "application/octet-stream";
}

static HttpResponse serveFile(const ServerConfig & config, const std::string & filePath) {
    std::ifstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open()) 
        return handleError(config, STATUS_INTERNAL_SERVER_ERROR);

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();

    if (file.bad())
        return handleError(config, STATUS_INTERNAL_SERVER_ERROR);

    HttpResponse response;
    response.setStatus(STATUS_OK);
    response.setHeader("Content-Type", getMimeType(filePath));
    
    response.setBody(content);

    return response;
}

HttpResponse handleGet(const HttpRequest & request, const ServerConfig & config, const LocationConfig & loc) {
    std::string requestPath = request.getPath();
    std::string filePath = resolveFilePath(loc, requestPath);

    struct stat fileInfo;
    if (stat(filePath.c_str(), &fileInfo) != 0)
        return handleError(config, STATUS_NOT_FOUND);

    if (S_ISDIR(fileInfo.st_mode)) {
        if (requestPath[requestPath.size() - 1] != '/') {
            std::string target = request.getUri().substr(0, request.getUri().find('?')) + "/";
            return makeRedirect(STATUS_MOVED_PERMANENTLY, target);
        }
    
        if (access(filePath.c_str(), R_OK | X_OK) != 0)
            return handleError(config, STATUS_FORBIDDEN);

        std::string dirPath = filePath;
        if (dirPath[dirPath.size() - 1] != '/')
            dirPath += "/";

        if (!loc.getIndex().empty()) {
            std::string indexPath = dirPath + loc.getIndex();

            struct stat indexInfo;
            if (stat(indexPath.c_str(), &indexInfo) == 0 &&
                S_ISREG(indexInfo.st_mode) &&
                access(indexPath.c_str(), R_OK) == 0)
                return serveFile(config, indexPath);
        }
        
        // if (loc.getAutoindex())
        //     return handleAutoindex(config, dirPath, requestPath);

        return handleError(config, STATUS_FORBIDDEN);
    }

    if (access(filePath.c_str(), R_OK) != 0 || !S_ISREG(fileInfo.st_mode))
        return handleError(config, STATUS_FORBIDDEN);
    
    return serveFile(config, filePath);
}

HttpResponse handleDelete(const HttpRequest & request, const ServerConfig & config, const LocationConfig & loc) {
    std::string requestPath = request.getPath();
    std::string filePath = resolveFilePath(loc, requestPath);

    struct stat fileInfo;
    if (stat(filePath.c_str(), &fileInfo) != 0)
        return handleError(config, STATUS_NOT_FOUND);

    if (S_ISDIR(fileInfo.st_mode))
        return handleError(config, STATUS_FORBIDDEN);

    if (unlink(filePath.c_str()) != 0) {
        if (errno == EACCES || errno == EPERM)
            return handleError(config, STATUS_FORBIDDEN);

        if (errno == ENOENT)
            return handleError(config, STATUS_NOT_FOUND);

        return handleError(config, STATUS_INTERNAL_SERVER_ERROR);
    }

    HttpResponse response;
    response.setStatus(STATUS_NO_CONTENT);

    return response;
}
