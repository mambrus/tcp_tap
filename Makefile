.PHONY: all clean

all: tcp_tap
	@( gcc -otcp_tap -O0 -g3 main.c -lpthread 2>&1 ) | grcat conf.gcc

clean:
	rm -f *.o
	rm -f *.lib
	rm -f tcp_tap
	rm -f testserver

testserver:
	@( gcc -otestserver -O0 -g3 -DTEST server.c -lpthread 2>&1 ) | grcat conf.gcc
