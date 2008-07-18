DEPS=clutter-0.8 clutter-md2-0.1 cairo
LDFLAGS=`pkg-config $(DEPS) --libs`
CFLAGS=`pkg-config $(DEPS) --cflags` -g -Wall
OBJS=tractordodge.o tdnumber.o tdcornerlayout.o

all : tractordodge

tractordodge : $(OBJS)
	gcc $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o : %.c
	gcc $(CFLAGS) -c -o $@ $<

clean :
	rm -f *.o tractordodge

.PHONY : clean all
