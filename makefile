BINARY = uzlog
CC = gcc
CFLAGS = -Wall --std=c99 -g -O0 -c
LDFLAGS = -lm

SRCPATH = src
OBJPATH = obj

SRC = $(wildcard $(SRCPATH)/*.c)
OBJ = $(SRC:$(SRCPATH)/%.c=$(OBJPATH)/%.o)

$(BINARY): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJPATH)/%.o: $(SRCPATH)/%.c
	@mkdir -p $(OBJPATH)
	$(CC) $(CFLAGS) -o $@ -c $<
