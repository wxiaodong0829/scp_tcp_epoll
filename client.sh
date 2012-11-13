#! /bin/bash
 
./client 127.0.0.1:./serv/2.rmvb ./cli &
./client 127.0.0.1:./serv/server2.c ./cli &
./client ./cli/client.c 127.0.0.1:./serv &
./client ./cli/1.rmvb 127.0.0.1:./serv &

./client ./cli/server.c 127.0.0.1:./serv/ &
./client 127.0.0.1:./serv/client2.c ./cli &
./client 127.0.0.1:./serv/Makefile2 ./cli &
#./client 127.0.0.1:./serv/fs ./cli &
