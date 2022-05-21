COLOR_NONE = \e[0m
COLOR_WHITE = \e[1;37m
COLOR_BLUE = \e[1;34m
COLOR_GREEN = \e[1;32m
COLOR_PURPLE = \e[1;35m

SRC_DIR = .
OBJ_DIR = obj/

PING_SRCS =		$(SRC_DIR)/argv_handler.c 	$(SRC_DIR)/icmp4.c		$(SRC_DIR)/icmp6.c											\
				$(SRC_DIR)/main.c			$(SRC_DIR)/profile.c	$(SRC_DIR)/utils.c
PING_INCS =		ping.h
PING_OBJS =		$(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)%.o,$(PING_SRCS))

#minilib
MINILIB_DIR =	minilib
MINILIB_SRC =	$(MINILIB_DIR)/ft_atof.c	$(MINILIB_DIR)/ft_atoi.c	$(MINILIB_DIR)/ft_bzero.c	$(MINILIB_DIR)/ft_memcpy.c	\
				$(MINILIB_DIR)/ft_memset.c  $(MINILIB_DIR)/ft_strdup.c	$(MINILIB_DIR)/ft_strlen.c	$(MINILIB_DIR)/is_number.c

MINILIB_INCS = 	$(MINILIB_DIR)/minilib.h
MINILIB_OBJS = 	$(patsubst $(MINILIB_DIR)/%.c,$(OBJ_DIR)%.o,$(MINILIB_SRC))

NAME = ft_ping

CC = clang
LIBS = -lm
CCFL = -Wall -Wextra -Werror -O2

.PHONY: all clean fclean re

all: $(NAME)

$(NAME): $(PING_OBJS) $(MINILIB_OBJS)
	@printf "$(COLOR_GREEN)Compiled successfully$(COLOR_NONE)\n"
	@printf "$(COLOR_GREEN)Linking...$(COLOR_NONE)\n"
	@$(CC) $(PING_OBJS) $(MINILIB_OBJS) $(LIBS) -o $(NAME)
	@printf "$(COLOR_GREEN)Built successfully$(COLOR_NONE)\n"

$(OBJ_DIR)%.o:$(SRC_DIR)/%.c $(PING_INCS)
	@printf "$(COLOR_GREEN)Compiling $(COLOR_PURPLE)$<$(COLOR_GREEN) to $(COLOR_PURPLE)$@$(COLOR_GREEN)...$(COLOR_NONE)\n"
	@$(CC) $(CCFL) $(INCL) $< -c -o $@

$(OBJ_DIR)%.o:$(MINILIB_DIR)/%.c $(MINILIB_INCS)
	@printf "$(COLOR_GREEN)Compiling $(COLOR_PURPLE)$<$(COLOR_GREEN) to $(COLOR_PURPLE)$@$(COLOR_GREEN)...$(COLOR_NONE)\n"
	@$(CC) $(CCFL) $(INCL) $< -c -o $@

clean:
	@printf "$(COLOR_GREEN)Cleaning object files...$(COLOR_NONE)\n"
	@/bin/rm -rf $(PING_OBJS) $(MINILIB_OBJS)
	@printf "$(COLOR_GREEN)Cleaned successfully$(COLOR_NONE)\n"

fclean: clean
	@printf "$(COLOR_GREEN)Cleaning executable...$(COLOR_NONE)\n"
	@/bin/rm -f $(NAME)

re: fclean all