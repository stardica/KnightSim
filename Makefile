#compiler and linker
CC_FLAGS_64 = -g3 -O3 -Wall -Werror
CC_FLAGS_32 = -g3 -O3 -Wall -Werror -m32
CC = gcc
LINKER_FLAGS_64 = -static -lDESim64
LINKER_FLAGS_32 = -static -lDESim32

#DEsim
LIB_NAME_64 = "libDESim64.a"
LIB_NAME_32 = "libDESim32.a"

#Samples
DESIM_INC = ./
DESIM_LIB = ./


all: samples64 samples32

samples64: DESim64
		$(CC) $(CC_FLAGS_64) ./Samples/ProducerConsumer.c -o ./Samples/ProducerConsumer64 -I$(DESIM_INC) -L$(DESIM_LIB) $(LINKER_FLAGS_64)
		@echo "Built $@ successfully"
				
samples32: DESim32
		$(CC) $(CC_FLAGS_32) ./Samples/ProducerConsumer.c -o ./Samples/ProducerConsumer32 -I$(DESIM_INC) -L$(DESIM_LIB) $(LINKER_FLAGS_32)
		@echo "Built $@ successfully"
		

DESim: DESim64 DESim32

DESim64: desim64.o setjmp64.o longjmp64.o decode64.o encode64.o
		ar -r $(LIB_NAME_64) desim64.o setjmp64.o longjmp64.o decode64.o encode64.o
		@echo "Built $@ successfully"

DESim32: desim32.o setjmp32.o longjmp32.o decode32.o encode32.o
		ar -r $(LIB_NAME_32) desim32.o setjmp32.o longjmp32.o decode32.o encode32.o
		@echo "Built $@ successfully"


#64 bit versions
desim64.o:
	$(CC) $(CC_FLAGS_64) desim.c -c -o desim64.o

setjmp64.o: setjmp64.s
	$(CC) $(CC_FLAGS_64) setjmp64.s -c

longjmp64.o: longjmp64.s
	$(CC) $(CC_FLAGS_64) longjmp64.s -c

decode64.o: decode64.s
	$(CC) $(CC_FLAGS_64) decode64.s -c

encode64.o: encode64.s
	$(CC) $(CC_FLAGS_64) encode64.s -c


#32 bit versions
desim32.o:
	$(CC) $(CC_FLAGS_32) desim.c -c -o desim32.o
	
setjmp32.o: setjmp32.s
	$(CC) $(CC_FLAGS_32) setjmp32.s -c

longjmp32.o: longjmp32.s
	$(CC) $(CC_FLAGS_32) longjmp32.s -c
	
decode32.o: decode32.s
	$(CC) $(CC_FLAGS_32) decode32.s -c
	
encode32.o: encode32.s
	$(CC) $(CC_FLAGS_32) encode32.s -c
	
clean:
	rm -f *.o *.a ./Samples/ProducerConsumer64 ./Samples/ProducerConsumer32