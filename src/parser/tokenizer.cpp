#include <fcntl.h>
#include <string>
#include <fstream>
#include <iostream>

void tokenizer(const std::string file) {
    // open file
     std::ifstream config_file(file.c_str());

    if (!config_file.is_open())
        throw std::runtime_error("Error: Could not open the file!");

    std::string line;
    while (std::getline(config_file, line)) {
        std::cout << line << '\n';
    }

    // remove comments

    // separate each rule

    // handle special chars '{};'
}