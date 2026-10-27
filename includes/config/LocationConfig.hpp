#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <string>
#include <vector>
#include <utility> // Necessário para std::pair

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

public:
    LocationConfig();
    LocationConfig(const LocationConfig& other);
    LocationConfig& operator=(const LocationConfig& other);
    ~LocationConfig();

    // Getters
    const std::string&                  getPath() const;
    const std::vector<std::string>&     getAllowedMethods() const;
    const std::string&                  getRoot() const;
    bool                                getAutoindex() const;
    const std::string&                  getIndex() const;
    const std::string&                  getCgiExtension() const;
    const std::string&                  getUploadPath() const;
    const std::pair<int, std::string>&  getRedirect() const;

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

    // Utilitário
    void addAllowedMethod(const std::string& method);
};

#endif