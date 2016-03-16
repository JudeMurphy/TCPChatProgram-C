//
//  main.c
//  TCPHost-Assignment1
//
//  Created by Murphy, Jude {BIS} on 2/20/16.
//  Copyright © 2016 Iona College. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <string.h>
#include <time.h>

#define MAX_CLIENTS 100

int clients[MAX_CLIENTS];
char *randstring(int length);

const char * myPort;
char myUniqueID[8];
int serverSocket = 0;
int hostShouldBeKilled = 0;
void * client_thread(void * arg);
void toString(char str[], int num);
unsigned int rand_interval(unsigned int min, unsigned int max);

struct Client
{
    char clientName[100];
    int clientSocket;
};

static void write_sock(int sock, const char *msg)
{
    int len = (int)strlen(msg);
    if (write(sock, msg, len) != len)
    {
        perror("short write on socket");
        exit(1);
    }
}

int main(int argc, const char * argv[])
{
    if (argv[1] == 0)
    {
        printf("Please enter a port number after Host.\nExample: ./Launch Host 5001\n\n");
        return 0;
    }
    
    printf("***** HOST STARTED *****");
    
    srand((int)time(NULL));
    
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket < 0)
    {
        printf("Cannot open socket.\n");
        return 0;
    }
    
    server = gethostbyname("127.0.0.1");
    
    if(server == NULL){
        printf("Cannot find server.\n");
        return 0;
    }
    
    bzero( (char*) &serv_addr, sizeof(serv_addr) );
    serv_addr.sin_family = AF_INET;
    bcopy( (char*)server->h_addr_list[0], (char*)&serv_addr.sin_addr.s_addr, server->h_length);
    
    serv_addr.sin_port = htons(5000);
    if(connect(serverSocket, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Cannot connect.\n");
        return 0;
    }
    
    int uniqueIdentifier = rand_interval(1000000, 9999999);
    
    //CRAFT MESSAGE TO SEND TO SERVER
    char * messageType = "H";
    myPort = argv[1];
    toString(myUniqueID, uniqueIdentifier);
    
    char hostString[13];
    strcat(hostString, messageType);
    strcat(hostString, myPort);
    strcat(hostString, myUniqueID);
    
    //SEND INITIAL MESSAGE TO SERVER STATING THE HOSTS ITEMS
    write_sock(serverSocket, hostString);
    
    //WAIT FOR MESSAGE BACK FROM SERVER
    char initialMessageFromServer[1024];
    int receivedInitialMessageBackFromServer = 0;
    while(receivedInitialMessageBackFromServer == 0)
    {
        int numBytesRead = (int)read(serverSocket, initialMessageFromServer, 1024);
        if(numBytesRead > 0)
        {
            receivedInitialMessageBackFromServer = 1;
            initialMessageFromServer[numBytesRead] = 0;
        }
    }
    
    //IF ERROR RECEIVED BACK FROM SERVER | SHOW ERROR
    if (initialMessageFromServer[0] == 'P' || initialMessageFromServer[0] == 'U' || initialMessageFromServer[0] == 'T')
    {
        close(serverSocket);
        printf("ERROR RECEIVED FROM SERVER: %s", initialMessageFromServer);
    }
    //SERVER HAS RECORD OF HOST CHAT
    else
    {
        close(serverSocket);
        
        struct sockaddr_in server_addr, client_addr;
        socklen_t sin_len = sizeof(client_addr);
        
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if(fd < 0)
        {
            printf("Cannot open Socket.\n");
            return 0;
        }
        
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(atoi(myPort));
        
        if(bind(fd, (struct sockaddr*)& server_addr, sizeof(server_addr)) < 0)
        {
            printf("Cannot bind socket.\n");
            return 0;
        }
        
        //FILL ARRAY WITH ZEROS
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            clients[i] = 0;
        }
        
        listen(fd, 5);
        while(1)
        {
            int client_fd = accept(fd, (struct sockaddr *) &client_addr, &sin_len);
            printf("Client connected.\n");
            
            if(client_fd == -1)
            {
                printf("Cannot accept connection.\n");
                return 0;
            }
            
            int newClientFound = 0;
            for (int n = 0; n < MAX_CLIENTS; n++)
            {
                if (clients[n] == 0)
                {
                    clients[n] = client_fd;
                    newClientFound = 1;
                    n = MAX_CLIENTS;
                }
            }
            
            if (newClientFound == 1)
            {
                int *arg = malloc(sizeof(int));
                *arg = client_fd;
                
                struct Client *newClient = malloc(sizeof(struct Client));
                newClient->clientSocket = client_fd;
                
                pthread_t t;
                pthread_create(&t, NULL, client_thread, newClient);
            }
            else
            {
                write_sock(client_fd, "THIS CHAT HAS THE MAX AMOUNT OF CLIENTS - PLEASE WAIT UNTIL SOME CLIENTS LEAVE\n");
                close(client_fd);
                printf("Client disconnected!\n");
            }
        }
    }
    
    return 0;
}

void * client_thread(void * arg)
{
    struct Client * newClient = (struct Client*)arg;
    int clientSocket = newClient->clientSocket;
    
    if(!arg || clientSocket == 0)
        return NULL;
    
    write_sock(clientSocket, "\n********** WELCOME TO MURPH CHAT **********\n-TYPE QUIT TO LEAVE THIS CHAT\n-TYPE KILLHOST TO CUT ALL CLIENT CONNECTIONS\n-START TYPING ANYTHING NOW (KEEP IT PG)\n\n");
    
    char clientMessage[1024];
    while(1)
    {
        int n = (int)read(clientSocket, clientMessage, 1024);
        
        //KILLS THE HOST PROCESS
        if(clientMessage[0] == 'K' && clientMessage[1] == 'I' && clientMessage[2] == 'L' && clientMessage[3] == 'L' && clientMessage[4] == 'H' && clientMessage[5] == 'O' && clientMessage[6] == 'S' && clientMessage[7] == 'T')
        {
            //ERROR READING FROM SOCKET
            if(n < 0)
            {
                printf("Error reading from socket.\n");
                close(clientSocket);
                break;
            }
            
            char disconnectedMessage [1024] = "*** HOST HAS BEEN DISCONNECTED ***\n";
            
            write_sock(clientSocket, disconnectedMessage);
            for(int i=0; i < MAX_CLIENTS; ++i)
            {
                if(clients[i] == clientSocket && clients[i] != 0)
                    continue;
                
                write(clients[i], disconnectedMessage, strlen(disconnectedMessage));
            }
            
            struct sockaddr_in serv_addr;
            struct hostent *server;
            
            serverSocket = socket(AF_INET, SOCK_STREAM, 0);
            if(serverSocket < 0)
            {
                printf("Cannot open socket.\n");
                return 0;
            }
            
            server = gethostbyname("127.0.0.1");
            
            if(server == NULL)
            {
                printf("Cannot find server.\n");
                return 0;
            }
            
            bzero( (char*) &serv_addr, sizeof(serv_addr) );
            serv_addr.sin_family = AF_INET;
            bcopy( (char*)server->h_addr_list[0], (char*)&serv_addr.sin_addr.s_addr, server->h_length);
            
            serv_addr.sin_port = htons(5000);
            
            if(connect(serverSocket, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
            {
                printf("Cannot connect to server.\n");
                return 0;
            }
            
            char * messageType = "D";
            char messageToServer[13];
            strcat(messageToServer, messageType);
            strcat(messageToServer, myPort);
            strcat(messageToServer, myUniqueID);
            
            write_sock(serverSocket, messageToServer);
            
            close(serverSocket);
            exit(0);
        }
        else if(clientMessage[0] == 'Q' && clientMessage[1] == 'U' && clientMessage[2] == 'I' && clientMessage[3] == 'T')
        {
            for(int i=0; i < MAX_CLIENTS; ++i)
            {
                if(clients[i] == clientSocket)
                    continue;
                
                if (clients[i] != 0)
                {
                    write(clients[i], clientMessage, n);
                }
            }
            
            for(int i = 0; i < MAX_CLIENTS; ++i)
            {
                if(clients[i] == clientSocket)
                {
                    clients[i] = 0;
                }
            }
        }
        
        //ERROR READING FROM SOCKET
        else if(n < 0)
        {
            printf("Error reading from socket.\n");
            close(clientSocket);
            break;
        }
        else
        {
            for(int i=0; i < MAX_CLIENTS; ++i)
            {
                if(clients[i] == clientSocket)
                    continue;
                
                if (clients[i] != 0)
                {
                    write(clients[i], clientMessage, n);
                }
            }
        }
    }
    
    return NULL;
}

unsigned int rand_interval(unsigned int min, unsigned int max)
{
    int r;
    const unsigned int range = 1 + max - min;
    const unsigned int buckets = RAND_MAX / range;
    const unsigned int limit = buckets * range;
    
    do
    {
        r = rand();
    } while (r >= limit);
    
    return min + (r / buckets);
}

void toString(char str[], int num)
{
    int i, rem, len = 0, n;
    
    n = num;
    while (n != 0)
    {
        len++;
        n /= 10;
    }
    for (i = 0; i < len; i++)
    {
        rem = num % 10;
        num = num / 10;
        str[len - (i + 1)] = rem + '0';
    }
    str[len] = '\0';
}