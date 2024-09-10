#include "storage_server.h"

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}
pthread_t serverThreadId;


size_t size_of_SS(StorageServer S){
    size_t size_ss = sizeof(S.nm_port);
    size_ss += sizeof(S.ss_port);
    size_ss += sizeof(S.ip_address);
    size_ss += sizeof(S.current_path);
    size_ss += sizeof(S.accessablepaths);
    size_ss += sizeof(S.No_of_Paths);
    size_ss += sizeof(S.rep_port);
    return size_ss;
}


int createSocket(const char* ip,int port,int socket,char c){
    struct sockaddr_in server_addr;
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if(c == 'S'){
        if (bind(socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("[-]Bind error");
            exit(1);
        }
        printf("[+]Bind to the port number: %d\n", port);

        listen(socket, 10);  
        printf("Listening...\n");
    }
    else if(c == 'C'){
        if (connect(socket,(struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("[-]Connection error");
            exit(1);
        }
        printf("[+]Connected to server on port %d.\n", port);
    }
    
    return socket;
}

int createClientSocket(const char *ip, int port) {
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        perror("[-]Socket error");
        exit(1);
    }
    printf("[+]TCP client socket created.\n");

    return createSocket(ip,port,client_sock,'C');
}



int createServerSocket(const char *ip, int port) {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("[-]Socket error");
        exit(1);
    }
    printf("[+]TCP server socket created.\n");


    return createSocket(ip,port,server_sock,'S');
}

int acceptClient(int server_sock) {
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(client_addr);
    int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size);
    if (client_sock < 0) {
        perror("[-]Accept error");
        exit(1);
    }
    printf("[+]Client connected.\n");

    return client_sock;
}


StorageServer getUserInput(StorageServer SS) {
    char buffer[100];
    SS.accessablepaths[0] = '\0';

    while (1) {       
        fgets(buffer, sizeof(buffer), stdin);
        if (buffer[0] == '\n') {
            break;
        }

        strcat(SS.accessablepaths, buffer);
        strcat(SS.accessablepaths, " ");
        SS.No_of_Paths++;
    }

    size_t len = strlen(SS.accessablepaths);
    if (len > 0) {
        SS.accessablepaths[len - 1] = '\0';  
    }
    printf("%s\n",SS.accessablepaths);
    return SS;
}


int WriteToFile(char* file_path,int* clientSocket) {
    pthread_mutex_lock(&write_lock);
    FILE* file = fopen(file_path, "a");
    if (file == NULL) {
        perror("Error opening the file");
        return 0;
    }
    char buf[1024];
    // while(1){
        bzero(buf, sizeof(buf));
        if (recv(*clientSocket, buf, sizeof(buf), 0) == -1) {
            perror("Error receiving data from client");
            return 0;
        }
        fprintf(file, "%s\n", buf);
    // }
    fclose(file);
    printf("Server: Successfully Written to the file\n");
    pthread_mutex_unlock(&write_lock);
    return 1;
}


int ReadFile(char* file_path, int* clientSocket) {
    FILE* file = fopen(file_path, "r");

    if (file == NULL) {
        printf("Error opening the file");
        return 0;
    }

    char buffer[1024];
    while ((fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (send(*clientSocket, buffer, sizeof(buffer), 0) == -1) {
            printf("Error sending data to client");
            return 0;
        }
        printf("%s", buffer);
    }
    bzero(buffer, sizeof(buffer));
    strcpy(buffer, "STOP");
    send(*clientSocket, buffer, sizeof(buffer), 0);
    fclose(file);
    return 1;
}


void SendPermissions(char* file_path, int* clientSocket) {
    struct stat file_stat;

    if (stat(file_path, &file_stat) == -1) {
        perror("Error getting file information");
        return;
    }

    if (send(*clientSocket, &file_stat, sizeof(file_stat), 0) == -1) {
        perror("Error sending file mode to client");
    }
}

int endsWithTxt(const char *str) {
    size_t len = strlen(str);
    if (strcmp(str + len - 1, "/")) {
        return 1; // String ends with "anything"
    } else {
        return 0; // String does not end with "/"
    }
}

int Create (char*Home, char* Path){

    // Path should start with '/'

    int isFile=0;
    isFile=endsWithTxt(Path);
    char Name[MAX_BUFFER_SIZE];
    strcpy(Name,Home);
    strcat(Name,Path);
    if(isFile){
        FILE  *file = fopen(Name, "w+");
        if (file == NULL) {
            printf("Error opening file!\n");
            return 0;
        }
        fclose(file);
    }
    else{
        int status = mkdir(Name, S_IRWXU);
        if (status != 0) {
            printf("Error creating directory!\n");
            return 0;
        }
    }
    return 1;
}

int Delete(char* Home, char* Path){

    // Path should start with '/'

    char Name[MAX_BUFFER_SIZE];
    strcpy(Name,Home);
    strcat(Name,Path);
    
    int result = remove(Name);     // Can only delete empty directories 

    // if non empty directories need to be deleted (Uncomment below)

    // char command[MAX_BUFFER_SIZE];
    // sprintf(command, "rm -r %s", Name);     
    // int result = system(command);

    if (result == 0) {
        printf("File or folder deleted successfully\n");
    } else {
        printf("Error deleting file or folder\n");
        return 0;
    }
    return 1;
}

int copyFile(char *sourcePath, char *destinationPath) {

    FILE *sourceFile, *destinationFile;
    char buffer[1024];
    size_t bytesRead;
    char final_d[MAX_BUFFER_SIZE];
    strcpy(final_d,destinationPath);

    // Open the source file for reading
    sourceFile = fopen(sourcePath, "rb");
    if (sourceFile == NULL) {
        perror("Error opening source file");
        return 0;
    }

    struct stat statB1;
    if((stat(destinationPath, &statB1) == 0 && S_ISDIR(statB1.st_mode))){
        char *lastSlash = strrchr(sourcePath, '/');  // Find the last occurrence of '/'

        if (lastSlash != NULL) {
            // Index of the last '/' in the string
            size_t lastIndex = lastSlash - sourcePath;

            // Extract the substring from the last '/' until the end
            char *substring = sourcePath + lastIndex + 1;
            strcat(final_d,"/");
            strcat(final_d,substring);
        } else {
            strcat(final_d,"/");
            strcat(final_d,sourcePath);
            // printf("Destination cant be a directory\n");
        }
    }
    // Open the destination file for writing
    destinationFile = fopen(final_d, "wb");
    if (destinationFile == NULL) {
        perror("Error opening destination file");
        fclose(sourceFile);
        return 0;
    }

    // Copy data from source to destination
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), sourceFile)) > 0) {
        fwrite(buffer, 1, bytesRead, destinationFile);
    }

    // Close the files
    fclose(sourceFile);
    fclose(destinationFile);

    return 1;
}

int copyDirectory(char *sourcePath, char *destinationPath) {
    DIR *dir;
    struct dirent *entry;
    struct stat statBuffer;
    char sourceFilePath[MAX_BUFFER_SIZE];
    char destinationFilePath[MAX_BUFFER_SIZE];

    // Open the source directory
    dir = opendir(sourcePath);
    if (dir == NULL) {
        perror("Error opening source directory");
        return 0;
    }

    // Check if the destination path is an existing directory
    if (stat(destinationPath, &statBuffer) == 0 && S_ISDIR(statBuffer.st_mode)) {
        // printf("Destination directory already exists. Copying contents inside.\n");
        // ok
    } else {
        // Create the destination directory
        if (mkdir(destinationPath, 0777) == -1) {
            perror("Error creating destination directory");
            closedir(dir);
            return 0;
        }
    }
    int CD=0,CF=0;
    // Read the contents of the source directory
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue; // Skip "." and ".."
        }

        snprintf(sourceFilePath, MAX_BUFFER_SIZE, "%s/%s", sourcePath, entry->d_name);
        snprintf(destinationFilePath, MAX_BUFFER_SIZE, "%s/%s", destinationPath, entry->d_name);

        // Check if the entry is a directory
        if (stat(sourceFilePath, &statBuffer) == 0 && S_ISDIR(statBuffer.st_mode)) {
            // Recursively copy subdirectories
            CD=copyDirectory(sourceFilePath, destinationFilePath);
        } else {
            // Copy files
            CF=copyFile(sourceFilePath, destinationFilePath);
        }
    }
    printf("Contents Copied Successfully\n");
    // Close the source directory
    closedir(dir);
    return CD || CF;
}

int copy_IN_SSN(char* HOME, char *final_s, char *final_d) {
    struct stat statB1,statB2,statB3,statB4;
    int CD=0,CF=0;
    char sourcePath[MAX_BUFFER_SIZE];
    char destinationPath[MAX_BUFFER_SIZE];
    strcpy(sourcePath,HOME);
    strcpy(destinationPath,HOME);
    strcat(sourcePath,"/");
    strcat(destinationPath,"/");
    strcat(sourcePath,final_s);
    strcat(destinationPath,final_d);
    printf("Source =%s\n",sourcePath);
    printf("Destination =%s\n",destinationPath);
    if ((stat(sourcePath, &statB1) == 0 && S_ISDIR(statB1.st_mode)) &&
           (stat(destinationPath, &statB2) == 0 && S_ISDIR(statB2.st_mode)) ) {
        CD=copyDirectory(sourcePath, destinationPath);
    } else if((stat(sourcePath, &statB3) == 0 &&  S_ISREG(statB3.st_mode)) &&
                (stat(sourcePath, &statB4) == 0 && (S_ISREG(statB4.st_mode)) || S_ISDIR(statB4.st_mode))) {
        CF=copyFile(sourcePath, destinationPath);
    }
    else{
        printf("Source or Destination doesn't exist or doesn't match \n");
    }
    return CD || CF;
}

void* ExecuteNmCommand(void *clientsock)
{
    int *clientsocket = (int *)clientsock;
    InputDetails buffer;
    size_t size_cc = sizeof(buffer.Type) + sizeof(buffer.Path) + sizeof(buffer.Dest_path) + sizeof(buffer.CliSock);
    if(recv(*clientsocket, &buffer, size_cc, 0) == -1 || !strcmp(buffer.Path,"")){
        perror("Error receiving data from client");
        close(*clientsocket);
        printf("[+]Client disconnected.\n\n");
    }
    printf("%s: %s\n", "Client", buffer.Path);
    if(!strncmp(buffer.Type,"CREATE",6)){
        int ack = Create("", buffer.Path);
        send(*clientsocket, &ack, sizeof(ack), 0);
    }
    else if(!strncmp(buffer.Type,"DELETE",6)){
        int ack = Delete("", buffer.Path);
        send(*clientsocket, &ack, sizeof(ack), 0);
    }
    else if(!strncmp(buffer.Type,"COPY",4)){
        char HOME[MAX_BUFFER_SIZE];
        getcwd(HOME,MAX_BUFFER_SIZE);
        int ack=copy_IN_SSN(HOME,buffer.Path,buffer.Dest_path);
        send(*clientsocket, &ack, sizeof(ack), 0);
        // copy from a certain SS into this
    }
    else if(!strncmp(buffer.Type,"PASTE",5)){
        // paste into a certain SS from this 
    }
    close(*clientsocket);
    printf("[+]Client disconnected.\n\n");
}


void* ExecuteClientCommand(void *clientsock)
{
    int *clientsocket = (int *)clientsock;
    // const int clientSocket = (int)*clientsocker;
    // InputDetails buffer;
    // size_t size_cc = sizeof(buffer.Type) + sizeof(buffer.Path) + sizeof(buffer.Dest_path);
    // printf("%d\n",clientSocket);
    // int x;
    // recv(clientSocket, &x,sizeof(x), 0);
    // printf("%s: %d\n", "Client",x);
    // // printf("%s: %s\n", "Client", buffer.Type);

    InputDetails buffer;
    size_t size_cc = sizeof(buffer.Type) + sizeof(buffer.Path) + sizeof(buffer.Dest_path) + sizeof(buffer.CliSock);
    recv(*clientsocket, &buffer,size_cc, 0);
    printf("%s: %s\n", "Client",buffer.Path);
    printf("%s: %s\n", "Client", buffer.Type);

    if(!strncmp(buffer.Type,"READ",4)){
        int ack = ReadFile(buffer.Path, clientsocket);
        send(*clientsocket, &ack, sizeof(ack), 0);

    }
    else if(!strncmp(buffer.Type,"WRITE",5)){
        int ack = WriteToFile(buffer.Path, clientsocket);
        send(*clientsocket, &ack, sizeof(ack), 0);
    }
    else if(!strncmp(buffer.Type,"FILEINFO",8))
        SendPermissions(buffer.Path, clientsocket);

    else{
        printf("Permission Denied");
    }
    close(*clientsocket);        // printf("Server: %s\n", response);

    printf("[+]Client disconnected.\n\n");
}

void* serverThread(void* arg) {
    // char** argv = (char**) arg;
    const char* ip = "127.0.0.1";
    int ss_port = atoi((char*)arg);
    const int serverSocket = createServerSocket(ip, ss_port);

    while (1) {
        int clientSocket = acceptClient(serverSocket);

        int selector = 0;
        recv(clientSocket, &selector, sizeof(selector), 0);
        pthread_t nm_thread, cc_thread;
        if(selector == 1){
            if (pthread_create(&nm_thread, NULL, ExecuteNmCommand, (void *)&clientSocket))
            {
                perror("[-] Error creating nm server thread");
                close(clientSocket);
                exit(EXIT_FAILURE);
            }
        }
        else if(selector == 2){
            if (pthread_create(&cc_thread, NULL, ExecuteClientCommand, (void *)&clientSocket))
            {
                perror("[-] Error creating client server thread");
                close(clientSocket);
                exit(EXIT_FAILURE);
            }
        }
        
        // char response[1024];
        // strcpy(response, "Hello from server");
        // printf("Server: %s\n", response);
        // send(clientSocket, response, strlen(response), 0);
    }
}
void* clientThread(void* arg) {
    char** argv = (char**) arg;
    const char* ip = "127.0.0.1";
    int portNumber = atoi(argv[1]);
    int clientSocket = createClientSocket(ip, portNumber);
    
    StorageServer SS;
    // SS.id = -1;
    SS.nm_port = portNumber;
    SS.ss_port = atoi(argv[2]);
    SS.rep_port = 0;
    strcpy(SS.ip_address, ip);
    if (getcwd(SS.current_path, sizeof(SS.current_path)) == NULL) {
        error("Error getting current directory");
    }
    SS.current_path[strlen(SS.current_path)] = '\0';
    int selection = 1;
    send(clientSocket, &selection, sizeof(selection), 0);
    printf("Enter the Accessible Paths:\n");
    SS = getUserInput(SS);

    size_t size_ss = size_of_SS(SS);

    send(clientSocket, &SS, size_ss, 0);
    printf("[+] Message sent to Server:\n");
    printf("SS Port: %d\n", SS.ss_port);
    printf("Current Path: %s\n", SS.current_path);
    printf("Accessible Paths:%s\n",SS.accessablepaths);
    printf("\n");

    char response[1024];
    if(recv(clientSocket, response, sizeof(response), 0) == -1){
        perror("Error receiving data from server");
        exit(EXIT_FAILURE);
    }

    printf("[+] Response from the server: %s\n", response);
    
    if (pthread_create(&serverThreadId, NULL, serverThread, argv[2]) != 0) {
        perror("[-] Error creating server thread");
        exit(EXIT_FAILURE);
    }
    close(clientSocket);
}


int main() {
    // if (argc != 3) {
    //     fprintf(stderr, "Usage: %s <nms_port_number> <ss_port_number>\n", argv[0]);
    //     exit(EXIT_FAILURE);
    // }
    char* argv[] = {"","6789","8080"};
    pthread_t clientThreadId;

    
    if (pthread_create(&clientThreadId, NULL, clientThread, (void*)argv) != 0) {
        perror("[-] Error creating client thread");
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&write_lock, NULL);

    
    pthread_join(clientThreadId, NULL);
    pthread_join(serverThreadId, NULL);

    return 0;
}
