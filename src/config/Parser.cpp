#include "config/ConfigData.hpp"
#include "config/Parser.hpp"
#include <iostream>
#include <sstream>

static int string_to_int(const std::string &str) {
    std::stringstream ss(str);
    int res = 0;
    ss >> res;
    return res;
}

static size_t parse_body_size(const std::string &str) {
    if (str.empty()) return 0;
    
    char unit = str[str.length() - 1];
    std::string num_str = str;
    size_t multiplier = 1;

    if (unit == 'M' || unit == 'm') {
        multiplier = 1024 * 1024;
        num_str = str.substr(0, str.length() - 1);
    } else if (unit == 'K' || unit == 'k') {
        multiplier = 1024;
        num_str = str.substr(0, str.length() - 1);
    }

    std::stringstream ss(num_str);
    size_t bytes = 0;
    ss >> bytes;
    return bytes * multiplier;
}

void Parser::parse_server(ServerConfig &server, std::string name, std::vector<std::string> args) {
    if (args.empty()) {
        throw std::runtime_error("Syntax Error: No values provided for directive '" + name + "'.");
    }

    if (name == "listen") {
        if (args.size() != 1) throw std::runtime_error("Syntax Error: 'listen' expects exactly 1 argument.");
        
        size_t colon = args[0].find(':');
        if (colon != std::string::npos) {
            server.host = args[0].substr(0, colon);
            server.port = string_to_int(args[0].substr(colon + 1));
        } else {
            server.port = string_to_int(args[0]);
        }
    } 
    else if (name == "host") {
        if (args.size() != 1) throw std::runtime_error("Syntax Error: 'host' expects exactly 1 argument.");
        server.host = args[0];
    }
    else if (name == "server_name") {
        server.server_names = args;
    } 
    else if (name == "client_max_body_size") {
        if (args.size() != 1) throw std::runtime_error("Syntax Error: 'client_max_body_size' expects exactly 1 argument.");
        server.client_max_body_size = parse_body_size(args[0]);
    } 
    else if (name == "error_page") {
        if (args.size() < 2) throw std::runtime_error("Syntax Error: 'error_page' expects at least 2 arguments.");
        
        std::string page = args.back();
        for (size_t i = 0; i < args.size() - 1; ++i) {
            int code = string_to_int(args[i]);
            server.error_pages[code] = page;
        }
    } 
    else {
        throw std::runtime_error("Syntax Error: Unknown server directive '" + name + "'.");
    }
}


void Parser::parse_location(LocationConfig &loc, std::string name, std::vector<std::string> args) {
    if (args.empty()) {
        throw std::runtime_error("Syntax Error: No values provided for directive '" + name + "'.");
    }

    if (name == "root") {
        if (args.size() != 1) throw std::runtime_error("Syntax Error: 'root' expects exactly 1 argument.");
        loc.root = args[0];
    } 
    else if (name == "allow_methods") {
        loc.allowed_methods = args;
    } 
    else if (name == "upload_store" || name == "upload_path") {
        if (args.size() != 1) throw std::runtime_error("Syntax Error: '" + name + "' expects exactly 1 argument.");
        loc.upload_path = args[0];
    } 
    else if (name == "cgi_extension") {
        if (args.empty()) throw std::runtime_error("Syntax Error: 'cgi_extension' expects at least 1 argument.");
        loc.cgi_extension = args[0];
    } 
    else if (name == "index") {
        if (args.size() != 1) throw std::runtime_error("Syntax Error: 'index' expects 1 argument.");
        loc.index = args[0];
    } 
    else if (name == "autoindex") {
        if (args.size() != 1 || (args[0] != "on" && args[0] != "off")) {
            throw std::runtime_error("Syntax Error: 'autoindex' expects 'on' or 'off'.");
        }
        loc.autoindex = (args[0] == "on");
    }
    else if (name == "redirect") {
        if (args.size() != 2) throw std::runtime_error("Syntax Error: 'redirect' expects 2 arguments (code and url).");
        int code = string_to_int(args[0]);
        loc.redirect = std::make_pair(code, args[1]);
    }
    else {
        throw std::runtime_error("Syntax Error: Unknown directive '" + name + "'.");
    }
}

Config Parser::parse_tokens(std::vector<std::string> tokens) {
    if (tokens.empty()) {
        throw std::runtime_error("Syntax Error: Config file is empty!");
    }

    Config conf;
    std::vector<std::string>::iterator it = tokens.begin();

    while (it != tokens.end()) {
        if (*it == "server") {
            ServerConfig serverConf;
            it++;
            
            if (it == tokens.end() || *it != "{") {
                throw std::runtime_error("Syntax Error: '{' expected after 'server'.");
            }
            it++;

            while (it != tokens.end() && *it != "}") {
                if (*it == "location") {
                    LocationConfig locConf;
                    it++;
                    
                    if (it == tokens.end()) {
                        throw std::runtime_error("Syntax Error: Unexpected end of file after 'location'.");
                    }

                    if (*it != "{") {
                        locConf.path = *it;
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
                        
                        parse_location(locConf, directive, directive_args);
                        it++;
                    }

                    if (it == tokens.end()) {
                        throw std::runtime_error("Syntax Error: Missing '}' to close location block.");
                    }

                    serverConf.addLocation(locConf);
                    it++;
                }
                else {
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
                    
                    parse_server(serverConf, directive, directive_args);
                    it++;
                }
            }

            if (it == tokens.end()) {
                throw std::runtime_error("Syntax Error: Missing '}' to close server block.");
            }

            conf.addServer(serverConf);
        } else {
            throw std::runtime_error("Syntax Error: Unexpected token '" + *it + "' at global scope.");
        }
        
        it++; 
    }

    std::cout << conf << std::endl; // print config file teste
    return conf;
}
