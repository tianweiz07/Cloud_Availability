CC = gcc

all: user

user:
	$(CC) -o sender sender.c -lpthread
	$(CC) -o receiver receiver.c -lpthread
clean:
	rm -rf sender receiver
