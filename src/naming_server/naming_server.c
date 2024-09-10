#include "naming_server.h"

// #define MAX_clients 2

#define LRU_SIZE 10

typedef struct all_access_path_trie *access_trieptr;

struct all_access_path_trie
{
    int SS_no;
    access_trieptr sub_letters[123];
};
typedef struct Node_info* Node;

struct Node_info
{
    int SS_no;
    char Path[1024];
    Node next;
    Node prev;
};

Node Head;
Node Tail;
int len;
int connectedClients = 0;
// pthread_mutex_t clientCounterMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_message_mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t count_clients = PTHREAD_MUTEX_INITIALIZER;




void logMessage(const char *message)
{
    pthread_mutex_lock(&log_message_mutex);
    FILE *logFile = fopen("../../logfile.txt", "a"); // Open log file in append mode
    if (logFile != NULL)
    {
        fprintf(logFile, "%s\n", message);
        fclose(logFile);
    }
    pthread_mutex_unlock(&log_message_mutex);
}


void Update(Node Current){
    Node Next=Current->next;
    Node Prev=Current->prev;
    if(Current==Tail && Prev != NULL){
        Tail=Prev;
    }
    if (Next != NULL) {
        Next->prev = Prev;
    }
    if (Prev != NULL) {
        Prev->next = Next;
    }
    else{
        Next->prev=NULL;
    }
    if(Next==NULL){
        Prev->next = NULL;
    }
    Current->prev=NULL;
    Current->next=Head;
    if (Head != NULL) {
        Head->prev = Current;
    }
    Head=Current;
}

int Search_in_LRU(char* path){
    Node temp=Head;
    for(int i=0; i<LRU_SIZE  && temp != NULL ;i++){
        if(strcmp(temp->Path,path)==0){
            if(temp!=Head)
                Update(temp);
            printf("Found in Cache\n");
            return temp->SS_no;
        }
        temp=temp->next;
    }
    return 0;
}

void Add_to_LRU(char* path, int SS_no){
    Node NewNode=(Node)malloc(sizeof(struct Node_info));
    NewNode->next=Head;
    NewNode->prev=NULL;
    strcpy(NewNode->Path,path);
    NewNode->SS_no=SS_no;
    if(Head==NULL){
        Tail=NewNode;
    }
    else{
        Head->prev=NewNode;
    }
    Head=NewNode;
    len++;
    if(len>LRU_SIZE){
        len=LRU_SIZE;
        Tail=Tail->prev;
        if(Tail!=NULL){
            free(Tail->next);
            Tail->next=NULL;
        }
    }
    return;
}

SS_Details SS_List[MAX_SS];
access_trieptr create_Node(void)
{
    access_trieptr new_node = NULL;
    new_node = (access_trieptr)malloc(sizeof(struct all_access_path_trie));
    if (new_node != NULL)
    {
        int indexe;
        new_node->SS_no = -1;
        for (int indexe = 0; indexe < 60; indexe++)
        {
            new_node->sub_letters[indexe] = NULL;
        }
    }
    return new_node;
}
access_trieptr Root;
int indexe;

void insert(access_trieptr Root, char *path, int ss_no)
{
    int len = strlen(path);
    int i, indexe;
    access_trieptr pres_ptr = Root;
    for (int i = 0; i < len; i++)
    {
        indexe = (int)(path[i]);
        if (pres_ptr->sub_letters[indexe] == NULL)
        {
            pres_ptr->sub_letters[indexe] = create_Node();
        }
        pres_ptr = pres_ptr->sub_letters[indexe];
    }
    pres_ptr->SS_no = ss_no;
}

access_trieptr copytrie(access_trieptr tocopy, int ss_no)
{
    if (tocopy == NULL)
    {
        return NULL;
    }
    access_trieptr new_node = NULL;
    new_node = (access_trieptr)malloc(sizeof(struct all_access_path_trie));
    if (new_node != NULL)
    {
        int indexe;
        new_node->SS_no = ss_no;
        for (int indexe = 0; indexe < 123; indexe++)
        {
            new_node->sub_letters[indexe] = copytrie(tocopy->sub_letters[indexe], ss_no);
        }
    }
    return new_node;
}

int search(access_trieptr Root, char *path)
{
    int IN_LRU = Search_in_LRU(path);
    if (IN_LRU)
        return IN_LRU;
    int len = strlen(path);
    int i, indexe;
    access_trieptr pres_ptr = Root;
    for (int i = 0; i < len; i++)
    {
        indexe = (int)(path[i]);
        if (pres_ptr->sub_letters[indexe] == NULL)
        {
            return -1;
        }
        pres_ptr = pres_ptr->sub_letters[indexe];
    }
    Add_to_LRU(path, pres_ptr->SS_no);
    return pres_ptr->SS_no;
}

int createServerSocket(const char *ip, int port)
{
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("[-]Socket error");
        exit(1);
    }
    printf("[+]TCP server socket created.\n");

    struct sockaddr_in server_addr;
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("[-]Bind error");
        exit(1);
    }
    printf("[+]Bind to the port number: %d\n", port);

    listen(server_sock, 5); // Listen for a single connection
    printf("Listening...\n");

    return server_sock;
}

int acceptClient(int server_sock)
{
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(client_addr);
    int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);
    if (client_sock < 0)
    {
        perror("[-]Accept error");
        exit(1);
    }

    
    printf("[+]Client connected.\n");

    return client_sock;
}   
int create_storagesocket(int *storageSocket, int port)
{
    char ip[] = "127.0.0.1";
    *storageSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (*storageSocket < 0)
    {
        perror("[-]Socket error");
        exit(1);
    }
    printf("[+]TCP server socket created.\n");

    struct sockaddr_in storage_sock;
    memset(&storage_sock, '\0', sizeof(storage_sock));
    storage_sock.sin_family = AF_INET;
    storage_sock.sin_port = htons(port);
    storage_sock.sin_addr.s_addr = inet_addr(ip);
    if (connect(*storageSocket, (struct sockaddr *)&storage_sock, sizeof(storage_sock)) < 0)
    {
        perror("Error connecting to Server");
        close(*storageSocket);
        return -1;
    }
    return 0;
}

int check_txtfile(const char *str)
{
    size_t len = strlen(str);
    if (strcmp(str + len - 1, "/"))
    {
        char *lastSlash = strrchr(str, '/');
        if (lastSlash != NULL)
        {
            *(lastSlash + 1) = '\0';
        }
    }
    else
    {
        char *lastSlash = strrchr(str, '/');
        if (lastSlash != NULL)
        {
            *(lastSlash) = '\0';
        }
        lastSlash = strrchr(str, '/');
        if (lastSlash != NULL)
        {
            *(lastSlash + 1) = '\0';
        }
        printf("%s", str);
        return 0; // String does not end with ".txt"
    }
}

void *client_thread_function(void *clientsock)
{
    int *clientsocker = (int *)clientsock;
    const int clientSocket = (int)*clientsocker;
    const char *serverIP = "127.0.0.1";
    Client_sent temp;
    size_t size_ss = sizeof(temp.Type) + sizeof(temp.Path) + sizeof(temp.Dest_path) + sizeof(temp.CliSock);
    bzero(&temp, size_ss);
       if(recv(clientSocket, &temp, size_ss, 0) == -1 || !strcmp(temp.Path,"") || !strcmp(temp.Type,"")){
        close(*clientsocker);      
        printf("[+]Client disconnected.\n\n");
        return NULL;
    }
    
    // if(!strcmp(temp.Type,"EXIT")){
    //     close(*clientsocker);
    //     printf("[+]Client disconnected.\n\n");
    //     return NULL;
    // }
    printf("data received %s\n",temp.Type);
    // Customize your response based on the received request

    int req_ss = -1;
    char pathtoverify[1024];
    strcpy(pathtoverify, temp.Path);

    // Log the received request
    char logMessageBuffer[2048];
    if(!strcmp(temp.Type,"COPY"))
        snprintf(logMessageBuffer, sizeof(logMessageBuffer), "Received request: Type=%s, Path=%s, Dest_path=%s", temp.Type,temp.Path, temp.Dest_path);
    else
        snprintf(logMessageBuffer, sizeof(logMessageBuffer), "Received request: Type=%s, Path=%s", temp.Type, temp.Path);
    logMessage(logMessageBuffer);

    if (!strcmp(temp.Type, "CREATE"))
    {
        check_txtfile(pathtoverify);
    }
    req_ss = search(Root, pathtoverify);

    char* start = strchr(temp.Path + 1, '/');
    strcpy(temp.Path, start+1);
    if(req_ss != -1)
        printf("The Selected SS is %d %s\n", req_ss, SS_List[req_ss - 1].Base_path);
    if (req_ss != -1)
    {
        int store_socket, store_socket2, store_socket3;
        int req_port = SS_List[req_ss - 1].client_port;
    again:
        int flag2 = -1;
        int flag3 = -1;
        int flag = create_storagesocket(&store_socket, req_port);
        if(SS_List[req_ss - 1].rep1_port != req_port){
         flag2 = create_storagesocket(&store_socket2, SS_List[req_ss - 1].rep1_port);
        }
        if(SS_List[req_ss - 1].rep1_port != req_port && SS_List[req_ss - 1].rep1_port != SS_List[req_ss - 1].rep2_port){
        int flag3 = create_storagesocket(&store_socket3, SS_List[req_ss - 1].rep2_port);
        }
        if (flag == 0)
        {

            if (strcmp("CREATE", temp.Type) == 0)
            {
                int selection = 1;
                send(store_socket, &selection, sizeof(selection), 0);
                if(flag2 == 0){
                    send(store_socket2, &selection, sizeof(selection), 0);
                }
                else{
                    SS_List[req_ss - 1].rep1_port = 0;
                }
                if(flag3 == 0){
                    send(store_socket3, &selection, sizeof(selection), 0);   
                }
                else{
                    SS_List[req_ss - 1].rep2_port = 0;
                }
                size_t size_pack = sizeof(temp.Type) + sizeof(temp.Path) + sizeof(temp.Dest_path) + sizeof(temp.CliSock);
                send(store_socket, &temp, size_pack, 0);
                if(flag2 == 0){
                send(store_socket2, &temp, size_pack, 0);
                }
                else{
                    SS_List[req_ss - 1].rep1_port = 0;
                }
                if(flag3 == 0){
                send(store_socket3, &temp, size_pack, 0);
                }
                else{
                    SS_List[req_ss - 1].rep2_port = 0;
                }
                int ack = 0;
                if(recv(store_socket, &ack, sizeof(ack), 0) == -1 ){
                    printf("Data not Recieved\n");
                    close(clientSocket);
                    return NULL;
                }
                char* lastSlash = strrchr(SS_List[req_ss-1].Base_path, '/');
                char tempo[1024] = {0};
                strcpy(tempo, lastSlash);
                strcat(tempo, "/");
                insert(Root, strcat(tempo, temp.Path), req_ss);
                if (ack == 1)
                {
                    int acko = 1;
                    send(clientSocket, &acko, sizeof(acko), 0);
                    printf("File Created Successfully\n");
                    snprintf(logMessageBuffer, sizeof(logMessageBuffer), "-----File %s Created Successfully-----", temp.Path);
                    logMessage(logMessageBuffer);
                    
                    close(clientSocket);
                }
                else
                {
                    int acko = 0;
                    send(clientSocket, &acko, sizeof(ack), 0);
                    printf("File Creation Failed\n");
                    snprintf(logMessageBuffer, sizeof(logMessageBuffer), "-----Failed to create file %s-----", temp.Path);
                    logMessage(logMessageBuffer);
                    
                    close(clientSocket);
                }
            }
            else if (strcmp("DELETE", temp.Type) == 0)
            {
                int selection = 1;
                send(store_socket, &selection, sizeof(selection), 0);
                if(flag2 == 0){
                    send(store_socket2, &selection, sizeof(selection), 0);
                }
                if(flag3 == 0){
                    send(store_socket3, &selection, sizeof(selection), 0);   
                }
                size_t size_pack = sizeof(temp.Type) + sizeof(temp.Path) + sizeof(temp.Dest_path) + sizeof(temp.CliSock);
                send(store_socket, &temp, size_pack, 0);
                send(store_socket2, &temp, size_pack, 0);
                send(store_socket3, &temp, size_pack, 0);

                int ack = 0;
                recv(store_socket, &ack, sizeof(ack), 0);
                char* lastSlash = strrchr(SS_List[req_ss-1].Base_path, '/');
                char tempo[1024] = {0};
                strcpy(tempo, lastSlash);
                insert(Root, strcat(tempo, temp.Path), -1);
                if (ack == 1)
                {
                    printf("File Deleted Successfully\n");
                    snprintf(logMessageBuffer, sizeof(logMessageBuffer), "-----File %s Deleted Successfully-----", temp.Path);
                    logMessage(logMessageBuffer);
                }
                else
                {
                    printf("File Deletion Failed\n");
                    snprintf(logMessageBuffer, sizeof(logMessageBuffer), "-----Failed to delete file %s-----", temp.Path);
                    logMessage(logMessageBuffer);
                }
                send(clientSocket, &ack, sizeof(ack), 0);
                
                close(clientSocket);
            }
            else if (strcmp("COPY", temp.Type) == 0)
            {
                int selection = 1;
                send(store_socket, &selection, sizeof(selection), 0);
                // if(flag2 == 0){
                //     send(store_socket2, &selection, sizeof(selection), 0);
                // }
                // if(flag3 == 0){
                //     send(store_socket3, &selection, sizeof(selection), 0);   
                // }
                char* start = strchr(temp.Dest_path + 1, '/');
                strcpy(temp.Dest_path, start+1);
                size_t size_pack = sizeof(temp.Type) + sizeof(temp.Path) + sizeof(temp.Dest_path) + sizeof(temp.CliSock);
                // send(store_socket, &temp, size_pack, 0);
                // send(store_socket2, &temp, size_pack, 0);
                // send(store_socket3, &temp, size_pack, 0);

                int ack = 0;
                recv(store_socket, &ack, sizeof(ack), 0);
                if (ack == 1)
                {
                    printf("File/Folder Copy Successfully\n");
                    snprintf(logMessageBuffer, sizeof(logMessageBuffer), "-----File %s Copy Successfully-----", temp.Path);
                    logMessage(logMessageBuffer);
                }
                else
                {
                    printf("File copy Failed\n");
                    snprintf(logMessageBuffer, sizeof(logMessageBuffer), "-----Failed to Copy file %s-----", temp.Path);
                    logMessage(logMessageBuffer);
                }
                send(clientSocket, &ack, sizeof(ack), 0);
                
                close(clientSocket);
            }
            else
            {
                int selection = 3;
                send(store_socket, &selection, sizeof(selection), 0);
                int temp_port = SS_List[req_ss - 1].client_port;
                size_t size_ss_mini = sizeof(SS_List[req_ss - 1].Base_path) + sizeof(SS_List[req_ss - 1].client_port) + sizeof(SS_List[req_ss - 1].ip_address) + sizeof(SS_List[req_ss - 1].nm_port) + sizeof(SS_List[req_ss - 1].ss_id) + sizeof(SS_List[req_ss - 1].status)+sizeof(SS_List[req_ss - 1].Rep1_status)+sizeof(SS_List[req_ss - 1].Rep2_status)+sizeof(SS_List[req_ss - 1].rep1_port)+sizeof(SS_List[req_ss - 1].rep2_port) + sizeof(SS_List[req_ss - 1].Rep_Base1_path)+sizeof(SS_List[req_ss - 1].Rep_Base2_path);
                SS_List[req_ss - 1].client_port = req_port;
                send(clientSocket, &SS_List[req_ss - 1], size_ss_mini, 0);
                SS_List[req_ss - 1].client_port = temp_port;
                int ack = -1;
                recv(clientSocket,&ack,sizeof(ack),0);
                if(ack){ 
                    printf("Operation Successfull\n");
                    snprintf(logMessageBuffer, sizeof(logMessageBuffer), "-----File %s has been %s Successfully-----", temp.Path,temp.Type);
                    logMessage(logMessageBuffer);
                }
                else{
                    printf("Operation Failed\n");
                    snprintf(logMessageBuffer, sizeof(logMessageBuffer), "-----Failed to %s file %s-----",temp.Type,temp.Path);
                    logMessage(logMessageBuffer);
                }
                close(clientSocket);
            }
        close(store_socket);
        close(store_socket2);
        close(store_socket3);
        }
        else if(req_ss != -1 && (SS_List[req_ss - 1].Rep1_status == 1 || SS_List[req_ss - 1].Rep2_status == 1)){
            close(store_socket);
            close(store_socket2);
            close(store_socket3);
            req_port = SS_List[req_ss - 1].rep1_port;

            if(req_port == 0){
                req_port = SS_List[req_ss - 1].rep2_port;
            }
            snprintf(logMessageBuffer, sizeof(logMessageBuffer), "Storage Server with Port :%d is disconnected and connected to Replicated SS with Port : %d",SS_List[req_ss-1].client_port,req_port);
            
            if(strcmp("READ", temp.Type) == 0){
                goto again;
            }
        }
        else
        {
            snprintf(logMessageBuffer, sizeof(logMessageBuffer), "Storage Server with Port :%d is disconnected",SS_List[req_ss-1].client_port);
            logMessage(logMessageBuffer);
            printf("The required SS is not found\n");
            SS_List[req_ss - 1].status = 0;
            SS_Details err_msg;
            err_msg.ss_id = -4;
            size_t size_ss_mini = sizeof(err_msg.Base_path) + sizeof(err_msg.client_port) + sizeof(err_msg.ip_address) + sizeof(err_msg.nm_port) + sizeof(err_msg.ss_id) + sizeof(err_msg.status);
            send(clientSocket, &err_msg, size_ss_mini, 0);
            // close(clientSocket);
        close(store_socket);
        close(store_socket2);
        close(store_socket3);
        }
    }
    else
    {
        printf("The required SS is not found\n");
        SS_Details err_msg;
        err_msg.ss_id = -1;
        size_t size_ss_mini = sizeof(err_msg.Base_path) + sizeof(err_msg.client_port) + sizeof(err_msg.ip_address) + sizeof(err_msg.nm_port) + sizeof(err_msg.ss_id) + sizeof(err_msg.status);
        send(clientSocket, &err_msg, size_ss_mini, 0);
        // close(clientSocket);
    }

    // snprintf(logMessageBuffer, sizeof(logMessageBuffer), "Processed request: Type=%s, Path=%s, Dest_path=%s", temp.Type, temp.Path, temp.Dest_path);
    // logMessage(logMessageBuffer);
    

    printf("[+]Client disconnected.\n\n");
}

void *storage_server_thread(void *clientsock)
{
    int *clientsocker = (int *)clientsock;
    const int clientSocket = (int)*clientsocker;
    const char *serverIP = "127.0.0.1";
    SS_Details_inc_path temp;
    size_t size_ss = sizeof(temp.Base_path) + sizeof(temp.client_port) + sizeof(temp.ip_address) + sizeof(temp.nm_port) + sizeof(temp.accessible_paths) + sizeof(temp.total_no) + sizeof(temp.rep_port);
    bzero(&temp, size_ss);
        if(recv(clientSocket, &temp, size_ss, 0) == -1 || temp.nm_port == 0){
        close(*clientsocker);
        printf("[+]Client disconnected.\n\n");
        return NULL;
    }
    int flago = 0;
    int require_index = indexe;
    for (int i = 0; i < indexe; i++)
    {
        if(SS_List[i].client_port == temp.client_port &&  temp.rep_port == 0){
            SS_List[i].status = 1;
            require_index = i;
            flago = 1;
            int selection = 6;
            int store_socket;
            int flag = create_storagesocket(&store_socket, SS_List[i - 1].client_port);
            send(store_socket, &selection, sizeof(selection), 0);
        }
        else if(SS_List[i].client_port == temp.client_port && SS_List[i].Rep1_status == 1){
            printf("Storage Server with Port :%d is already connected.\n So using this as replica 2\n",temp.client_port);
            flago = 1;
            require_index = i;
            strcpy(SS_List[i].Rep_Base2_path,temp.Base_path);
            SS_List[i].Rep2_status = 1;
            SS_List[i].rep2_port = temp.rep_port;
            break;
        }
        else if(SS_List[i].client_port == temp.client_port){
            printf("Storage Server with Port :%d is already connected.\n So using this as replica 1\n",temp.client_port);
            flago = 1;
            require_index = i;
            strcpy(SS_List[i].Rep_Base1_path,temp.Base_path);
            SS_List[i].Rep1_status = 1;
            SS_List[i].rep1_port = temp.rep_port;

            break;
        }
    }

    if(flago == 0){
        SS_List[indexe].ss_id = indexe + 1;
        SS_List[indexe].nm_port = temp.nm_port;
        SS_List[indexe].client_port = temp.client_port;
        SS_List[indexe].status = 1;
        SS_List[indexe].Rep1_status = -1;
        SS_List[indexe].Rep2_status = -1;
        strcpy(SS_List[indexe].ip_address, temp.ip_address);
        strcpy(SS_List[indexe].Base_path, temp.Base_path);
        require_index = indexe;
    }

    // please tokenise //
    printf("%s\n", SS_List[require_index].Rep_Base1_path);
    printf("%s\n", SS_List[require_index].Rep_Base2_path);

    char *token = strtok(temp.accessible_paths, " ");
    char* lastSlash = strrchr(SS_List[indexe].Base_path, '/');
    if(lastSlash == NULL){
        char response[1024];
        strcpy(response, "Data Not Recieved!");
        printf("Server: %s\n", response);
        send(clientSocket, response, strlen(response), 0);
        goto ss_dup;
    }
    while (token != NULL)
    {
        char tempo[1024] = {0};
        strcpy(tempo, lastSlash);
        token[strlen(token) - 1] = '\0';
        insert(Root, strcat(tempo, token), SS_List[require_index].ss_id);
        token = strtok(NULL, " ");
    }
    char response[1024];
    strcpy(response, "Data Recieved!");
    printf("Server: %s\n", response);
    send(clientSocket, response, strlen(response), 0);

    char logMessageBuffer[2048];
    snprintf(logMessageBuffer, sizeof(logMessageBuffer), "Received storage server details: SS_Port =%d, IP=%s, Base_path=%s", SS_List[indexe].client_port, SS_List[indexe].ip_address, SS_List[indexe].Base_path);
    logMessage(logMessageBuffer);
    // int selection = 3;
    // send(clientSocket, &selection, sizeof(selection), 0);
    if(flago == 0){
        indexe++;
    }

    ss_dup:
    close(clientSocket);
    
    printf("[+]Client disconnected.\n\n");
    
    // int q = search(Root, "test.txt");
    // printf("\n%d\n", q);
}

int main(int argc, char **argv)
{
    // if (argc != 2)
    // {
    //     printf("Usage: %s <port>\n", argv[0]);
    //     exit(1);
    // }    
    Head=NULL;
    len=0;
    const char *serverIP = "127.0.0.1";
    // serverPort = atoi(argv[1]);
    serverPort = 6789;
    logMessage("Naming Server initialized.");
    indexe = 0;
    Root = create_Node();
    const int serverSocket = createServerSocket(serverIP, serverPort);
    while (1)
    {
        int clientSocket = acceptClient(serverSocket);
        if(clientSocket != -1){
            int selector;
            recv(clientSocket, &selector, sizeof(selector), 0);
            printf("sele = %d\n", selector);
            pthread_t ss_Thread, cc_Thread;
        if (selector == 1)
        {
            logMessage("Storage server thread created.");
            if (pthread_create(&ss_Thread, NULL, storage_server_thread, (void *)&clientSocket))
            {
                perror("[-] Error creating storage server thread");
                close(clientSocket);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            logMessage("Client thread created.");
            if (pthread_create(&cc_Thread, NULL, client_thread_function, (void *)&clientSocket))
            {
                perror("[-] Error creating storage server thread");
                close(clientSocket);
                exit(EXIT_FAILURE);
            }
        }
        }
    }
    logMessage("Naming Server shutdown.");
    // pthread_join(common_thread, NULL);
    close(serverSocket);

    return 0;
}
