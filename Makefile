NAME        = ircserv

CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++98

SRC_DIR     = server
OBJ_DIR     = obj
INC_DIR     = .

SRCS        = main.cpp \
              $(SRC_DIR)/server.cpp \
              $(SRC_DIR)/channel.cpp

OBJS        = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SRCS))

RESET       = \033[0m
GREEN       = \033[1;32m
YELLOW      = \033[1;33m
RED         = \033[1;31m
CYAN        = \033[1;36m

all: $(NAME)

$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "$(GREEN)✔ $(NAME) basariyla derlendi!$(RESET)"

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "$(CYAN)Compiling:$(RESET) $<"
	@$(CXX) $(CXXFLAGS) -I$(INC_DIR) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR)
	@echo "$(YELLOW)🧹 Nesne dosyalari (obj/) temizlendi.$(RESET)"

fclean: clean
	@rm -f $(NAME)
	@echo "$(RED)🗑️ Executable ($(NAME)) silindi.$(RESET)"

re: fclean all