#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>

class HttpResponse {
	public:
		HttpResponse();
		HttpResponse( const HttpResponse& src ); // necessario?
		HttpResponse& operator=(  const HttpResponse& src  ); // necessario?
		~HttpResponse();

		void setStatus(int status);
		void setHeader(const std::string & key, const std::string & value);
		void setBody(const std::string & body);

		int getStatusCode() const;
		
		std::string serialize();

	private:
		int _statusCode;
		std::map<std::string, std::string> _headers;
		std::string _body;

};

#endif