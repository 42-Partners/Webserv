#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <string>
#include <vector>
#include <utility>

class LocationConfig {
private:
    std::string                 _path;
    std::vector<std::string>    _allowedMethods;
    std::string                 _root;
    bool                        _autoindex;
    std::string                 _index;
    std::string                 _cgiExtension;
    std::string                 _uploadPath;
    std::pair<int, std::string> _redirect;
    bool                        _hasMaxBodySize;
    size_t                      _maxBodySize;

public:
    LocationConfig();
    LocationConfig(const LocationConfig& other);
    LocationConfig& operator=(const LocationConfig& other);
    ~LocationConfig();

    // Getters
    const std::string&  getPath() const;
    const std::vector<std::string>& getAllowedMethods() const;
    std::string getAllowedMethodsString() const;
    const std::string& getRoot() const;
    bool getAutoindex() const;
    const std::string& getIndex() const;
    const std::string& getCgiExtension() const;
    const std::string& getUploadPath() const;
    const std::pair<int, std::string>& getRedirect() const;
    bool hasMaxBodySize() const;
    size_t getMaxBodySize() const;

    // Setters
    void setPath(const std::string& path);
    void setAllowedMethods(const std::vector<std::string>& methods);
    void setRoot(const std::string& root);
    void setAutoindex(bool autoindex);
    void setIndex(const std::string& index);
    void setCgiExtension(const std::string& ext);
    void setUploadPath(const std::string& path);
    void setRedirect(const std::pair<int, std::string>& redirect);
    void setRedirect(int code, const std::string& url);
    void setMaxBodySize(size_t size);

    // Utilitário
    void addAllowedMethod(const std::string& method);
    bool isMethodAllowed(const std::string& method) const;
    bool hasRedirect() const;
    std::string resolveFilePath( const std::string & requestPath ) const;
};

#endif