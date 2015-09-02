CC = gcc
LDFLAGS = -lpthread
TARGET = stream
RM = rm -rf

all: $(TARGET) 

$(TARGET): $(TARGET).c
	$(CC) -o $(TARGET) $(TARGET).c $(LDFLAGS)

clean:
	$(RM) $(TARGET) 
