CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
TARGET = editor.exe
OBJS = main.o canvas.o shapes.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del /f /q $(OBJS) $(TARGET) 2>nul || rm -f $(OBJS) $(TARGET)
