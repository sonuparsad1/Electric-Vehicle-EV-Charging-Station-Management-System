CC=gcc
CFLAGS=-Wall -Wextra -std=c11 -pthread

SRC=main.c auth.c vehicle.c queue.c charger.c billing.c dashboard.c report.c logger.c
OBJ=$(SRC:.c=.o)
TARGET=ev_station

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
