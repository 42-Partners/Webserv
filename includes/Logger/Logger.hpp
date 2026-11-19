#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>

class Logger {

	public:
		static void info( const std::string &, const std::string & );
		static void error( const std::string &, const std::string & );

	private:
		Logger();
};

#endif