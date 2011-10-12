.PHONY: all clean install

all: install


HAS_GRCAT_BIN := $(shell which grcat)

ifeq ($(HAS_GRCAT_BIN),"")
    HAS_GRCAT=no
else
    HAS_GRCAT := $(shell if [ -f ~/.grc/conf.gcc ]; then echo "yes"; else echo "no"; fi)
    ifeq ($(HAS_GRCAT), no)
        $(warning "NOTE: you have grcat installed, but no configuration file in for it (~/.grc/conf.gcc)")
    endif
endif


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

ifeq ($(HAS_GRCAT), yes)
tcp_tap: Makefile main.c tcp_tap.h server.c server.h switchboard.c switchboard.h sig_mngr.h sig_mngr.c
	@rm -f tcp_tap
	@( gcc -otcp_tap -O0 -g3 main.c switchboard.c server.c sig_mngr.c -lpthread 2>&1 ) | grcat conf.gcc

switchboard: Makefile switchboard.c  switchboard.h server.c server.h
	@rm -f switchboard
	@( gcc -oswitchboard -O0 -g3 -DTEST_SWITCH switchboard.c server.c -lpthread 2>&1 ) | grcat conf.gcc

switchboard2: Makefile switchboard.c  switchboard.h server.c server.h
	@( gcc -oswitchboard2 -O0 -g3 -DTEST2 switchboard.c server.c -lpthread 2>&1 ) | grcat conf.gcc
	
testserver: Makefile server.c server.h
	@rm -f testserver
	@( gcc -otestserver -O0 -g3 -DTEST server.c -lpthread 2>&1 ) | grcat conf.gcc
else

tcp_tap: Makefile main.c tcp_tap.h server.c server.h switchboard.c switchboard.h sig_mngr.h sig_mngr.c
	@rm -f tcp_tap
	gcc -otcp_tap -O0 -g3 main.c switchboard.c server.c sig_mngr.c -lpthread

switchboard: Makefile switchboard.c  switchboard.h server.c server.h
	@rm -f switchboard
	@gcc -oswitchboard -O0 -g3 -DTEST_SWITCH switchboard.c server.c -lpthread

switchboard2: Makefile switchboard.c  switchboard.h server.c server.h
	@gcc -oswitchboard2 -O0 -g3 -DTEST2 switchboard.c server.c -lpthread
	
testserver: Makefile server.c server.h
	@rm -f testserver
	@gcc -otestserver -O0 -g3 -DTEST server.c -lpthread 2>&1

endif
