#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <map>
#include <vector>
#include <set>
#include <iostream>
#include <stdexcept>


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

		void feed( std::string & );

		void setMethod(const std::string& method);
		void setUri(const std::string& uri);
		void setPath(const std::string& path);
		void setQueryString(const std::string& query);
		void setHttpVersion(const std::string& version);
		void addHeader(const std::string& key, const std::string& value);
		void setBody(const std::string& body);
		void appendBody(const std::string& chunk);
		void setState(ParsingState state);
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
		ParsingState       getState() const;
		int                getErrorCode() const;
		size_t             getContentLength() const;
		bool               isChunked() const;
		void clear();
		
	private:
		int                                 _errorCode;
		ParsingState                        _state;
		bool                                _isChunked;
		bool                                _hasContentLength;
		size_t                              _contentLength;

		std::string                         _method;
		std::string                         _uri;
		std::string                         _path;
		std::string                         _queryString;
		std::string                         _httpVersion;
		std::string                         _body;

		std::map<std::string, std::string>  _headers;

		bool parseRequestLine( std::string & );
		bool parseMethod( const std::string & );
		bool parseUri( const std::string & );
		bool parseVersion( const std::string & );

		bool parseHeaders( std::string & );
		bool parseHeaderLine( const std::string & );
		bool finishHeaders();

		bool parseBody( std::string & );
		bool parseBodyContentLength( std::string &  );
		bool parseBodyChunked( std::string & );

		bool setError( int );

		static const size_t MAX_REQUEST_LINE = 8192;
		static const size_t MAX_HEADERS_SIZE = 8192;
		static const size_t MAX_HEADER_COUNT = 100;
		static const size_t MAX_CHUNK_SIZE_LINE = 32;

		static const size_t DEFAULT_MAX_BODY = 1048576; //adicionar setter para _maxBodySize (vem do serverconfig)
};


//apagar: essa funcao fica responsavel por popular o objeto HttpRequest, não deve ir para a versão final de entrega do projeto.
void parseIncomingBuffer(HttpRequest& req, const std::string& rawBuffer);

#endif