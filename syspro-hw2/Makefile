# Makefile for diseaseAggregator
SRC = ./src

# .o files will be stored here
BLD = ./obj
BLD_COMMON = $(BLD)/common
BLD_MASTER = $(BLD)/master
BLD_WORKER = $(BLD)/worker

# Specify derictories of .c files
MODULES = $(SRC)/modules
TOOLS = $(SRC)/tools

WORKER = $(SRC)/worker
WORKER_QS = $(WORKER)/queries
WORKER_FIO = $(WORKER)/file_io

MASTER = $(SRC)/master
MASTER_SIG = $(MASTER)/signals

# Compiler options
CC = gcc
CFLAGS = -Wall -Wextra -I. -I$(SRC) -I$(MODULES) -Wno-unused-parameter
CFLAGS += -I$(TOOLS) -I$(MASTER) -I$(MASTER_SIG) -I$(WORKER) -I$(WORKER_QS) -I$(WORKER_FIO)

# Executable file names
EXE_MASTER = ./diseaseAggregator
EXE_WORKER = ./diseaseAggregator_worker

COMMON_OBJS = $(MODULES)/list.o $(MODULES)/avl.o $(MODULES)/hash_table.o
COMMON_OBJS += $(TOOLS)/ipc.o $(TOOLS)/date.o  $(TOOLS)/fifo_dir.o

# Worker .o needed
OBJS_WORKER =  $(WORKER)/worker.o $(WORKER)/signal_handling.o 
OBJS_WORKER += $(WORKER_FIO)/io_files.o $(WORKER_FIO)/file_parse.o
OBJS_WORKER += $(WORKER_QS)/stats.o $(WORKER_QS)/queries.o $(WORKER_QS)/patients.o  $(WORKER_QS)/glob_structs.o

# Master .o needed
OBJS_MASTER = $(MASTER)/master.o $(MASTER)/setup_workers.o $(MASTER)/m_queries.o $(MASTER)/validation.o
OBJS_MASTER += $(MASTER_SIG)/sig_manage.o $(MASTER_SIG)/sig_actions.o

# Build executables
all: $(EXE_MASTER) $(EXE_WORKER)
	mkdir -p $(BLD_COMMON)
	mkdir -p $(BLD_MASTER)
	mkdir -p $(BLD_WORKER)
	mv -f $(COMMON_OBJS) $(BLD_COMMON)
	mv -f $(OBJS_MASTER) $(BLD_MASTER)
	mv -f $(OBJS_WORKER) $(BLD_WORKER)

$(EXE_MASTER): $(OBJS_MASTER) $(COMMON_OBJS)
	$(CC) $(CFLAGS) $(OBJS_MASTER) $(COMMON_OBJS) -o $(EXE_MASTER)

$(EXE_WORKER): $(OBJS_WORKER) $(COMMON_OBJS)
	$(CC) $(CFLAGS) $(OBJS_WORKER) $(COMMON_OBJS) -o $(EXE_WORKER)

# Delete executable & object files
clean:
	rm -f $(EXE_MASTER)
	rm -f $(EXE_WORKER)
	rm -rf $(BLD)

# Clean and compile
comp: clean all