#DEsim
LIB_NAME_64 = "libDESim64.a"
CC_FLAGS_64 = -g3 -O3 -Wall

LIB_NAME_32 = "libDESim32.a"
CC_FLAGS_32 = -g3 -O3 -Wall -m32

CC = gcc


all: DESim64 DESim32

DESim64: desim64.o eventcount64.o list64.o tasking64.o contexts64.o setjmp64.o longjmp64.o decode64.o encode64.o
		ar -r $(LIB_NAME_64) desim64.o eventcount64.o list64.o tasking64.o contexts64.o setjmp64.o longjmp64.o decode64.o encode64.o
		@echo "Built $@ successfully"
		
DESim32: desim32.o eventcount32.o list32.o tasking32.o contexts32.o setjmp32.o longjmp32.o decode32.o encode32.o
		ar -r $(LIB_NAME_32) desim32.o eventcount32.o list32.o tasking32.o contexts32.o setjmp32.o longjmp32.o decode32.o encode32.o
		@echo "Built $@ successfully"



#64 bit versions
desim64.o:
	$(CC) $(CC_FLAGS_64) desim.c -c -o desim64.o
	
eventcount64.o:
	$(CC) $(CC_FLAGS_64) eventcount.c -c -o eventcount64.o
	
list64.o:
	$(CC) $(CC_FLAGS_64) list.c -c -o list64.o

tasking64.o: 
	$(CC) $(CC_FLAGS_64) tasking.c -c -o tasking64.o
	
contexts64.o:
	$(CC) $(CC_FLAGS_64) contexts.c -c -o contexts64.o
	
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
	
eventcount32.o:
	$(CC) $(CC_FLAGS_32) eventcount.c -c -o eventcount32.o
	
list32.o:
	$(CC) $(CC_FLAGS_32) list.c -c -o list32.o

tasking32.o: 
	$(CC) $(CC_FLAGS_32) tasking.c -c -o tasking32.o 
	
contexts32.o:
	$(CC) $(CC_FLAGS_32) contexts.c -c -o contexts32.o

setjmp32.o: setjmp32.s
	$(CC) $(CC_FLAGS_32) setjmp32.s -c

longjmp32.o: longjmp32.s
	$(CC) $(CC_FLAGS_32) longjmp32.s -c
	
decode32.o: decode32.s
	$(CC) $(CC_FLAGS_32) decode32.s -c
	
encode32.o: encode32.s
	$(CC) $(CC_FLAGS_32) encode32.s -c
	

clean:
	rm -f *.o *.a