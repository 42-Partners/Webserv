#include "config/LocationConfig.hpp"

LocationConfig::LocationConfig()
    : _path(""),
      _allowedMethods(),
      _root(""),
      _autoindex(false),
      _index(""),
      _cgiExtension(""),
      _uploadPath(""),
      _redirect(std::make_pair(0, "")) {
}

LocationConfig::LocationConfig(const LocationConfig& other)
    : _path(other._path),
      _allowedMethods(other._allowedMethods),
      _root(other._root),
      _autoindex(other._autoindex),
      _index(other._index),
      _cgiExtension(other._cgiExtension),
      _uploadPath(other._uploadPath),
      _redirect(other._redirect) {
}

LocationConfig& LocationConfig::operator=(const LocationConfig& other) {
    if (this != &other) {
        this->_path = other._path;
        this->_allowedMethods = other._allowedMethods;
        this->_root = other._root;
        this->_autoindex = other._autoindex;
        this->_index = other._index;
        this->_cgiExtension = other._cgiExtension;
        this->_uploadPath = other._uploadPath;
        this->_redirect = other._redirect;
    }
    return *this;
}

LocationConfig::~LocationConfig() {
}

// Getters
const std::string& LocationConfig::getPath() const { return this->_path; }
const std::vector<std::string>& LocationConfig::getAllowedMethods() const { return this->_allowedMethods; }
const std::string& LocationConfig::getRoot() const { return this->_root; }
bool LocationConfig::getAutoindex() const { return this->_autoindex; }
const std::string& LocationConfig::getIndex() const { return this->_index; }
const std::string& LocationConfig::getCgiExtension() const { return this->_cgiExtension; }
const std::string& LocationConfig::getUploadPath() const { return this->_uploadPath; }
const std::pair<int, std::string>& LocationConfig::getRedirect() const { return this->_redirect; }

// Setters
void LocationConfig::setPath(const std::string& path) { this->_path = path; }
void LocationConfig::setAllowedMethods(const std::vector<std::string>& methods) { this->_allowedMethods = methods; }
void LocationConfig::setRoot(const std::string& root) { this->_root = root; }
void LocationConfig::setAutoindex(bool autoindex) { this->_autoindex = autoindex; }
void LocationConfig::setIndex(const std::string& index) { this->_index = index; }
void LocationConfig::setCgiExtension(const std::string& ext) { this->_cgiExtension = ext; }
void LocationConfig::setUploadPath(const std::string& path) { this->_uploadPath = path; }
void LocationConfig::setRedirect(const std::pair<int, std::string>& redirect) { this->_redirect = redirect; }
void LocationConfig::setRedirect(int code, const std::string& url) { this->_redirect = std::make_pair(code, url); }

void LocationConfig::addAllowedMethod(const std::string& method) {
    this->_allowedMethods.push_back(method);
}