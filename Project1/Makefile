AR=ar
AS=as
CC=gcc
CPP=cpp
CXX=g++
LD=ld
OBJCOPY = objcopy
OBJDUMP = objdump
STRIP = strip

INC_DIR = ./include
SRC_DIR = ./src
OBJ_DIR = ./obj
BIN_DIR = ./bin

DEBUG_MODE=TRUE



ifdef DEBUG_MODE
DEFINES  += -DDEBUG
CFLAGS   += -g -ggdb -D_GLIBCXX_DEBUG
#LDFLAGS  += -Wl,-no_pie
else 
CFLAGS   += -O3
endif

INCLUDE  += -I $(INC_DIR)
CFLAGS   +=  -Wall

#LDFLAGS += -lgdk_imlib
CPPFLAGS += -std=c++11 
GAME_NAME = ashell

GAME_OBJS = $(OBJ_DIR)/main.o                   \


all: directories $(BIN_DIR)/$(GAME_NAME)

$(BIN_DIR)/$(GAME_NAME): $(GAME_OBJS)
	$(CXX) $(GAME_OBJS) -o $(BIN_DIR)/$(GAME_NAME) $(CFLAGS) $(CPPFLAGS) $(DEFINES)
	
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CFLAGS) $(CPPFLAGS) $(DEFINES) $(INCLUDE) -c $< -o $@
	
.PHONY: directories
directories:
	mkdir -p $(OBJ_DIR)
	
clean::
	-rm $(GAME_OBJS)
	
.PHONY: clean