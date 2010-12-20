C_WARNINGS = -Wall 
CFLAGS = -O2 -I/opt/local/include -I/opt/local/lib -DXP_UNIX $(C_WARNINGS)
ifdef JSPC_OUTPUT
CFLAGS += -DJSPC_OUTPUT 
endif 
LDLIBS = -L. -lpthread -ljs
LDFLAGS = -Wl,-rpath,`pwd`
CC = gcc
RM = rm -f
BINS = xHunter
LIB = xh.dylib

.PHONY: all clean

all: $(LIB)

xHunter.o: xHunter.c
	$(CC) $(CFLAGS) -fPIC -c $<

xHunter: xHunter.o
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

xh.dylib: xHunter.o
	$(CC) -dynamiclib $(CFLAGS)  $< -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	$(RM) *.o $(BINS) $(LIB)
