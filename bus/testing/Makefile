CC = gcc

all: user

user:
	$(CC) -o sequential_access sequential_access.c -lpthread
	$(CC) -o random_access random_access.c -lpthread
clean:
	rm -rf sequential_access random_access
