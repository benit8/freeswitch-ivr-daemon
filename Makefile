NAME = ivrd

SOURCES = src/main.c                \
          src/esl/esl.c             \
          src/esl/esl_buffer.c      \
          src/esl/esl_config.c      \
          src/esl/esl_event.c       \
          src/esl/esl_json.c        \
          src/esl/esl_threadmutex.c \
          deps/cJSON/cJSON.c

OBJS = $(SOURCES:.c=.o)

CFLAGS += -Iinclude -Ideps
CFLAGS += -Wall -Wextra
LDFLAGS += -lpthread

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) -o $(NAME) $(OBJS) $(LDFLAGS)

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all $(NAME) clean fclean re
