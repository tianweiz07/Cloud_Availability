CC = gcc
LDFLAGS = -lpthread
TARGET1 = serial_stream
TARGET2 = random_stream
RM = rm -rf

all: $(TARGET1) $(TARGET2)

$(TARGET1): $(TARGET1).c
	$(CC) -o $(TARGET1) $(TARGET1).c $(LDFLAGS)

$(TARGET2): $(TARGET2).c
	$(CC) -o $(TARGET2) $(TARGET2).c $(LDFLAGS)

clean:
	$(RM) $(TARGET1) $(TARGET2)
