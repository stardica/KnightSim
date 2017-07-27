
#DEsim
LIB_NAME_64 = "libDESim64.a"
CC_FLAGS_64 = -g -O3 -Wall

LIB_NAME_32 = "libDESim32.a"
CC_FLAGS_32 = -g -O3 -Wall -m32

CC = gcc

all: DESim64 DESim32

DESim64: tasking64.o wakeupcall64.o contexts64.o setjmp64.o longjmp64.o
		ar -r $(LIB_NAME_64) tasking64.o wakeupcall64.o contexts64.o setjmp64.o longjmp64.o
		@echo "Built $@ successfully"
		
DESim32: tasking32.o wakeupcall32.o contexts32.o setjmp32.o longjmp32.o
		ar -r $(LIB_NAME_32) tasking32.o wakeupcall32.o contexts32.o setjmp32.o longjmp32.o
		@echo "Built $@ successfully"


#64 bit versions
tasking64.o: 
	$(CC) $(CC_FLAGS_64) tasking.c -c -o tasking64.o
	
wakeupcall64.o:
	$(CC) $(CC_FLAGS_64) wakeupcall.c -c -o wakeupcall64.o 

contexts64.o:
	$(CC) $(CC_FLAGS_64) contexts.c -c -o contexts64.o
	
setjmp64.o: setjmp64.s
	$(CC) $(CC_FLAGS_64) setjmp64.s -c

longjmp64.o: longjmp64.s
	$(CC) $(CC_FLAGS_64) longjmp64.s -c


#32 bit versions
tasking32.o: 
	$(CC) $(CC_FLAGS_32) tasking.c -c -o tasking32.o 
	
wakeupcall32.o:
	$(CC) $(CC_FLAGS_32) wakeupcall.c -c -o wakeupcall32.o 

contexts32.o:
	$(CC) $(CC_FLAGS_32) contexts.c -c -o contexts32.o

setjmp32.o: setjmp32.s
	$(CC) $(CC_FLAGS_32) setjmp32.s -c

longjmp32.o: longjmp32.s
	$(CC) $(CC_FLAGS_32) longjmp32.s -c

clean:
	rm -f *.o *.a

