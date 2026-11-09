#include "config/LocationConfig.hpp"
#include "HttpResponse/ErrorPage.hpp"
#include "Handlers/ErrorHandler.hpp"
#include "Handlers/StaticFileHandler.hpp"
#include "Handlers/UploadHandler.hpp"
#include "RequestProcessor/RequestProcessor.hpp"
#include "StatusCode/StatusCode.hpp"

#include <cassert>

void process(Connection & connection, const ServerConfig & config) {
	(void)connection;
	(void)config;
}
