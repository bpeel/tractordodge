DEPS=clutter-0.7 clutter-md2-0.1
LDFLAGS=`pkg-config $(DEPS) --libs`
CFLAGS=`pkg-config $(DEPS) --cflags` -g -Wall
OBJS=tractordodge.o

all : tractordodge

tractordodge : $(OBJS)
	gcc $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o : %.c
	gcc $(CFLAGS) -c -o $@ $<

clean :
	rm -f *.o tractordodge

.PHONY : clean all
