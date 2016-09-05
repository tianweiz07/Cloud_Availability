CC = gcc
LDFLAGS = -lpthread
TARGET = memory_access
RM = rm -rf

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) -o $(TARGET) $(TARGET).c $(LDFLAGS)

clean:
	$(RM) $(TARGET)
