NAME		:= build/webserv

CPP			:= c++
CPPFLAGS 	:= -Wall -Wextra -Werror -std=c++98 -MMD -MP

# Directories
CPP_DIR		:= src
HPP_DIR		:= includes
OBJ_DIR		:= build

# Colors
GREEN 		:= \033[0;32m
YELLOW		:= \033[0;33m
RED			:= \033[0;31m
BLUE		:= \033[0;34m
RESET		:= \033[0m

# Files
SRC			:=	src/main.cpp \
				src/Client/Client.cpp \
				src/config/ConfigData.cpp \
				src/config/Parser.cpp \
				src/config/Tokenizer.cpp \
				src/HttpRequest/HttpRequest.cpp  \
				src/ServerManager/ServerManager.cpp
				src/Connection/Connection.cpp \
				src/HttpResponse/HttpResponse.cpp  \
				src/parser/arg_parser.cpp \
				src/Socket/Socket.cpp

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

.PHONY: all clean fclean re

-include $(DEPS)