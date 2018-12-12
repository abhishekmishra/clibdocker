CC=gcc
CFLAGS=-std=c11 `pkg-config --cflags json-c` `pkg-config --cflags libcurl`
LIBS=`pkg-config --libs json-c` `pkg-config --libs libcurl`
LDFLAGS=-fPIC -g -shared

all:		clibdocker

clibdocker:	main.o docker_connection_util.so docker_containers.so
			$(CC) $(CFLAGS) -o clibdocker main.o docker_connection_util.so docker_containers.so $(LIBS)

main.o: docker_connection_util.so docker_containers.so main.c
			$(CC) $(CFLAGS) -c -o main.o main.c docker_connection_util.so docker_containers.so $(LIBS)
			
docker_containers.so: docker_connection_util.so docker_containers.h docker_containers.c
			$(CC) $(CFLAGS) -c -o docker_containers.so docker_connection_util.so docker_containers.c $(LIBS) $(LDFLAGS)
			
docker_connection_util.so: docker_connection_util.c docker_connection_util.h
			$(CC) $(CFLAGS) -c -o docker_connection_util.so docker_connection_util.c $(LIBS) $(LDFLAGS)

clean:
			rm *.o clibdocker