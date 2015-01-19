CC = gcc
LDFLAGS = -lpthread
TARGET = mem_stream
TARGET1 = adaptive_mem_stream
RM = rm -rf

all: $(TARGET) $(TARGET1)

$(TARGET): $(TARGET).c
	$(CC) -o $(TARGET) $(TARGET).c $(LDFLAGS)

$(TARGET1): $(TARGET1).c
	$(CC) -o $(TARGET1) $(TARGET1).c $(LDFLAGS)

clean:
	$(RM) $(TARGET) $(TARGET1)
