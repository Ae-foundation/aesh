CC = gcc
CFLAGS += -Wall --std=gnu89 -g -O3
TARGET = aesh

all: $(TARGET)

$(TARGET): aesh.c
	$(CC) $(CFLAGS) -o $(TARGET) aesh.c

clean: 
	rm -f ${TARGET}

