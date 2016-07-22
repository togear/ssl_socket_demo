LDFLAGS = -lssl
CFLAGS = -g -O0 -Wall

all : ssl_client ssl_server ssl_recv_fd

ssl_client : ssl_client.o
	gcc $(CFLAGS) $(LDFLAGS) $^ -o $@

ssl_server : ssl_server.o unix_sock.o
	gcc -o ssl_server ssl_server.o unix_sock.o  $(LDFLAGS)

ssl_recv_fd : ssl_recv_fd.o
	gcc -o ssl_recv_fd ssl_recv_fd.o unix_sock.o  $(LDFLAGS)

ssl_server.o : ssl_server.c
	gcc -c ssl_server.c $(CFLAGS)

unix_sock.o : unix_sock.c
	gcc -c unix_sock.c $(CFLAGS) 

.PHONY : clean
clean :
	rm ssl_client ssl_server ssl_recv_fd ssl_client.o ssl_server.o unix_sock.o ssl_recv_fd.o

