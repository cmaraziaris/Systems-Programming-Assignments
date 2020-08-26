
SRC = ./src
BLD = ./obj

MODULES = $(SRC)/modules
TOOLS = $(SRC)/tools
CORE = $(SRC)/core

CC = gcc
CFLAGS = -Wall -Wextra -I. -I$(MODULES) -I$(CORE) -I$(TOOLS)

PROGRAM = diseaseMonitor

OBJS =  $(SRC)/main.o
OBJS += $(MODULES)/avl.o $(MODULES)/hash_table.o $(MODULES)/binary_heap.o
OBJS += $(CORE)/helpers.o $(CORE)/stats.o $(CORE)/patients.o
OBJS += $(TOOLS)/date.o $(TOOLS)/utilities.o $(TOOLS)/interface.o

$(PROGRAM): clean $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(PROGRAM)
	mkdir -p $(BLD)
	mv $(OBJS) $(BLD)

clean:		# delete executable & object files
	rm -f $(PROGRAM)
	rm -rf $(BLD)