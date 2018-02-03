#compiler and linker
CC_FLAGS_64 = -g3 -O3 -Wall -Werror
CC_FLAGS_32 = -g3 -O3 -Wall -Werror -m32
CC = g++

#DEsim
LIB_NAME_64 = "libKnightSim64.a"
LIB_NAME_32 = "libKnightSim32.a"

all: knightsim64 knightsim32

knightsim64: knightsim64.o setjmp64.o longjmp64.o decode64.o encode64.o
		ar -r ../lib/$(LIB_NAME_64) knightsim64.o setjmp64.o longjmp64.o decode64.o encode64.o
		@echo "Built $@ successfully"

knightsim32: knightsim32.o setjmp32.o longjmp32.o decode32.o encode32.o
		ar -r ../lib/$(LIB_NAME_32) knightsim32.o setjmp32.o longjmp32.o decode32.o encode32.o
		@echo "Built $@ successfully"


#64 bit versions
knightsim64.o:
	$(CC) $(CC_FLAGS_64) knightsim.c -c -o knightsim64.o

setjmp64.o: setjmp64.s
	$(CC) $(CC_FLAGS_64) setjmp64.s -c

longjmp64.o: longjmp64.s
	$(CC) $(CC_FLAGS_64) longjmp64.s -c

decode64.o: decode64.s
	$(CC) $(CC_FLAGS_64) decode64.s -c

encode64.o: encode64.s
	$(CC) $(CC_FLAGS_64) encode64.s -c


#32 bit versions
knightsim32.o:
	$(CC) $(CC_FLAGS_32) knightsim.c -c -o knightsim32.o
	
setjmp32.o: setjmp32.s
	$(CC) $(CC_FLAGS_32) setjmp32.s -c

longjmp32.o: longjmp32.s
	$(CC) $(CC_FLAGS_32) longjmp32.s -c
	
decode32.o: decode32.s
	$(CC) $(CC_FLAGS_32) decode32.s -c
	
encode32.o: encode32.s
	$(CC) $(CC_FLAGS_32) encode32.s -c
	
clean:
	rm -f \
	*.o \
	../lib/$(LIB_NAME_64) \
	../lib/$(LIB_NAME_32)
	
	
