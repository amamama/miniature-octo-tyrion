
TARGET=a.out
CC=gcc
CFLAGS= -ansi -std=c99 -c -W -Wall -Werror
LDFLAGS= -lcurl -loauth -lcrypto
OBJS = \
	tweet.o \
	main.o


all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)


.c.o :
	gcc $(CFLAGS) -O2 $<

clean:
	-rm $(TARGET)
	-rm *.o

