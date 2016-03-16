//
//  main.c
//  TCPClient-Assignment1
//
//  Created by Murphy, Jude {BIS} on 2/23/16.
//  Copyright © 2016 Iona College. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <netdb.h>
#include <time.h>

int hostSocket;
int clientIsConnected;

static void write_sock(int sock, const char *msg)
{
    int len = (int)strlen(msg);
    if (write(sock, msg, len) != len)
    {
        perror("short write on socket");
        exit(1);
    }
}

struct Client
{
    char clientName[100];
    int clientSocket;
};

void * sendtoHost(void * arg)
{
    struct Client * newClient = (struct Client*) arg;
    int clientSocket = newClient->clientSocket;
    
    if(!arg || clientSocket == 0)
        return NULL;
    
    int initialJoin = 0;
    
    char buffer[1024];
    while(1)
    {
        fgets(buffer, 1024, stdin);
        
        char message [2048];
        if (initialJoin == 0)
        {
            char joinedMessage [2048];
            strcat(joinedMessage, "--- ");
            strcat(joinedMessage, newClient->clientName);
            strcat(joinedMessage, " has joined the chat ---\n");
            if(write(hostSocket, joinedMessage, strlen(joinedMessage) + 1) > 0)
            {
                strcpy(joinedMessage, "");
            }
        }
        else
        {
            strcat(message, newClient->clientName);
            strcat(message, ": ");
            strcat(message, buffer);
        }
        
        //QUITS CLIENT
        if(buffer[0] == 'Q' && buffer[1] == 'U' && buffer[2] == 'I' && buffer[3] == 'T')
        {
            char leaveMessage [2048];
            strcat(leaveMessage, "--- ");
            strcat(leaveMessage, newClient->clientName);
            strcat(leaveMessage, " has left the chat ---\n");
            if(write(hostSocket, leaveMessage, strlen(leaveMessage) + 1) > 0)
            {
                strcpy(leaveMessage, "");
                exit(1);
            }
        }
        //KILLS THE HOST PROCESS AND KILLS ALL CLIENTS ON HOST SIDE
        else if(buffer[0] == 'K' && buffer[1] == 'I' && buffer[2] == 'L' && buffer[3] == 'L' && buffer[4] == 'H' && buffer[5] == 'O' && buffer[6] == 'S' && buffer[7] == 'T')
        {
            if(write(hostSocket, buffer, strlen(buffer) + 1) > 0)
            {
                exit(1);
            }
        }
        else if(write(hostSocket, message, strlen(message) + 1) < 0)
        {
            printf("Error writing to socket.\n");
            return NULL;
        }
        else
        {
            initialJoin = 1;
        }
        
        strcpy(buffer, "");
        strcpy(message, "");
    }
    
    return NULL;
}

void *recvFromHost(void * data)
{
    char buffer[1024];
    while(1)
    {
        int numBytesRead = (int)read(hostSocket, buffer, 1024);
        if(numBytesRead < 0)
        {
            printf("Error reading from socket.\n");
            return NULL;
        }
        
        buffer[numBytesRead] = 0;
        printf("%s", buffer);
    }
}

int main(int argc, const char * argv[])
{
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
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
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server -> h_addr_list[0], (char *) &serv_addr.sin_addr.s_addr, server->h_length);
    
    serv_addr.sin_port = htons(5000);
    
    if(connect(serverSocket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Cannot connect.\n");
        return 0;
    }
    
    //SEND INITIAL MESSAGE TO SERVER STATING THE HOSTS ITEMS
    write_sock(serverSocket, "C");
    
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
            printf("%s\n", initialMessageFromServer);
        }
    }
    
    close(serverSocket);
    
    //GET MESSAGE FROM USER
    char portNumber[1024];
    printf("ENTER THE PORT NUMBER YOU WANT TO JOIN: ");
    scanf("%s", portNumber);
    
    char clientName[100];
    printf("ENTER YOUR USERNAME: ");
    scanf("%s", clientName);
    
    hostSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(hostSocket < 0)
    {
        printf("Cannot open socket.\n");
        return 0;
    }
    
    struct sockaddr_in host_addr;
    struct hostent *hostHost;
    hostHost = gethostbyname("127.0.0.1");
    
    if(hostHost == NULL)
    {
        printf("Cannot find server.\n");
        return 0;
    }
    
    bzero((char *) &host_addr, sizeof(host_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)hostHost->h_addr_list[0], (char*)&host_addr.sin_addr.s_addr, hostHost->h_length);
    
    host_addr.sin_port = htons(atoi(portNumber));
    
    if(connect(hostSocket, (struct sockaddr*) &host_addr, sizeof(host_addr)) < 0)
    {
        printf("Cannot connect.\n");
        return 0;
    }
    else
    {
        int *arg = malloc(sizeof(int));
        *arg = hostSocket;

        struct Client *newClient = malloc(sizeof(struct Client));
        newClient->clientSocket = hostSocket;
        strcpy(newClient->clientName, clientName);

        pthread_t recvThread, sendThread;
        
        pthread_create(&sendThread, NULL, sendtoHost, newClient);
        pthread_create(&recvThread, NULL, recvFromHost, NULL);
        
        pthread_join(sendThread, NULL);
        pthread_join(recvThread, NULL);
    }

    
    return 0;
}
