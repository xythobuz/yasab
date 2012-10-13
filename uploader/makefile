TARGET = uploader
SRC = main.c

CC = gcc
STANDARD = gnu99

ifdef SystemRoot
# Windows
RM = del
SRC += winSerial.c
else
# Probably Unix
RM = rm -f
SRC += serial.c
endif

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
