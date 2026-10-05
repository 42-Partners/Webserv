#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "ConfigData.hpp"


class Parser {
    public:
        static Config parse_tokens(std::vector<std::string> tokens);
    private:
        /* data */
};


#endif