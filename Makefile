CC = gcc
CFLAGS += -Wall --std=c89 -g -O3 -static -Wtraditional
TARGET = aesh

all: $(TARGET)

$(TARGET): aesh.c
	$(CC) $(CFLAGS) -o $(TARGET) aesh.c

install:
	mkdir -p /usr/local/bin/
	install -m 755 $(TARGET) /usr/local/bin/

clean: 
	rm -f ${TARGET}

