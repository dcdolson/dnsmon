# Note: some features of this Makefile seem to require gmake

OBJS:=obj/main.o obj/GetTimestamp.o
CC:=gcc
#CFLAGS := -O2 -std=c++11 -g
CFLAGS := -O0 -std=c++11 -g

obj/dnsmon: $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS) -lstdc++

obj/%.o: %.cpp
	mkdir -p obj
	$(CC) -c -o $@ $< $(CFLAGS)

obj/main.o: PacketDump.h

