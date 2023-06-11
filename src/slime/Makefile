INC_PATH = $(abspath ./)
SRC = $(shell find $(abspath ./) -name "*.c" -or -name "*.cpp")

BUILD_DIR = $(abspath ./build)
OBJ_DIR = $(BUILD_DIR)/obj

CC = g++
GDB = gdb
INCFLAG += $(addprefix -I , $(INC_PATH))
CFLAGS  += $(INCFLAG) 

TESTFILE = $(abspath ./test.txt)

BIN = $(OBJ_DIR)/compiler

$(BIN): $(SRC)
	$(shell mkdir -p $(OBJ_DIR)) 
	$(CC) $^ $(CFLAGS) -o $(OBJ_DIR)/compiler

run: $(BIN)
	$(abspath $(BIN)) $(TESTFILE)

clean: 
	rm -rf $(BUILD_DIR)

	