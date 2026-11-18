#include "Handlers/ErrorHandler.hpp"
#include "Handlers/UploadHandler.hpp"
#include "StatusCode/StatusCode.hpp"
#include "Utils/Utils.hpp"

#include <cerrno>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>


static int parseMultipart(const std::string& body, const std::string& boundary, const std::string& uploadStore);
static std::string extractBoundary(const std::string& contentType);

HttpResponse handlePost(const HttpRequest &req, const ServerConfig & config, const LocationConfig &loc) {
    std::cout << "[INFO] handlePost: " << req.getPath() << std::endl; // debug test retira
    if (loc.getUploadPath().empty())
        return handleError(config, STATUS_INTERNAL_SERVER_ERROR);

    std::string contentType = req.getHeader("Content-Type");
    if (contentType.find("multipart/form-data") == std::string::npos) 
        return handleError(config, STATUS_BAD_REQUEST);

    int multipartStatus = parseMultipart(req.getBody(), extractBoundary(contentType), loc.getUploadPath());
    if (multipartStatus != STATUS_CREATED)
        return handleError(config, multipartStatus);

    HttpResponse response; //! aqui nao tem content-type?
    response.setStatus(STATUS_CREATED);
    response.setBody("File uploaded successfully."); 

    return response;
}

static int parseMultipart(const std::string& body, const std::string& boundary, const std::string& uploadStore) {
    if (boundary.empty()) return STATUS_BAD_REQUEST;

    if (mkdir(uploadStore.c_str(), 0755) != 0) {
        if (errno != EEXIST) {
            std::cerr << "Error: Couldn’t create upload directory. " << uploadStore << std::endl;
            return STATUS_INTERNAL_SERVER_ERROR;
        }
    }

    std::string delimiter = "--" + boundary;
    
    size_t fnPos = body.find("filename=\"");
    if (fnPos == std::string::npos) {
        std::cerr << "Error: No file found in multipart." << std::endl;
        return STATUS_BAD_REQUEST;
    }
    fnPos += 10;

    size_t fnEnd = body.find("\"", fnPos);
    if (fnEnd == std::string::npos) return STATUS_BAD_REQUEST;
    
    std::string filename = body.substr(fnPos, fnEnd - fnPos);
    if (filename.empty()) return STATUS_BAD_REQUEST;

    size_t dataStart = body.find("\r\n\r\n", fnEnd);
    if (dataStart == std::string::npos) return STATUS_BAD_REQUEST;
    dataStart += 4;

    size_t dataEnd = body.find("\r\n" + delimiter, dataStart);
    if (dataEnd == std::string::npos) return STATUS_BAD_REQUEST;

    std::string fileContent = body.substr(dataStart, dataEnd - dataStart);

    std::string fullPath = uploadStore + "/" + filename; //! pode ter path transversal aqui?
    std::ofstream outFile(fullPath.c_str(), std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Error opening multipart file for writing: " << fullPath << std::endl;
        return STATUS_INTERNAL_SERVER_ERROR;
    }

    outFile.write(fileContent.data(), fileContent.size());
    outFile.close();
    if (outFile.fail())
        return STATUS_INTERNAL_SERVER_ERROR;

    std::cout << "[Sucesso] Arquivo extraído e salvo: " << fullPath << std::endl;
    return STATUS_CREATED;
}

static std::string extractBoundary(const std::string& contentType) {
    std::vector<std::string> parts = split(contentType, ";");

    for (size_t i = 0; i < parts.size(); ++i) {
        std::string item = trim(parts[i]);
        if (item.find("boundary=") == 0) {
            std::string boundary = item.substr(9);
            if (boundary.size() >= 2 && boundary[0] == '"' && boundary[boundary.size() - 1] == '"') {
                boundary = boundary.substr(1, boundary.size() - 2); 
            }
            return trim(boundary);
        }
    }
    return "";
}
