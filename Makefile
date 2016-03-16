ChatProgram: TCPServer.c
	gcc -std=c99 -lpthread -o TCPServer.out TCPServer.c
	gcc -std=c99 -lpthread -o TCPHost.out TCPHost.c
	gcc -std=c99 -lpthread -o TCPClient.out TCPClient.c