// client.h
#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 1024
#define MAX_DATA 1024

typedef struct{
    int ss_id;
    char ip_address[25];
    int nm_port;
    int client_port;
    char Base_path[1024];
    int status;
}StorageServer;

typedef struct{
    char Type[100];
    char Path[100];
    char Dest_path[100];
    int CliSock;
}InputDetails;

#endif // CLIENT_H 