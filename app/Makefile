CC=/home/lmi/workspace/nano/buildroot/output/host/usr/bin/aarch64-linux-gnu-gcc
CFLAGS=-Wall -std=gnu11
OBJ=app.o

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

app: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm app *.o
