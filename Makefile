CC=gcc

ODIR=.
LIBS = -lmsgpackc

all: server client

server: server.o
	$(CC) -o server server.o $(LIBS)

client: client.o
	$(CC) -o client client.o $(LIBS)

clean:
	rm -f $(ODIR)/*.o
	rm server client
