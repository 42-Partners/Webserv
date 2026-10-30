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
        		src/ServerManager/ServerManager.cpp \
				src/Connection/Connection.cpp \
				src/Handlers/UploadHandler.cpp  \
				src/HttpRequest/HttpRequest.cpp  \
				src/HttpRequest/RequestParser.cpp  \
				src/HttpRequest/RequestProcessor.cpp  \
				src/HttpResponse/ErrorPage.cpp  \
				src/HttpResponse/HttpResponse.cpp  \
				src/parser/arg_parser.cpp \
				src/Utils/Utils.cpp

OBJ			:= $(SRC:%.cpp=$(OBJ_DIR)/%.o)

DEPS 		:= $(OBJ:.o=.d)

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
					tests/request/parser.cpp \
					tests/request/status_code.cpp \
					src/HttpRequest/HttpRequest.cpp \
					src/HttpRequest/RequestParser.cpp \
					src/Utils/Utils.cpp \
					src/HttpResponse/ErrorPage.cpp

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

.PHONY: all clean fclean re test