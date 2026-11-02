#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>

class HttpResponse {
	public:
		HttpResponse();
		~HttpResponse();

		bool isError();
		
		void setStatus( int );
		void setHeader( const std::string &, const std::string & );
		void setBody( const std::string & );

		int getStatusCode() const;
		std::string getBody() const;
		
		std::string serialize();

	private:
		int _statusCode;

		std::map<std::string, std::string> _headers;
		
		std::string _body;
};

#endif