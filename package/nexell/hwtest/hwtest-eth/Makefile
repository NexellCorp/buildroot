VERSION_STR="\"0.2\""

#CFLAGS=-DVERSION_STR=$(VERSION_STR) -g 
CFLAGS=-DVERSION_STR=$(VERSION_STR) -static
#LDFLAGS=-lpthread
LDFLAGS=
C_OBJECTS=client.o 
S_OBJECTS=server.o

all: client server
 
client: $(C_OBJECTS) 
	$(CC) -o $@ $(C_OBJECTS) $(LDFLAGS) 

server: $(S_OBJECTS)
	$(CC) -o $@ $(S_OBJECTS) 

clean: 
	rm -f $(C_OBJECTS) client
	rm -f $(S_OBJECTS) server
