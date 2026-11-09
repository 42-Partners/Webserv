#include "ServerManager/ServerManager.hpp"
#include "config/ServerConfig.hpp"
#include "config/LocationConfig.hpp"
#include <iostream>
#include <vector>

std::vector<ServerConfig> createMockConfigs() {
    std::vector<ServerConfig> configs;

    LocationConfig defaultLocation;
    defaultLocation.setPath("/");
    defaultLocation.setRoot("./www");
    defaultLocation.setIndex("index.html");
    defaultLocation.addAllowedMethod("GET");
    defaultLocation.addAllowedMethod("POST");

    // --- Servidor 1 (Porta 8080) ---
    ServerConfig config1;
    config1.setHost("127.0.0.1");
    config1.setPort(8080);
    config1.addServerName("localhost");
    config1.setClientMaxBodySize(10485760);
    config1.addErrorPage(404, "/404.html");
    config1.addLocation(defaultLocation);

    // --- Servidor 2 (Porta 8081 - Teste Multi-porta) ---
    ServerConfig config2;
    config2.setHost("127.0.0.1");
    config2.setPort(8081);
    config2.addServerName("test.local");
    config2.setClientMaxBodySize(2097152); // 2MB em bytes
    config2.addLocation(defaultLocation);

    // --- Servidor 3 (Porta 8080 - Virtual Host) ---
    LocationConfig virtualLocation;
    virtualLocation.setPath("/");
    virtualLocation.setRoot("./www/virtual");
    virtualLocation.setIndex("index.html");

    ServerConfig config3;
    config3.setHost("127.0.0.1");
    config3.setPort(8080);
    config3.addServerName("virtual.local");
    config3.addLocation(virtualLocation);

    // Adiciona os servidores ao vetor de retorno
    configs.push_back(config1);
    configs.push_back(config2);
    configs.push_back(config3);

    return configs;
}

int main() {
    try {
        std::cout << "[INFO] Iniciando o ServerManager com mock de configuracoes..." << std::endl;

        std::vector<ServerConfig> configs = createMockConfigs();
        ServerManager manager(configs);

        manager.run();

    } catch (const std::exception& e) {
        std::cerr << "[ERRO] Excecao capturada no main: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}