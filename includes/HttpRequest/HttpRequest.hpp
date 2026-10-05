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

		HttpRequest();
		~HttpRequest();
	
		void feed(const std::string & rawBuffer);
		bool hasHeader(const std::string & key) const;

		ParsingState getState() const;
		int getErrorCode() const;
		const std::string& getMethod() const;
		const std::string& getUri() const;
		const std::string& getPath() const;
		const std::string& getQueryString() const;
		const std::string& getHttpVersion() const;
		const std::string& getBody() const;
		std::string getHeader(const std::string & key) const;
		const std::map<std::string, std::string> & getHeaders() const;

	private:
		ParsingState _state;
		int _errorCode;
		bool _isChunked;
		size_t _contentLength;

		std::string _method;
		std::string _uri;
		std::string _path;
		std::string _queryString;
		std::string _httpVersion;
		std::string _body;

		std::map<std::string, std::string> _headers;

	const std::string& getMethod() const;
	const std::string& getUri() const;
	const std::string& getPath() const;
	const std::string& getQueryString() const;
	const std::string& getHttpVersion() const;
	std::string        getHeader(const std::string& key) const;
	bool               hasHeader(const std::string& key) const;
	const std::map<std::string, std::string>& getHeaders() const;
	const std::string& getBody() const;
	RequestState       getState() const;
	int                getErrorCode() const;
	size_t             getContentLength() const;
	bool               isChunked() const;
	//Limpa o DTO da requisição
	void clear();
};

#endif