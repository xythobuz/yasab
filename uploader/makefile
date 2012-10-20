TARGET = yasab
SRC = main.c
SRC += serial.c
SRC += parser.c

CC = gcc
STANDARD = gnu99
RM = rm -f

# --------------------------------------

OBJ = $(SRC:.c=.o)

all: $(TARGET)

%.o : %.c
	$(CC) -x c -std=$(STANDARD) -c $< -o $@

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $(OBJ)

# Delete intermediate files
clean:
	$(RM) *.o
	$(RM) $(TARGET)
