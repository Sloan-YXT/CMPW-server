server:server.o data.o database.o cutil.o
	g++ server.o database.o cutil.o -lpthread -lmysqlclient -o server -g
CFLAGS=-w -std=c++14
ifdef DEBUG
CFLAGS+= -g
endif
ifdef TEST
CFLAGS += -DTEST 
endif
server.o:server.cpp
	g++ -c server.cpp -o server.o $(CFLAGS)
%.o:%.cpp %.h
	g++ -c $< -o $@ $(CFLAGS)
clean:
	rm server *.o
.PHONY:clean