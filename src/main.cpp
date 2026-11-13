#include "ServerManager/ServerManager.hpp"
#include "config/ServerConfig.hpp"
#include "config/Parser.hpp"
#include <iostream>
#include <vector>

int main(int argc, char** argv, char **envp) {
    (void)envp;
    try {
        std::cout << "[INFO] Lendo config file..." << std::endl;
        std::vector<ServerConfig> configs = arg_parser(argc, argv);
        
        std::cout << "[INFO] Iniciando o ServerManager com config file..." << std::endl;
        ServerManager manager(configs);

        manager.run();

    } catch (const std::exception& e) {
        std::cerr << "[ERRO] Excecao capturada no main: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}