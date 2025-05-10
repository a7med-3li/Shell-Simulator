CC = gcc
CFLAGS = -Wall -Wextra
TARGET = shell_simulator

all: $(TARGET)

$(TARGET): shell_simulator.c
	$(CC) $(CFLAGS) -o $(TARGET) shell_simulator.c

clean:
	rm -f $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
	cp dos_linux_mapping.txt /usr/local/etc/

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean install run
