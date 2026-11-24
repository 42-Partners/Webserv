NAME		:= build/webserv

CPP			:= c++
CPPFLAGS 	:= -Wall -Wextra -Werror -std=c++98 -MMD -MP

# Directories
CPP_DIR		:= src
HPP_DIR		:= includes
OBJ_DIR		:= build

TEST_DIR    := $(OBJ_DIR)/test

# Colors
GREEN 		:= \033[0;32m
YELLOW		:= \033[0;33m
RED			:= \033[0;31m
BLUE		:= \033[0;34m
RESET		:= \033[0m

# Files
SRC			:=	src/main.cpp \
				src/config/Tokenizer.cpp \
				src/config/LocationConfig.cpp \
				src/config/ServerConfig.cpp \
				src/config/Parser.cpp \
				src/ServerManager/ServerManager.cpp \
				src/Connection/Connection.cpp \
				src/Handlers/AutoIndexHandler.cpp \
				src/Handlers/ErrorHandler.cpp  \
				src/Handlers/RedirectHandler.cpp \
				src/Handlers/StaticFileHandler.cpp  \
				src/Handlers/UploadHandler.cpp  \
				src/Handlers/CgiHandler.cpp \
				src/HttpRequest/HttpRequest.cpp  \
				src/HttpRequest/RequestParser.cpp  \
				src/HttpRequest/RequestProcessor.cpp  \
				src/HttpResponse/ErrorPage.cpp  \
				src/HttpResponse/HttpResponse.cpp  \
				src/Logger/Logger.cpp  \
				src/Utils/Utils.cpp

OBJ			:= $(SRC:%.cpp=$(OBJ_DIR)/%.o)

DEPS 		:= $(OBJ:.o=.d)

TESTER_CONF = config/tester.conf

$(NAME): $(OBJ)
	@echo "$(YELLOW)🔧 Linking objects...$(RESET)"
	@$(CPP) $(CPPFLAGS) $(OBJ) -o $(NAME)
	@echo "$(GREEN)✅ $(NAME) built successfully$(RESET)"

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "$(BLUE)Compiling $<$(RESET)"
	@$(CPP) $(CPPFLAGS) -I$(HPP_DIR) -c $< -o $@

# rules
all: $(NAME)

clean:
	@rm -rf $(OBJ_DIR)
	@echo "$(RED)🧴 Object files removed.$(RESET)"

fclean: clean
	@rm -f $(NAME)
	@echo "$(RED)🧼 Everything cleaned$(RESET)"

re: fclean all

-include $(DEPS)

# tests

TEST_REQUEST     := $(TEST_DIR)/test_request_line

TEST_REQUEST_SRC := tests/main.cpp \
					tests/request/location.cpp \
					tests/request/parser.cpp \
					tests/request/route.cpp \
					tests/request/status_code.cpp \
					src/config/ServerConfig.cpp \
					src/config/LocationConfig.cpp \
					src/Handlers/ErrorHandler.cpp \
					src/Handlers/RedirectHandler.cpp \
					src/Handlers/StaticFileHandler.cpp \
					src/Handlers/UploadHandler.cpp \
					src/HttpRequest/HttpRequest.cpp \
					src/HttpRequest/RequestParser.cpp \
					src/HttpResponse/ErrorPage.cpp \
					src/HttpResponse/HttpResponse.cpp \
					src/Utils/Utils.cpp

TEST_HEADERS     := $(wildcard $(HPP_DIR)/*/*.hpp tests/*.hpp tests/*/*.hpp)

TEST_CPPFLAGS    := $(CPPFLAGS) -I$(HPP_DIR) -Isrc

$(TEST_DIR):
	mkdir -p $(TEST_DIR)

$(TEST_REQUEST): $(TEST_REQUEST_SRC) | $(TEST_DIR)
	@echo "$(YELLOW)🔧 Building request line tests...$(RESET)"
	@$(CPP) $(TEST_CPPFLAGS) $(TEST_REQUEST_SRC) -o $@
	@echo "$(GREEN)✅ Tests built successfully$(RESET)"

test: $(TEST_REQUEST)
	@echo "$(BLUE)🧪 Running request line tests...$(RESET)"
	@./$(TEST_REQUEST)

all_tests: $(NAME)
	@echo "$(BLUE)🧪 Running all tests...$(RESET)"
	./tests/integration/base_test.sh
	./tests/integration/cgi_test.sh
	./tests/integration/stress_test.sh
	@echo "$(GREEN)✅ All tests passed successfully$(RESET)"

stop-port:
	@fuser -k 8080/tcp 2>/dev/null || true

subtest: $(NAME) stop-port
	@./$(NAME) $(TESTER_CONF) > /tmp/webserv_subtest.log 2>&1 & \
	SERVER_PID=$$!; \
	trap "kill $$SERVER_PID 2>/dev/null || true; wait $$SERVER_PID 2>/dev/null || true" EXIT INT TERM; \
	sleep 0.5; \
	echo "🧪 Running subject tester with $(TESTER_CONF)..."; \
	./tests/tester http://localhost:8080

.PHONY: all clean fclean re test