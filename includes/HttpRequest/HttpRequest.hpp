#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include "../include.hpp"

enum RequestState {
	PARSING_REQUEST_LINE,
	PARSING_HEADERS,
	PARSING_BODY,
	REQUEST_COMPLETE,
	REQUEST_ERROR
};

class HttpRequest {
private:
	std::string                         _method;
	std::string                         _uri;
	std::string                         _path;
	std::string                         _queryString;
	std::string                         _httpVersion;

	std::map<std::string, std::string>  _headers;

	std::string                         _body;

	RequestState                        _state;
	int                                 _errorCode;
	size_t                              _contentLength;
	bool                                _isChunked;

public:
	HttpRequest();
	~HttpRequest();

	void setMethod(const std::string& method);
	void setUri(const std::string& uri);
	void setPath(const std::string& path);
	void setQueryString(const std::string& query);
	void setHttpVersion(const std::string& version);
	void addHeader(const std::string& key, const std::string& value);
	void setBody(const std::string& body);
	void appendBody(const std::string& chunk);
	void setState(RequestState state);
	void setErrorCode(int code);
	void setContentLength(size_t length);
	void setIsChunked(bool isChunked);

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
	void clear();
};


//apagar: essa funcao fica responsavel por popular o objeto HttpRequest, não deve ir para a versão final de entrega do projeto.
void parseIncomingBuffer(HttpRequest& req, const std::string& rawBuffer);


#endif