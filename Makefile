KNIGHTSIM_DIR = ./KnightSim/
LIB_DIR = ./lib/
INCLUDE_DIR = ./include/

CC_FLAGS_64 = -g3 -O3 -Wall
CC = gcc
LINKER_FLAGS_64 = -lKnightSim64

all: 
	$(MAKE) -C $(KNIGHTSIM_DIR) all
	$(CC) $(CC_FLAGS_64) Event.c -o Event64 -I$(KNIGHTSIM_DIR) -I$(INCLUDE_DIR) -L$(LIB_DIR) $(LINKER_FLAGS_64)
	@echo "Built $@ successfully"
	#$(CC) $(CC_FLAGS_64) ProducerConsumer.c -o ProducerConsumer64 -I$(KNIGHTSIM_DIR) -I$(INCLUDE_DIR) -L$(LIB_DIR) $(LINKER_FLAGS_64)
	#@echo "Built $@ successfully"
	#$(CC) $(CC_FLAGS_64) Arbiter.c -o Arbiter64 -I$(KNIGHTSIM_DIR) -I$(INCLUDE_DIR) -L$(LIB_DIR) $(LINKER_FLAGS_64)
	#@echo "Built $@ successfully"
	#$(CC) $(CC_FLAGS_64) Switch.c -o Switch64 -I$(KNIGHTSIM_DIR) -I$(INCLUDE_DIR) -L$(LIB_DIR) $(LINKER_FLAGS_64)
	#@echo "Built $@ successfully"

clean:
	$(MAKE) -C $(KNIGHTSIM_DIR) clean
	rm -f \
	ProducerConsumer64 \
	Arbiter64 \
	Switch64 \
	Event64
