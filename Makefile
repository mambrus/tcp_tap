.PHONY: all clean

all: tcp_tap

clean:
	rm -f *.o
	rm -f *.lib
	rm -f tcp_tap
	rm -f testserver

tcp_tap: Makefile tcp_tap.c server.c
	@rm -f tcp_tap
	@( gcc -otcp_tap -O0 -g3 main.c -lpthread 2>&1 ) | grcat conf.gcc

testserver: Makefile server.c
	@rm -f testserver
	@( gcc -otestserver -O0 -g3 -DTEST server.c -lpthread 2>&1 ) | grcat conf.gcc
