NAME        = ircserv
CXX         = c++
CXXFLAGS    = -Wall -Wextra -Werror -std=c++98 -I includes/

SRC_DIR     = sources
OBJ_DIR     = obj

SRCS        = $(SRC_DIR)/main.cpp \
              $(SRC_DIR)/core/Server.cpp \
              $(SRC_DIR)/core/Network.cpp \
              $(SRC_DIR)/core/Parser.cpp \
              $(SRC_DIR)/commands/Authenticate.cpp \
              $(SRC_DIR)/commands/ChannelOps.cpp \
              $(SRC_DIR)/commands/Connection.cpp \
              $(SRC_DIR)/commands/Messaging.cpp \
              $(SRC_DIR)/commands/Bot.cpp \
              $(SRC_DIR)/models/Client.cpp \
              $(SRC_DIR)/models/Channel.cpp \
              $(SRC_DIR)/utils/Utils.cpp

OBJS        = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

RESET       = \033[0m
GREEN       = \033[1;32m
YELLOW      = \033[1;33m
RED         = \033[1;31m
CYAN        = \033[1;36m

all: $(NAME)

$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "$(GREEN)  $(NAME) successfully compiled!$(RESET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "$(CYAN)Compiling:$(RESET) $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR)
	@echo "$(YELLOW)  Object files cleaned.$(RESET)"

fclean: clean
	@rm -f $(NAME)
	@echo "$(RED)  Executable ($(NAME)) removed.$(RESET)"

re: fclean all

.PHONY: all clean fclean re