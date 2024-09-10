#ifndef STORAGE_SERVER_H
#define STORAGE_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <dirent.h>


#define MAX_PATH_LENGTH 100
#define MAX_BUFFER_SIZE 1024

pthread_mutex_t write_lock;

typedef struct{
    int nm_port;
    int ss_port;
    int No_of_Paths;
    int rep_port;
    char ip_address[25];
    char current_path[1024];
    char accessablepaths[MAX_BUFFER_SIZE];
}StorageServer;

typedef struct{
    char Type[100];
    char Path[100];
    char Dest_path[100];
    int* CliSock;
}InputDetails;

#endif // STORAGE_SERVER_Hhello osn course is best
