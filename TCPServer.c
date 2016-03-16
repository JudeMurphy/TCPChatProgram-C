//
//  main.c
//  TCPServer-Assignment1
//
//  Created by Jude Murphy on 2/20/16.
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
#include <err.h>
#include <string.h>
#include <pthread.h>

#define MAXNUMBEROFACTIVECHATSESSIONS 100

void toString(char str[], int num);

struct Host
{
    int port;
    int identifier;
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
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_len = sizeof(client_addr);
    
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0)
    {
        printf("Cannot open Socket.\n");
        return 0;
    }
    
    int port = 5000;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if(bind(fd, (struct sockaddr*)& server_addr, sizeof(server_addr)) < 0)
    {
        printf("Cannot bind socket.\n");
        return 0;
    }
    
    printf("***** SERVER STARTED *****\n");
    
    //ALLOCATE MEMORY TO OPEN THE ACTIVE CHAT SESSIONS
    struct Host *activeChatSessions[MAXNUMBEROFACTIVECHATSESSIONS];
    for (int i = 0; i < MAXNUMBEROFACTIVECHATSESSIONS; i++)
    {
        struct Host * placeHolder = malloc(sizeof(struct Host));
        placeHolder->identifier = 0;
        placeHolder->port = 0;
        
        activeChatSessions[i] = placeHolder;
    }
    
    listen(fd, 5);
        
    //WAIT FOR A CONNECTION AND HANDLE IT APPROPRIATELY
    while(1)
    {
        //RECEIVED CONNECTION FROM CLIENT OR HOST
        int client_fd = accept(fd, (struct sockaddr*) &client_addr, &sin_len);
        printf("HOST|CLIENT CONNECTED\n");
        
        //MAKES SURE CONNECTION IS ACCEPTED AND WORKING
        if(client_fd == -1)
        {
            printf("Cannot accept connection.\n");
            return 0;
        }
        //CONNECTION WORKED CORRECTLY
        else
        {
            //WAIT FOR INFORMATION FROM HOST OR CLIENT
            char messageFromHost[1024];
            int waitingForInformationFromClient = 0;
            while(waitingForInformationFromClient == 0)
            {
                //RECEIVES INFORMATION FROM HOST
                int numBytesRead = (int)read(client_fd, messageFromHost, 1024);
                if (numBytesRead > 0)
                {
                    //SETS END OF BUFFER
                    messageFromHost[numBytesRead] = 0;
                    
                    //DETERMINE IF THE REQUEST IS COMING FROM A CLIENT OR HOST
                    if (messageFromHost[0] == 'H')
                    {
                        //GETS HOST PORT FROM HOST MESSAGE
                        char hostPort[5] = {messageFromHost[1], messageFromHost[2], messageFromHost[3], messageFromHost[4]};
                        int hostPortInt = 0;
                        sscanf(hostPort, "%d", &hostPortInt);
                        
                        //GETS HOSTS IDENTIFIER FROM HOST MESSAGE
                        char hostUniqueIdentifier[7] = {messageFromHost[5], messageFromHost[6], messageFromHost[7], messageFromHost[8], messageFromHost[9], messageFromHost[10], messageFromHost[11]};
                        int hostUniqueIdentifierInt = 0;
                        sscanf(hostUniqueIdentifier, "%d", &hostUniqueIdentifierInt);
                        
                        //SETS PORT AND UNIQUE IDENTIFIER IN A HOST OBJECT
                        struct Host * host = malloc(sizeof(struct Host));
                        host->identifier = hostUniqueIdentifierInt;
                        host->port = hostPortInt;
                        
                        //CHECK TO SEE IF PORT ALREADY EXISTS OR HOST IDENTIFIER ALREADY EXISTS
                        int chatSessionIndex = 0;
                        int hostNotFound = 0;
                        while (chatSessionIndex < MAXNUMBEROFACTIVECHATSESSIONS)
                        {
                            if (activeChatSessions[chatSessionIndex]->port == host->port || host->port == 5000)
                            {
                                hostNotFound = 1;
                                chatSessionIndex = MAXNUMBEROFACTIVECHATSESSIONS;
                                
                                write_sock(client_fd, "Port Already In Use\n");
                            }
                            else if(activeChatSessions[chatSessionIndex]->identifier == host->identifier)
                            {
                                hostNotFound = 1;
                                chatSessionIndex = MAXNUMBEROFACTIVECHATSESSIONS;
                                
                                write_sock(client_fd, "Unique Identifier Already In Use\n");
                            }
                            
                            chatSessionIndex++;
                        }
                        
                        if (hostNotFound == 0)
                        {
                            //SEARCHES THROUGH ACTIVE CHAT SESSIONS AND STORES NEW SESSION IN LOCAL SERVER
                            int index = 0;
                            int exceededMaxNumberOfSessions = 0;
                            while (index < MAXNUMBEROFACTIVECHATSESSIONS)
                            {
                                if (activeChatSessions[index]->port == 0 && activeChatSessions[index]->identifier == 0)
                                {
                                    exceededMaxNumberOfSessions = 1;
                                    activeChatSessions[index] = host;
                                    char * messageToHost = "Successfully Added Chat Session\n";
                                    write_sock(client_fd, messageToHost);
                                    
                                    index = MAXNUMBEROFACTIVECHATSESSIONS;
                                }
                                
                                index++;
                            }
                            
                            if (exceededMaxNumberOfSessions == 0)
                            {
                                write_sock(client_fd, "Too Many Active Sessions | Please Wait Until Chats Are Finished - Closing Connection\n");
                            }
                        }
                    }
                    //REQUEST TO DELETE CHAT FROM LIST OF AVAILABLE CHATROOMS
                    else if(messageFromHost[0] == 'D')
                    {
                        //GETS HOST PORT FROM HOST MESSAGE
                        char hostPort[5] = {messageFromHost[1], messageFromHost[2], messageFromHost[3], messageFromHost[4]};
                        int hostPortInt = 0;
                        sscanf(hostPort, "%d", &hostPortInt);
                        
                        //GETS HOSTS IDENTIFIER FROM HOST MESSAGE
                        char hostUniqueIdentifier[7] = {messageFromHost[5], messageFromHost[6], messageFromHost[7], messageFromHost[8], messageFromHost[9], messageFromHost[10], messageFromHost[11]};
                        int hostUniqueIdentifierInt = 0;
                        sscanf(hostUniqueIdentifier, "%d", &hostUniqueIdentifierInt);
                        
                        //SETS PORT AND UNIQUE IDENTIFIER IN A HOST OBJECT
                        struct Host * host = malloc(sizeof(struct Host));
                        host->identifier = hostUniqueIdentifierInt;
                        host->port = hostPortInt;
                        
                        int chatSessionIndex = 0;
                        int chatSessionNotFound = 0;
                        while (chatSessionIndex < MAXNUMBEROFACTIVECHATSESSIONS)
                        {
                            if (activeChatSessions[chatSessionIndex]->port == host->port && activeChatSessions[chatSessionIndex]->identifier == host->identifier)
                            {
                                chatSessionNotFound = 1;
                                activeChatSessions[chatSessionIndex]->port = 0;
                                activeChatSessions[chatSessionIndex]->identifier = 0;
                                chatSessionIndex = MAXNUMBEROFACTIVECHATSESSIONS;
                            }
                            
                            chatSessionIndex++;
                        }
                        
                        if (chatSessionNotFound == 1)
                        {
                            write_sock(client_fd, "Deleted Active Chat Session From List\n");
                        }
                        else
                        {
                            write_sock(client_fd, "Deleted Active Chat Session From List\n");
                        }
                    }
                    //REQUEST CAME FROM A CLIENT - SEND LIST OF AVAILABLE CHATROOMS
                    else if (messageFromHost[0] == 'C')
                    {
                        int numberOfActiveChats = 0;
                        
                        //write_sock(client_fd, "*** CHOOSE FROM THE AVAILABLE CHAT SESSION PORTS BELOW ***\n");
                        //write_sock(client_fd, "*** TYPE YOUR SELECTED PORT AND HIT ENTER ***\n");
                        char activeSessionsForClientString[2048];
                        strcat(activeSessionsForClientString, "CHOOSE FROM THE AVAILABLE CHAT SESSION PORTS: ");
                        for (int i = 0; i < MAXNUMBEROFACTIVECHATSESSIONS; i++)
                        {
                            struct Host * host = activeChatSessions[i];
                            if (host->port != 0 && host->identifier != 0 && host != NULL)
                            {
                                char portNumber[6];
                                numberOfActiveChats++;
                                toString(portNumber, host->port);
                                strcat(portNumber, " ");
                                strcat(activeSessionsForClientString, portNumber);
                            }
                        }
                        
                        if (numberOfActiveChats == 0)
                        {
                            write_sock(client_fd, "*** THERE ARE NO ACTIVE CHATS OPEN RIGHT NOW ***\n");
                        }
                        else
                        {
                            write_sock(client_fd, activeSessionsForClientString);
                            memset(activeSessionsForClientString, 0, sizeof activeSessionsForClientString);
                        }
                    }
                    else
                    {
                        write_sock(client_fd, "Incorrect Message Format Sent To Server - Closing Connection\n");
                    }
                    
                    waitingForInformationFromClient = 1;
                }
            }
            
            printf("HOST|CLIENT DISCONNECTED\n");
            
            close(client_fd);
        }
    }
    
    return 0;
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