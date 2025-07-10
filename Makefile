CC = gcc
CFLAGS += -Wall --std=gnu89 -g -O3
TARGET = aesh

all: $(TARGET)

$(TARGET): aesh.c
	$(CC) $(CFLAGS) -o $(TARGET) aesh.c

install:
	mkdir -p /usr/local/bin/
	install -m 755 $(TARGET) /usr/local/bin/

clean: 
	rm -f ${TARGET}

