# Note: some features of this Makefile seem to require gmake

OBJS:= \
    obj/main.o \
    obj/GetTimestamp.o \
    obj/Options.o

CC:=gcc
#CFLAGS := -O2 -std=c++11 -g
CFLAGS := -O0 -std=c++11 -g

obj/dnsmon: $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS) -lstdc++

obj/%.o: %.cpp
	mkdir -p obj
	$(CC) -c -o $@ $< $(CFLAGS)

obj/main.o: GetTimestamp.h Options.h PacketDump.h
obj/Options.o: Options.h
obj/GetTimestamp.o: GetTimestamp.h

