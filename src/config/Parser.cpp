
#include "config/ConfigData.hpp"
#include "config/Parser.hpp"

void Parser::parse_config(Config &conf, std::string name, std::vector<std::string> args) {
    if (args.empty()) {
        throw std::runtime_error("Syntax Error: No values provided for directive '" + name + "'.");
    }

    if (name == "root") {
        if (args.size() != 1) throw std::runtime_error("Syntax Error: 'root' expects exactly 1 argument.");
        // set
    } 
    else if (name == "allow_methods") {
        // allow_methods pode ter mais de um valor
        // set
    } 
    else if (name == "upload_store") {
        if (args.size() != 1) throw std::runtime_error("Syntax Error: 'upload_store' expects exactly 1 argument.");
        // set
    } 
    else if (name == "cgi_extension") {
        // set
    } 
    else if (name == "index") {
        // index pode ter mais de um arquivo
        // set
    } 
    else if (name == "autoindex") {
        // set
    }
    else {
        throw std::runtime_error("Syntax Error: Unknown directive '" + name + "'.");
    }
    std::cout << "->" << name << " | args= ";
    {
        std::vector<std::string>::iterator it = args.begin();
        while (it != args.end()) {
            std::cout << " " << *it << ";";
            it++;
        }
    }
    std::cout << std::endl; // test

    (void)conf;
    return;
}

Config Parser::parse_tokens(std::vector<std::string> tokens) {
    if (tokens.empty()) {
        throw std::runtime_error("Syntax Error: Config file is empty!");
    }

    Config config; // Definir
    std::vector<std::string>::iterator it = tokens.begin();

    while (it != tokens.end()) {
        if (*it == "location") {
            it++;
            
            if (it == tokens.end()) {
                throw std::runtime_error("Syntax Error: Unexpected end of file after 'location'.");
            }

            std::string location_path = "";
            if (*it != "{") {
                location_path = *it;
                it++;
            }

            if (it == tokens.end() || *it != "{") {
                throw std::runtime_error("Syntax Error: '{' expected after location path.");
            }
            it++;

            while (it != tokens.end() && *it != "}") {
                std::string directive = *it; 
                it++;
                
                std::vector<std::string> directive_args;
                
                while (it != tokens.end() && *it != ";" && *it != "}") {
                    directive_args.push_back(*it);
                    it++;
                }
                
                if (it == tokens.end() || *it != ";") {
                    throw std::runtime_error("Syntax Error: ';' expected after directive '" + directive + "'.");
                }
                
                parse_config(config, directive, directive_args);
                
                it++;
            }

            if (it == tokens.end()) {
                throw std::runtime_error("Syntax Error: Missing '}' to close location block.");
            }

        } else {
            throw std::runtime_error("Syntax Error: Unexpected token '" + *it + "' at global scope.");
        }
        
        it++;
    }
    
    std::cout << "config file ok" << std::endl;
    return config;
}