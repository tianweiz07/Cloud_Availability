CC = gcc
LDFLAGS = -lpthread
TARGET = check_conflict_bank
RM = rm -rf

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) -o $(TARGET) $(TARGET).c $(LDFLAGS)

clean:
	$(RM) $(TARGET)
