OBJS:=recentmost.o heap.o
TGT:=$(firstword $(OBJS:%.o=%))
CFLAGS:=-ansi -pedantic -Wall -Wextra -s -Wno-long-long

$(TGT): $(OBJS)

clean:
	rm $(OBJS) *~
