.PHONY: all clean install

all: install

clean:
	rm -f *.o
	rm -f *.lib
	rm -f tcp_tap
	rm -f testserver
	rm -f switchboard

install: ${HOME}/bin/tcp_tap

${HOME}/bin/tcp_tap: tcp_tap
	rm -f ${HOME}/bin/tcp_tap
	cp tcp_tap ${HOME}/bin/tcp_tap

tcp_tap2: Makefile main.c 
	@rm -f tcp_tap
	@( gcc -otcp_tap -O0 -g3 main.c -lpthread 2>&1 ) | grcat conf.gcc

tcp_tap: Makefile main.c server.c server.h switchboard.c switchboard.h
	@rm -f tcp_tap
	( gcc -otcp_tap -O0 -g3 main.c switchboard.c server.c -lpthread 2>&1 ) | grcat conf.gcc

switchboard: Makefile switchboard.c  switchboard.h server.c server.h
	@rm -f switchboard
	@( gcc -oswitchboard -O0 -g3 -DTEST_SWITCH switchboard.c server.c -lpthread 2>&1 ) | grcat conf.gcc

switchboard2: Makefile switchboard.c  switchboard.h server.c server.h
	@( gcc -oswitchboard2 -O0 -g3 -DTEST2 switchboard.c server.c -lpthread 2>&1 ) | grcat conf.gcc
	
testserver: Makefile server.c server.h
	@rm -f testserver
	@( gcc -otestserver -O0 -g3 -DTEST server.c -lpthread 2>&1 ) | grcat conf.gcc
