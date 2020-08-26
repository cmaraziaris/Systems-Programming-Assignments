# Makefile for diseaseAggregator
SRC = ./src

# .o files will be stored here
BLD = ./obj
BLD_COMMON = $(BLD)/common
BLD_MASTER = $(BLD)/master
BLD_WORKER = $(BLD)/worker
BLD_CLIENT = $(BLD)/client
BLD_SERVER = $(BLD)/server

# Specify derictories of .c files
MODULES = $(SRC)/modules
COMMS = $(SRC)/communication

WORKER = $(SRC)/worker
WORKER_QS = $(WORKER)/queries
WORKER_FIO = $(WORKER)/file_io

MASTER = $(SRC)/master
MASTER_SIG = $(MASTER)/signals
MASTER_TLS = $(MASTER)/tools

CLIENT = $(SRC)/whoClient
SERVER = $(SRC)/whoServer
SERVER_TLS = $(SERVER)/tools
SERVER_MSG = $(SERVER)/requests

# Compiler options
CC = gcc
CFLAGS = -Wall -Wextra -I. -I$(SRC) -I$(MODULES) -I$(COMMS) -Wno-unused-parameter
CFLAGS_MASTER = -I$(MASTER) -I$(MASTER_SIG) -I$(MASTER_TLS)
CFLAGS_WORKER = -I$(WORKER) -I$(WORKER_QS) -I$(WORKER_FIO)
CFLAGS_CLIENT = -I$(CLIENT)
CFLAGS_SERVER = -I$(SERVER) -I$(SERVER_MSG) -I$(SERVER_TLS)

# Executable file names
EXE_MASTER = ./master
EXE_WORKER = ./worker
EXE_CLIENT = ./whoClient
EXE_SERVER = ./whoServer

COMMON_OBJS = $(MODULES)/list.o $(MODULES)/avl.o $(MODULES)/hash_table.o
COMMON_OBJS += $(COMMS)/ipc.o $(COMMS)/network.o

# Client .o needed
OBJS_CLIENT = $(CLIENT)/client.o $(CLIENT)/validate.o $(CLIENT)/operate.o

# Server .o needed
OBJS_SERVER =  $(SERVER)/server.o $(SERVER_TLS)/validate.o $(SERVER_TLS)/circular_buf.o
OBJS_SERVER += $(SERVER_MSG)/worker_stats.o $(SERVER_MSG)/client_requests.o $(SERVER_MSG)/queries.o 

# Worker .o needed
OBJS_WORKER =  $(WORKER)/worker.o $(WORKER)/operate.o $(WORKER_QS)/glob_structs.o 
OBJS_WORKER += $(WORKER_FIO)/io_files.o $(WORKER_FIO)/file_parse.o  $(WORKER_QS)/date.o
OBJS_WORKER += $(WORKER_QS)/stats.o $(WORKER_QS)/queries.o $(WORKER_QS)/patients.o

# Master .o needed
OBJS_MASTER = $(MASTER)/master.o $(MASTER)/setup_workers.o $(MASTER_TLS)/validation.o
OBJS_MASTER += $(MASTER_SIG)/sig_manage.o $(MASTER_SIG)/sig_actions.o $(MASTER_TLS)/fifo_dir.o

$(OBJS_CLIENT): CFLAGS += $(CFLAGS_CLIENT)
$(OBJS_WORKER): CFLAGS += $(CFLAGS_WORKER)
$(OBJS_MASTER): CFLAGS += $(CFLAGS_MASTER)
$(OBJS_SERVER): CFLAGS += $(CFLAGS_SERVER)

# Build executables
all: $(EXE_MASTER) $(EXE_WORKER) $(EXE_CLIENT) $(EXE_SERVER)
	mkdir -p $(BLD_COMMON)
	mkdir -p $(BLD_MASTER)
	mkdir -p $(BLD_WORKER)
	mkdir -p $(BLD_CLIENT)
	mkdir -p $(BLD_SERVER)
	mv -f $(COMMON_OBJS) $(BLD_COMMON)
	mv -f $(OBJS_MASTER) $(BLD_MASTER)
	mv -f $(OBJS_WORKER) $(BLD_WORKER)
	mv -f $(OBJS_CLIENT) $(BLD_CLIENT)
	mv -f $(OBJS_SERVER) $(BLD_SERVER)

$(EXE_MASTER): $(OBJS_MASTER) $(COMMON_OBJS)
	$(CC) $(CFLAGS) $(OBJS_MASTER) $(COMMON_OBJS) -o $(EXE_MASTER)

$(EXE_WORKER): $(OBJS_WORKER) $(COMMON_OBJS)
	$(CC) $(CFLAGS) $(OBJS_WORKER) $(COMMON_OBJS) -o $(EXE_WORKER)

$(EXE_CLIENT): $(OBJS_CLIENT) $(COMMON_OBJS)
	$(CC) -pthread $(CFLAGS) $(OBJS_CLIENT) $(COMMON_OBJS) -o $(EXE_CLIENT)

$(EXE_SERVER): $(OBJS_SERVER) $(COMMON_OBJS)
	$(CC) -pthread $(CFLAGS) $(OBJS_SERVER) $(COMMON_OBJS) -o $(EXE_SERVER)


# Delete executable & object files
clean:
	rm -f $(EXE_MASTER)
	rm -f $(EXE_WORKER)
	rm -f $(EXE_CLIENT)
	rm -f $(EXE_SERVER)
	rm -rf $(BLD)

# make -s
# make --silent