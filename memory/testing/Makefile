CC = gcc
LDFLAGS = -lpthread
TARGET = check_conflict_bank
VICTIM = victim
ATTACKER = attacker
RM = rm -rf

all: $(TARGET) $(VICTIM) $(ATTACKER)

$(TARGET): $(TARGET).c
	$(CC) -o $(TARGET) $(TARGET).c $(LDFLAGS)

$(VICTIM): $(VICTIM).c
	$(CC) -o $(VICTIM) $(VICTIM).c $(LDFLAGS)

$(ATTACKER): $(ATTACKER).c
	$(CC) -o $(ATTACKER) $(ATTACKER).c $(LDFLAGS)

clean:
	$(RM) $(TARGET) $(VICTIM) $(ATTACKER)
