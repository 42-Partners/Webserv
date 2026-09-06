#include <fcntl.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

std::vector<std::string> tokenizer(const std::string& file) {
    std::ifstream config_file(file.c_str());
    if (!config_file.is_open())
        throw std::runtime_error("Error: Could not open the file!");

    std::string content;
    std::string line;
    
    while (std::getline(config_file, line)) {
        size_t hash_pos = line.find('#');
        if (hash_pos != std::string::npos) {
            line = line.substr(0, hash_pos);
        }
        content.append(line + " ");
    }
    
    config_file.close();

    std::string padded_content;

    for (size_t i = 0; i < content.length(); ++i) {
        char c = content[i];

        if (c == '{' || c == '}' || c == ';') {
            padded_content += " ";
            padded_content += c;
            padded_content += " ";
        } else {
            padded_content += c;
        }
    }

    std::vector<std::string> tokens;
    std::stringstream ss(padded_content);
    std::string token;
    
    while (ss >> token) {
        tokens.push_back(token);
    }

    return tokens;
}
