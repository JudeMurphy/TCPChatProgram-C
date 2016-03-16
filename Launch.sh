#!/bin/bash

if [ "$1" = "Server" ]
then
    open -a Terminal ./TCPServer.out
fi

if [ "$1" = "Host" ]
then
    open -a Terminal ./TCPHost.out --args "$2"
fi

if [ "$1" = "Client" ]
then
    open -a Terminal ./TCPClient.out
fi