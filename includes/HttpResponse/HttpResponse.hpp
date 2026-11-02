#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>

class HttpResponse {
	public:
		HttpResponse();
		~HttpResponse();
		
		void setStatus( int );
		void setHeader( const std::string &, const std::string & );
		void setBody( const std::string & );

		bool isError();
		bool shouldClose() const;
		int getStatusCode() const;
		std::string getBody() const;		
		std::string serialize();

	private:
		int _statusCode;
		bool _closeConnection;
		std::map<std::string, std::string> _headers;
		std::string _body;
};

#endif