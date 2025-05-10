CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LDFLAGS = -lpthread
VALGRIND = valgrind --tool=memcheck --leak-check=full

TARGET = LogAnalyzer
SOURCES = 210104004065_main.c utils.c buffer.c

TEST_FILE = test.txt
SEARCH_TERM = lorem
NUM_WORKERS = 10
BUFFER_SIZE = 5

all: $(TARGET)

$(TARGET): $(SOURCES)
	@$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)
	@echo "Compiled $(TARGET)."

clean:
	@rm -f $(TARGET)
	@echo "Cleaned $(TARGET)."

# run: $(TARGET)
# 	@echo "Running $(TARGET) with $(NUM_WORKERS) $(BUFFER_SIZE) $(TEST_FILE) $(SEARCH_TERM)"
# 	@echo "=========================================="
# 	@./$(TARGET) $(NUM_WORKERS) $(BUFFER_SIZE) $(TEST_FILE) $(SEARCH_TERM)

# run2: $(TARGET)
# 	@echo "Running $(TARGET) with $(NUM_WORKERS) $(BUFFER_SIZE) $(TEST_FILE) $(SEARCH_TERM)"
# 	@echo "=========================================="
# 	@$(VALGRIND) ./$(TARGET) $(NUM_WORKERS) $(BUFFER_SIZE) $(TEST_FILE) $(SEARCH_TERM)
