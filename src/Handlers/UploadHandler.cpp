#include "Handlers/UploadHandler.hpp"
#include "StatusCode/StatusCode.hpp"

#include <cassert> // teste
void testExtractBoundary() {
    std::cout << "[Teste] Executando testExtractBoundary..." << std::endl;

    // Cenário 1: Content-Type padrão enviado por navegadores com boundary e aspas
    {
        std::string ct = "multipart/form-data; boundary=\"----WebKitFormBoundary7MA4YWxkTrZu0gW\"";
        std::string boundary = extractBoundary(ct);
        assert(boundary == "----WebKitFormBoundary7MA4YWxkTrZu0gW");
        std::cout << "[PASS] Teste 1: Boundary com aspas extraído com sucesso." << std::endl;
    }

    // Cenário 2: Content-Type sem aspas no boundary
    {
        std::string ct = "multipart/form-data; boundary=------------------------boundary123";
        std::string boundary = extractBoundary(ct);
        assert(boundary == "------------------------boundary123");
        std::cout << "[PASS] Teste 2: Boundary sem aspas extraído com sucesso." << std::endl;
    }

    // Cenário 3: Boundary no meio ou com espaços extras
    {
        std::string ct = "text/html; charset=utf-8;   boundary=my-custom-boundary   ";
        std::string boundary = extractBoundary(ct);
        assert(boundary == "my-custom-boundary");
        std::cout << "[PASS] Teste 3: Boundary com espaçamentos extras tratado com sucesso." << std::endl;
    }

    // Cenário 4: Content-Type sem a tag boundary (deve retornar vazio)
    {
        std::string ct = "application/json";
        std::string boundary = extractBoundary(ct);
        assert(boundary.empty());
        std::cout << "[PASS] Teste 4: Content-Type sem boundary retornou vazio corretamente." << std::endl;
    }

    std::cout << "[Teste] Todos os testes de extractBoundary passaram!\n" << std::endl;
}

HttpResponse handlePost(const HttpRequest &req, const LocationConfig &loc) {
    HttpResponse response;
    if (!loc.isMethodAllowed("POST")) {
        response.setStatus(STATUS_METHOD_NOT_ALLOWED); // 405
        response.setBody("Error: POST method not allowed for this route.");
        return response;
    }

    if (loc.getUploadPath().empty()) {
        response.setStatus(STATUS_INTERNAL_SERVER_ERROR); // 500
        response.setBody("Error: upload_path is not configured.");
        return response;
    }

    std::string contentType = req.getHeader("Content-Type");
    if (contentType.find("multipart/form-data") == std::string::npos) {
        response.setStatus(STATUS_BAD_REQUEST); // 400
        response.setBody("Error: Expected multipart/form-data for upload.");
        return response;
    }

    response.setStatus(STATUS_CREATED); // 201
    response.setBody("Upload validations passed successfully!");
    return response;
}

bool parseMultipart(const std::string& body, const std::string& boundary, const std::string& uploadStore) {
    if (mkdir(uploadStore.c_str(), 0755) != 0) {
        if (errno != EEXIST) {
            std::cerr << "Error: Coudn't create upload directory." << uploadStore << std::endl;
            return false;
        }
    }
    std::string delimiter = "--" + boundary;
    
    size_t fnPos = body.find("filename=\"");
    if (fnPos == std::string::npos) {
        std::cerr << "Error: No file found in multipart." << std::endl;
        return false;
    }
    fnPos += 10;

    size_t fnEnd = body.find("\"", fnPos);
    if (fnEnd == std::string::npos) return false;
    
    std::string filename = body.substr(fnPos, fnEnd - fnPos);

    size_t dataStart = body.find("\r\n\r\n", fnEnd);
    if (dataStart == std::string::npos) return false;
    dataStart += 4;

    size_t dataEnd = body.find(delimiter, dataStart);
    if (dataEnd == std::string::npos) {
        dataEnd = body.length();
    } else {
        // Remove o \r\n extra que costuma vir antes do boundary de fechamento
        if (dataEnd >= 2 && body[dataEnd - 2] == '\r' && body[dataEnd - 1] == '\n') {
            dataEnd -= 2;
        }
    }

    std::string fileContent = body.substr(dataStart, dataEnd - dataStart);

    std::string fullPath = uploadStore + "/" + filename;
    std::ofstream outFile(fullPath.c_str(), std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Error opening multipart file for writing: " << fullPath << std::endl;
        return false;
    }

    outFile.write(fileContent.data(), fileContent.size());
    outFile.close();

    std::cout << "[Sucesso] Arquivo extraído e salvo: " << fullPath << std::endl;
    return true;
}

std::string extractBoundary(const std::string& contentType) {
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
