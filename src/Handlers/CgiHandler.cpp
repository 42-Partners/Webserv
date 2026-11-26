#include "config/LocationConfig.hpp"
#include "Handlers/CgiHandler.hpp"
#include "Handlers/ErrorHandler.hpp"
#include "Logger/Logger.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

static HttpResponse executeCgi(const HttpRequest& req, const std::string& scriptPath, int serverPort, const std::string& serverName);

bool isCgiRequest(const std::string& uriPath, const LocationConfig& location) {
	std::string cleanPath = uriPath;
	size_t queryPos = cleanPath.find('?');
	if (queryPos != std::string::npos) {
		cleanPath = cleanPath.substr(0, queryPos);
	}

	const std::string& cgiExt = location.getCgiExtension();
	if (cgiExt.empty()) {
		return false;
	}

	size_t pos = cleanPath.find(cgiExt);
	while (pos != std::string::npos) {
		size_t endPos = pos + cgiExt.size();
		if (endPos == cleanPath.size() || cleanPath[endPos] == '/') {
			return true;
		}
		pos = cleanPath.find(cgiExt, pos + 1);
	}

	return false;
}

HttpResponse handleCgi(const HttpRequest & request, const ServerConfig & config, const LocationConfig & loc) {
	std::string scriptPath = loc.resolveFilePath(request.getPath());
	if (scriptPath.empty())
		return handleError(config, STATUS_INTERNAL_SERVER_ERROR);

	struct stat buffer;
	if (stat(scriptPath.c_str(), &buffer) != 0 || !S_ISREG(buffer.st_mode))
		return handleError(config, STATUS_NOT_FOUND);
	if (access(scriptPath.c_str(), X_OK) != 0)
		return handleError(config, STATUS_FORBIDDEN);
	Logger::info("CGI Handler", "Executing CGI script: " + scriptPath);
	HttpResponse res = executeCgi( 
		request,
		scriptPath,
		config.getPort(),
		config.getServerNames().empty() ? "localhost" : config.getServerNames()[0]);
	if (res.getStatusCode() >= 400)
		return handleError(config, res.getStatusCode());
	return res;
}

static void setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static char* createEnvString(const std::string& key, const std::string& value) {
	std::string full = key + "=" + value;
	return strdup(full.c_str());
}

static void freeEnvArray(std::vector<char*>& env) {
	for (size_t i = 0; i < env.size(); ++i) {
		if (env[i]) free(env[i]);
	}
}

static std::vector<char*> buildCgiEnvironment(const HttpRequest& req, int serverPort, const std::string& serverName) {
	std::vector<char*> env;

	env.push_back(createEnvString("REQUEST_METHOD", req.getMethod()));
	env.push_back(createEnvString("QUERY_STRING", req.getQueryString()));
	env.push_back(createEnvString("CONTENT_TYPE", req.getHeader("Content-Type")));
	env.push_back(createEnvString("CONTENT_LENGTH", req.getHeader("Content-Length")));
	env.push_back(createEnvString("SCRIPT_NAME", req.getPath()));
	env.push_back(createEnvString("PATH_INFO", ""));
	env.push_back(createEnvString("SERVER_PROTOCOL", "HTTP/1.1"));
	env.push_back(createEnvString("SERVER_NAME", serverName));

	std::ostringstream portStream;
	portStream << serverPort;
	env.push_back(createEnvString("SERVER_PORT", portStream.str()));
	
	env.push_back(createEnvString("GATEWAY_INTERFACE", "CGI/1.1"));
	env.push_back(createEnvString("REDIRECT_STATUS", "200"));

	env.push_back(NULL);
	return env;
}

static HttpResponse executeCgi(const HttpRequest& req, const std::string& scriptPath, int serverPort, const std::string& serverName) {
	HttpResponse response;
	CgiSession session;

	session.requestBody = req.getBody();
	session.bytesWritten = 0;
	session.isFinished = false;

	if (pipe(session.pipe_in) < 0 || pipe(session.pipe_out) < 0) {
		Logger::error("CGI Handler", "Failed to create CGI pipes");
		response.setStatus(500);
		response.setBody("Error: Internal Server Error (Pipe failed)"); //! errorhandler?
		return response;
	}

	std::vector<char*> envp = buildCgiEnvironment(req, serverPort, serverName);

	session.pid = fork();
	if (session.pid < 0) {
		Logger::error("CGI Handler", "Failed to fork CGI process");
		freeEnvArray(envp);
		close(session.pipe_in[0]); close(session.pipe_in[1]);
		close(session.pipe_out[0]); close(session.pipe_out[1]);
		response.setStatus(500);
		response.setBody("Error: Internal Server Error (Fork failed)"); //!errorhandler
		return response;
	}

	if (session.pid == 0) {
		dup2(session.pipe_in[0], STDIN_FILENO);
		dup2(session.pipe_out[1], STDOUT_FILENO);

		close(session.pipe_in[0]);
		close(session.pipe_in[1]);
		close(session.pipe_out[0]);
		close(session.pipe_out[1]);

		char* const args[] = { (char*)scriptPath.c_str(), NULL };
		execve(scriptPath.c_str(), args, envp.data());
		
		freeEnvArray(envp);
		exit(1);
	}

	freeEnvArray(envp);

	close(session.pipe_in[0]);
	close(session.pipe_out[1]);

	setNonBlocking(session.pipe_in[1]);
	setNonBlocking(session.pipe_out[0]);

	time_t startTime = time(NULL);
	int status = 0;
	bool timedOut = false;

	while (!session.isFinished) {
		if (time(NULL) - startTime > 5) {
			kill(session.pid, SIGKILL);
			timedOut = true;
			break;
		}

		pid_t result = waitpid(session.pid, &status, WNOHANG);
		if (result > 0) {
			session.isFinished = true;
			break;
		}

		struct pollfd pfd[2];
		pfd[0].fd = session.pipe_in[1];
		pfd[0].events = (session.bytesWritten < session.requestBody.size()) ? POLLOUT : 0;

		pfd[1].fd = session.pipe_out[0];
		pfd[1].events = POLLIN;

		int ret = poll(pfd, 2, 100);
		if (ret < 0) {
			Logger::error("CGI Handler", "CGI poll failed: " + std::string(strerror(errno)));
			break;
		}
		if (ret == 0) continue;

		if (pfd[0].revents & POLLOUT) {
			size_t remaining = session.requestBody.size() - session.bytesWritten;
			ssize_t written = write(session.pipe_in[1], session.requestBody.data() + session.bytesWritten, remaining);
			if (written > 0) {
				session.bytesWritten += written;
				if (session.bytesWritten >= session.requestBody.size()) {
					close(session.pipe_in[1]);
					session.pipe_in[1] = -1;
				}
			}
		}

		if (pfd[1].revents & POLLIN) {
			char buffer[4096];
			ssize_t bytesRead = read(session.pipe_out[0], buffer, sizeof(buffer));
			if (bytesRead > 0) {
				session.cgiOutput.append(buffer, bytesRead);
			} else if (bytesRead == 0) {
				session.isFinished = true;
			}
		}
	}

	if (session.pipe_in[1] != -1) close(session.pipe_in[1]);
	if (session.pipe_out[0] != -1) close(session.pipe_out[0]);
	
	waitpid(session.pid, &status, 0);

	int statusCode = STATUS_OK; // 200

	if (timedOut) {
		Logger::error("CGI Handler", "CGI script execution timed out");

		response.setStatus(STATUS_GATEWAY_TIMEOUT); // 504
		response.setBody("Error: 504 Gateway Timeout (CGI script took too long to execute)"); //!errorhandler?
		return response;
	}

	if (WIFSIGNALED(status)) {
		Logger::error("CGI Handler", "CGI process terminated by signal");

		response.setStatus(STATUS_INTERNAL_SERVER_ERROR); // 500
		response.setBody("Error: 500 Internal Server Error (CGI crashed by signal)"); //!errorhandler?
		return response;
	}

	if (WIFEXITED(status)) {
		int exitCode = WEXITSTATUS(status);
		if (exitCode != 0) {
			std::ostringstream ss;
			ss << exitCode;
			Logger::error("CGI Handler", "CGI process exited with status " + ss.str());

			response.setStatus(STATUS_INTERNAL_SERVER_ERROR); // 500
			response.setBody("Error: 500 Internal Server Error (CGI exited with non-zero status)"); //!errorhandler?
			return response;
		}
	}

	size_t headerEnd = session.cgiOutput.find("\r\n\r\n");
	std::string headersPart, bodyPart;
	if (headerEnd != std::string::npos) {
		headersPart = session.cgiOutput.substr(0, headerEnd);
		bodyPart = session.cgiOutput.substr(headerEnd + 4);
	} else {
		headerEnd = session.cgiOutput.find("\n\n");
		if (headerEnd != std::string::npos) {
			headersPart = session.cgiOutput.substr(0, headerEnd);
			bodyPart = session.cgiOutput.substr(headerEnd + 2);
		} else {
			bodyPart = session.cgiOutput;
		}
	}

	std::vector<std::string> lines = split(headersPart, "\n");
	for (size_t i = 0; i < lines.size(); ++i) {
		std::string line = trim(lines[i]);
		size_t colon = line.find(':');
		if (colon != std::string::npos) {
			std::string key = trim(line.substr(0, colon));
			std::string val = trim(line.substr(colon + 1));
			if (toLower(key) == "status") {
				statusCode = std::atoi(val.c_str());
			} else {
				response.setHeader(key, val);
			}
		}
	}

	response.setStatus(statusCode);
	response.setBody(bodyPart);
	return response;
}
