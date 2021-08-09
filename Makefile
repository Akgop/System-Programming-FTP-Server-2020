CC = gcc
all: cli srv

cli: client.c
	$(CC) -o $@ $<

srv: server.c
	$(CC) -o $@ $<

clean:
	rm -rf $(all)

