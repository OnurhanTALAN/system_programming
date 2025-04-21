CC = gcc
CFLAGS = -Wall -Wextra -pthread -std=c17

SOURCES = engineerSatellite.c
TARGET = engineerSatellite
NUM_SATELLITES = 10

all : clean $(TARGET)

$(TARGET) : $(SOURCES)
	@$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)
	@echo "Build complete"
	@echo "build program name 'engineerSatellite'"
	@echo "type 'make run' to run"
run : $(TARGET)
	@echo "===== STARTING ====="
	@./$(TARGET) $(NUM_SATELLITES)
	
clean :
	@rm -f $(TARGET)
	@echo "Cleaned"
	

