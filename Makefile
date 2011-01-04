greenpkg_EXEC = greenpkg
greenpkg_SRC = main.c install.c remove.c update.c search.c list.c clean.c sql.c util.c

CFLAGS = -Wall -ansi -pedantic -O2 -lsqlite3 -lpthread -g

all: $(greenpkg_EXEC)

$(greenpkg_EXEC): $(greenpkg_SRC) local.h
	$(CC) $(CFLAGS) $(greenpkg_SRC) -o $@
