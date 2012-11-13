all: server client

server: server.c err.c
	gcc -g server.c err.c -o server  
#-DDEBUG

client: client.c err.c
	gcc -g client.c err.c -o client  
#-DDEBUG

clean: 
	rm server client

./PHONY: clean
