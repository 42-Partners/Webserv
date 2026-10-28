#include "Handlers/UploadHandler.hpp"
#include "StatusCode/StatusCode.hpp"

void testUpload() {
    // 1. Mock de LocationConfig (ajuste os métodos conforme sua classe real)
    LocationConfig loc;
    loc.addAllowedMethod("POST");
    loc.setUploadPath("./uploads");

    // 2. Mock de HttpRequest com cabeçalhos e corpo multipart válidos
    HttpRequest req;
    req.setMethod("POST");
    req.addHeader("Content-Type", "multipart/form-data; boundary=------------------------boundary123");

    std::string boundary = "------------------------boundary123";
    std::string mockBody = 
        "--" + boundary + "\r\n" +
        "Content-Disposition: form-data; name=\"file\"; filename=\"teste_upload.txt\"\r\n" +
        "Content-Type: text/plain\r\n" +
        "\r\n" +
        "Conteudo de teste enviado via POST e salvo com sucesso!\r\n" +
        "--" + boundary + "--\r\n";
    
    req.setBody(mockBody);

    // 3. Testa o handlePost (validações)
    std::cout << "[Teste] Executando handlePost..." << std::endl;
    HttpResponse res = handlePost(req, loc);
    std::cout << "[Teste] Status code retornado: " << res.getStatusCode() << std::endl;
    std::cout << "[Teste] Corpo da resposta: " << res.getBody() << std::endl;

    // 4. Se passou pelas validações, testa o parseMultipart isoladamente
    if (res.getStatusCode() == STATUS_CREATED) {
        std::cout << "[Teste] Executando parseMultipart..." << std::endl;
        bool success = parseMultipart(req.getBody(), boundary, loc.getUploadPath());
        if (success) {
            std::cout << "[Teste] parseMultipart executado com sucesso! Verifique a pasta " << loc.getUploadPath() << std::endl;
        } else {
            std::cerr << "[Teste] Falha no parseMultipart." << std::endl;
        }
    }
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
