#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>

class HttpRequest {
	public:

		enum ParsingState {
			PARSING_REQUEST_LINE,
			PARSING_HEADERS,
			PARSING_BODY,
			REQUEST_COMPLETE,
			REQUEST_ERROR
		};

		typedef std::map<std::string, std::string>::const_iterator map_iterator;

		HttpRequest();
		~HttpRequest();
	
		void feed( const std::string & );

		int getErrorCode() const;
		ParsingState getState() const;
		const std::string& getMethod() const;
		const std::string& getUri() const;
		const std::string& getPath() const;
		const std::string& getQueryString() const;
		const std::string& getHttpVersion() const;
		const std::string& getBody() const;
		const std::map<std::string, std::string> & getHeaders() const;
		std::string getHeader( const std::string & ) const;

		bool hasHeader( const std::string & ) const;

	private:
		int _errorCode;
		ParsingState _state;
		bool _isChunked;
		size_t _contentLength;

		std::string _method;
		std::string _uri;
		std::string _path;
		std::string _queryString;
		std::string _httpVersion;
		std::string _body;

		std::map<std::string, std::string> _headers;
};

#endif