
#include "config/ConfigData.hpp"

Config parse_tokens(std::vector<std::string> tokens) {
    if (tokens.empty()) {
        throw std::runtime_error("Syntax Error: Config file is empty!");
    }

    std::vector<std::string>::iterator it = tokens.begin();
    bool location = false;
    while (it != tokens.end()) {
        if (!location) {
            if (*it == "location") {
                location = true;
                it++;
                if (*it != "{")
                    throw std::runtime_error("Syntax Error: '{' expected.");
            } else {
                throw std::runtime_error("Syntax Error: location expected.");
            }
        } else {
            if (*it == "}") {
                location = false;
                it++;
                continue;
            }
            // location
        }
        it++;
    }
    return Config();
}
