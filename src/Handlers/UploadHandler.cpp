#include "Handlers/UploadHandler.hpp"
#include "StatusCode/StatusCode.hpp"

void testUpload() {
    std::cout << "[Teste] Executando teste do UploadHandler..." << std::endl;

    // 1. Cria o objeto da requisição
    HttpRequest req;

    // 2. Monta o buffer HTTP bruto simulando o que viria do socket do cliente
    std::string boundary = "------------------------boundary123";
    std::string rawRequest = 
        "POST /upload HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n"
        "Content-Length: 150\r\n"
        "\r\n"
        "--" + boundary + "\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"teste_upload.txt\"\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Conteudo do arquivo de teste!\r\n"
        "--" + boundary + "--\r\n";

    // 3. Alimenta o HttpRequest com o buffer bruto (assim como o servidor faz)
    req.feed(rawRequest);

    // 4. Cria uma Mock Location para testar
    LocationConfig loc;
    loc.addAllowedMethod("POST");
    loc.setUploadPath("./uploads");

    // 5. Testa o handlePost
    HttpResponse res = handlePost(req, loc);
    std::cout << "[Teste] Status code retornado: " << res.getStatusCode() << std::endl;
    std::cout << "[Teste] Corpo da resposta: " << res.getBody() << std::endl;

    // 6. Testa o parseMultipart diretamente se a requisição estiver completa
    if (req.getState() == HttpRequest::REQUEST_COMPLETE) {
        std::cout << "[Teste] Executando parseMultipart..." << std::endl;
        parseMultipart(req.getBody(), boundary, loc.getUploadPath());
    } else {
        std::cout << "[Teste] AVISO: A requisição não atingiu o estado REQUEST_COMPLETE." << std::endl;
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
