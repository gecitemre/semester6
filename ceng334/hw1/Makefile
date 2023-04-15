SRCS = bgame.cpp logging.c logging.h message.c message.h

bgame: $(SRCS)
	g++ $(SRCS) -o bgame

tar: $(SRCS) Makefile
	tar -cf e2521581.tar.gz bgame.cpp Makefile
	@echo shell($?)

test: tar
	tar -xf e2521581.tar.gz
	g++ $(SRCS) -o bgame
	@echo "Last command $(.SHELLSTATUS)"