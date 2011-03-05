.PHONY: all clean

all: tcp_tap

clean:
	rm -f *.o
	rm -f *.lib
	rm -f tcp_tap
	rm -f testserver
	rm -f switchboard

tcp_tap2: Makefile main.c 
	@rm -f tcp_tap
	@( gcc -otcp_tap -O0 -g3 main.c -lpthread 2>&1 ) | grcat conf.gcc
	cp tcp_tap ~/bin/tcp_tap

tcp_tap: Makefile main.c server.c server.h switchboard.c switchboard.h
	@rm -f tcp_tap
	@( gcc -otcp_tap -O0 -g3 main.c switchboard.c server.c -lpthread 2>&1 ) | grcat conf.gcc
	cp tcp_tap ~/bin/tcp_tap

switchboard: Makefile switchboard.c  switchboard.h server.c server.h
	@rm -f switchboard
	@( gcc -oswitchboard -O0 -g3 -DTEST_SWITCH switchboard.c server.c -lpthread 2>&1 ) | grcat conf.gcc

switchboard2: Makefile switchboard.c  switchboard.h server.c server.h
	@( gcc -oswitchboard2 -O0 -g3 -DTEST2 switchboard.c server.c -lpthread 2>&1 ) | grcat conf.gcc
	

testserver: Makefile server.c server.h
	@rm -f testserver
	@( gcc -otestserver -O0 -g3 -DTEST server.c -lpthread 2>&1 ) | grcat conf.gcc
