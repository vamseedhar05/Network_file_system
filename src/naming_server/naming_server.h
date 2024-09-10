// naming_server.h
#ifndef NAMING_SERVER_H
#define NAMING_SERVER_H
#define MAX_SS 100
#define LRU_SIZE 10
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

typedef struct ss_details{
    int ss_id;
    char ip_address[25];
    int nm_port;
    int client_port;
    char Base_path[1024];
    int status;
    int Rep1_status;
    int Rep2_status;
    char Rep_Base1_path[1024];
    char Rep_Base2_path[1024];
    int rep1_port;
    int rep2_port;
} SS_Details;

typedef struct ss_details_inc_path{
    int nm_port;
    int client_port;
    int total_no;
    int rep_port;
    char ip_address[25];
    char Base_path[1024];
    char accessible_paths[1024];
} SS_Details_inc_path;

typedef struct TrieNode {
    int id;
    struct TrieNode* children[26];
} TrieNode;
int serverPort;

typedef struct client_sent{
    char Type[100];
    char Path[100];
    char Dest_path[100];
    int* CliSock;
} Client_sent;

#endif