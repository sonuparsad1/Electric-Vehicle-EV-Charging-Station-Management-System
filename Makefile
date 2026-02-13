CC=gcc
CFLAGS=-Wall -Wextra -std=c11 -pthread -Iinclude

SRC_DIR=src
OBJ_DIR=build

SRC=$(SRC_DIR)/main.c $(SRC_DIR)/auth.c $(SRC_DIR)/vehicle.c $(SRC_DIR)/queue.c $(SRC_DIR)/charger.c $(SRC_DIR)/billing.c $(SRC_DIR)/dashboard.c $(SRC_DIR)/report.c $(SRC_DIR)/logger.c
OBJ=$(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TARGET=ev_station

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
