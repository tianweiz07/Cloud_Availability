CC = gcc
LDFLAGS = -lpthread
TARGET = disk_stream
RM = rm -rf

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) -o $(TARGET) $(TARGET).c $(LDFLAGS)

clean:
	$(RM) $(TARGET)
