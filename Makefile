# Nom de l'exécutable
TARGET = myprogram

# Dossiers
SRC_DIR = src
INCLUDE_DIR = include

# Compilation
CC = gcc
CFLAGS = -I$(INCLUDE_DIR)
LDFLAGS = -lminiupnpc -lncurses

SRC = $(SRC_DIR)/main.c \
	  $(SRC_DIR)/client.c \
	  $(SRC_DIR)/server.c \
	  $(SRC_DIR)/crypt.c

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

# %.o: %.c
# 	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage
clean:
	rm -rf $(OBJ)
fclean: clean
	rm -rf $(TARGET)

re: fclean all
