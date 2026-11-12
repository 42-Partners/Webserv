#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>

class HttpResponse {
	public:
		HttpResponse();
		~HttpResponse();

		bool isError() const;
		bool getKeepAlive() const;

		int getStatusCode() const;
		const std::string & getBody() const;
		
		void setStatus( int );
		void setKeepAlive( bool );
		void setBody( const std::string & );
		void setHeader( const std::string &, const std::string & );

		bool hasHeader( const std::string & ) const;

		std::string serialize() const;

	private:
		int _statusCode;
		bool _keepAlive;
		std::map<std::string, std::string> _headers;
		std::string _body;
};

#endif