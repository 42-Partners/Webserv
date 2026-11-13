#include "config/ServerConfig.hpp"
#include "config/Parser.hpp"
#include <iostream>
#include <sstream>


/*parse args*/

static void file_handling( const char *file ) {
	if ( access(file, F_OK) || access(file, R_OK) )
		throw std::runtime_error("access(): File does not exist or is inaccessible."); 

	struct stat fileInfo;

	if (stat(file, &fileInfo) == 0) {
		if (!S_ISREG(fileInfo.st_mode))
			throw std::runtime_error("stat(): the file is not a regular file");
	}
	else
		throw std::runtime_error("stat(): Failed.");
}

std::vector<ServerConfig> arg_parser( int ac, char** av ) {
	std::string file = "config/default.conf";
	switch ( ac ) {
		case 1:
            std::cout << "[INFO] Usando config file padrão: " << file << std::endl;
			break;
		case 2:
            std::cout << "[INFO] Usando config file: \"" << av[1] << "\"" << std::endl;
			file = av[1];
			break;
		default:
			throw std::runtime_error("Usage: ./build/webserv\nor: ./build/webserv <config>"); 
	}
	file_handling(file.c_str());
	return parse_tokens(Tokenizer::tokenize(file));
}

/* parse tokens */

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

void parse_server(ServerConfig &server, std::string name, std::vector<std::string> args) { // to fix
    if (args.empty()) {
        throw std::runtime_error("Syntax Error: No values provided for directive '" + name + "'.");
    }

    if (name == "listen") {
        if (args.size() != 1) throw std::runtime_error("Syntax Error: 'listen' expects exactly 1 argument.");
        
        size_t colon = args[0].find(':');
        if (colon != std::string::npos) {
            server.setHost(args[0].substr(0, colon));
            server.setPort(string_to_int(args[0].substr(colon + 1)));
        } else {
            server.setPort(string_to_int(args[0]));
        }
    } 
    else if (name == "host") {
        if (args.size() != 1) throw std::runtime_error("Syntax Error: 'host' expects exactly 1 argument.");
        server.setHost(args[0]);
    }
    else if (name == "server_name") {
        server.setServerNames(args);
    } 
    else if (name == "client_max_body_size") {
        if (args.size() != 1) throw std::runtime_error("Syntax Error: 'client_max_body_size' expects exactly 1 argument.");
        server.setClientMaxBodySize(parse_body_size(args[0]));
    } 
    else if (name == "error_page") {
        if (args.size() < 2) throw std::runtime_error("Syntax Error: 'error_page' expects at least 2 arguments.");
        
        std::string page = args.back();
        for (size_t i = 0; i < args.size() - 1; ++i) {
            server.addErrorPage(string_to_int(args[i]), page);
        }
    } 
    else {
        throw std::runtime_error("Syntax Error: Unknown server directive '" + name + "'.");
    }
}


void parse_location(LocationConfig &loc, std::string name, std::vector<std::string> args) { // parsear location args
    if (args.empty()) {
        throw std::runtime_error("Syntax Error: No values provided for directive '" + name + "'.");
    }

    if (name == "root") {
        if (args.size() != 1) throw std::runtime_error("Syntax Error: 'root' expects exactly 1 argument.");
        loc.setRoot(args[0]);
    } 
    else if (name == "allow_methods") {
        loc.setAllowedMethods(args);
    } 
    else if (name == "upload_store" || name == "upload_path") {
        if (args.size() != 1) throw std::runtime_error("Syntax Error: '" + name + "' expects exactly 1 argument.");
        loc.setUploadPath(args[0]);
    } 
    else if (name == "cgi_extension") {
        if (args.empty()) throw std::runtime_error("Syntax Error: 'cgi_extension' expects at least 1 argument.");
        loc.setCgiExtension(args[0]);
    } 
    else if (name == "index") {
        if (args.size() != 1) throw std::runtime_error("Syntax Error: 'index' expects 1 argument.");
        loc.setIndex(args[0]);
    } 
    else if (name == "autoindex") {
        if (args.size() != 1 || (args[0] != "on" && args[0] != "off")) {
            throw std::runtime_error("Syntax Error: 'autoindex' expects 'on' or 'off'.");
        }
        loc.setAutoindex(args[0] == "on");
    }
    else if (name == "redirect") {
        if (args.size() != 2) throw std::runtime_error("Syntax Error: 'redirect' expects 2 arguments (code and url).");
        int code = string_to_int(args[0]);
        loc.setRedirect(code, args[1]);
    }
    else {
        throw std::runtime_error("Syntax Error: Unknown directive '" + name + "'.");
    }
}

std::vector<ServerConfig> parse_tokens(std::vector<std::string> tokens) {
    if (tokens.empty()) {
        throw std::runtime_error("Syntax Error: Config file is empty!");
    }
    std::vector<ServerConfig> servers;
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
                        locConf.setPath(*it);
                        it++;
                    }

                    if (it == tokens.end() || *it != "{" || (*it == "{" && locConf.getPath().empty())) {
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
                        
                        parse_location(locConf, directive, directive_args); // to fix
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

            servers.push_back(serverConf);
        } else {
            throw std::runtime_error("Syntax Error: Unexpected token '" + *it + "' at global scope.");
        }
        
        it++; 
    }

   
    // { // print config file teste
    //     for (size_t i = 0; i < servers.size(); ++i) {
    //         std::cout << "\n========== SERVER #" << i + 1 << " ==========\n";

    //         std::cout << "Host: " << servers[i].getHost() << std::endl;
    //         std::cout << "Port: " << servers[i].getPort() << std::endl;

    //         std::cout << "Server Names:" << std::endl;
    //         const std::vector<std::string>& names = servers[i].getServerNames();
    //         for (size_t j = 0; j < names.size(); ++j)
    //             std::cout << "  - " << names[j] << std::endl;

    //         std::cout << "Client Max Body Size: "
    //                 << servers[i].getClientMaxBodySize() << std::endl;

    //         std::cout << "Error Pages:" << std::endl;
    //         const std::map<int, std::string>& errors = servers[i].getErrorPages();
    //         for (std::map<int, std::string>::const_iterator it = errors.begin();
    //             it != errors.end(); ++it)
    //             std::cout << "  " << it->first << " -> " << it->second << std::endl;

    //         std::cout << "Locations: " << servers[i].getLocations().size()
    //                 << std::endl;
    //     }
    // }
    return servers;
}
