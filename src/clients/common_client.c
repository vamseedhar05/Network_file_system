#include "common_client.h"

enum ErrorCodes {
    FNF = 404   ,   // file not found
    FCF = 101   ,   // Failed to create file
    FRF = 103   ,   // Failed to Read File
    FWF = 104   ,   // Failed to Write File
    FDF = 102   ,   // Failed to Delete File
    II  = 108   ,   // Invalid Input
    CFF = 109   ,   // Failed to Copy File
    ECS = 99    ,   // Error Connecting to Server
};

int checkValidInput(char *input)
{
    int check = 1;
    if (strncmp(input, "CREATE", 6) && strncmp(input, "DELETE", 6) && strncmp(input, "WRITE", 5))
    {
        if (strncmp(input, "COPY", 4) && strncmp(input, "READ", 4) && strncmp(input, "FILEINFO", 8) && strncmp(input, "EXIT", 4))
        {
            check = 0;
        }
    }
    return check;
}

void tokenize_command(char *Input, char **Commands, int *num_commands)
{
    char *token;
    int index = 0;

    token = strtok(Input, " \t");
    while (token != NULL)
    {
        Commands[index++] = token;
        token = strtok(NULL, " \t");
    }
    Commands[index] = NULL;
    *num_commands = index;
}

int createSocket(int portnumber, char *ipAddress)
{
    int ClientSocket;
    if ((ClientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in nmAddr;
    nmAddr.sin_family = AF_INET;
    nmAddr.sin_port = htons(portnumber);
    nmAddr.sin_addr.s_addr = inet_addr(ipAddress);
    if (inet_aton(ipAddress, &nmAddr.sin_addr) == 0)
    {
        perror("Invalid IP address");
        exit(EXIT_FAILURE);
    }
    if (connect(ClientSocket, (struct sockaddr *)&nmAddr, sizeof(nmAddr)) < 0)
    {
        perror("Error connecting to Server");
        exit(EXIT_FAILURE);
    }

    return ClientSocket;
}

int ReceiveAck(int ClientSocket)
{
    int ack = 1;
    recv(ClientSocket, &ack, sizeof(ack), 0);
    printf("Acknowled: %d\n", ack);
    if (ack == 1)
    {
        printf("%s\n", "Successfully operation executed");
    }
    return ack;
}

int main(int argc, char *argv[])
{

    int NMS_socket, portNumber;
    char ipAddress[20];
    while (1)
    {
        bzero(ipAddress, sizeof(ipAddress));
        strcpy(ipAddress, "127.0.0.1");

        if ((NMS_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("Error creating socket");
            exit(EXIT_FAILURE);
        }
        struct sockaddr_in nmAddr;
        nmAddr.sin_family = AF_INET;
        nmAddr.sin_port = htons(6789);
        nmAddr.sin_addr.s_addr = inet_addr(ipAddress);
        if (inet_aton(ipAddress, &nmAddr.sin_addr) == 0) {
            perror("Invalid IP address");
            exit(EXIT_FAILURE);
        }
        if (connect(NMS_socket, (struct sockaddr *)&nmAddr, sizeof(nmAddr)) < 0)
        {
            printf("Error connecting to Server  (Error Code %d)\n",ECS);
            exit(EXIT_FAILURE);
        }

        int selection = 2;
        send(NMS_socket, &selection, sizeof(selection), 0);
        char Input[MAX_BUFFER_SIZE];
        printf("Input: ");
        fgets(Input, sizeof(Input), stdin);
        Input[strlen(Input) - 1] = '\0';
        if (!checkValidInput(Input))
        {
            printf("Invalid Input (ERROR CODE : %d)\n",II);
            continue;
        }
        if (strcmp(Input, "EXIT") == 0)
        {
            break;
        }

        char *Commands[10];
        int num_commands = 0;
        tokenize_command(Input, Commands, &num_commands);
        InputDetails Packet;
        strcpy(Packet.Type, Commands[0]);
        strcpy(Packet.Path, Commands[1]);
        if (num_commands > 2)
            strcpy(Packet.Dest_path, Commands[2]);
        else
            strcpy(Packet.Dest_path, "None");
        send(NMS_socket, &Packet, sizeof(Packet), 0);
        StorageServer SS;
        size_t size_ss_mini = sizeof(SS.Base_path) + sizeof(SS.client_port) + sizeof(SS.ip_address) + sizeof(SS.nm_port) + sizeof(SS.ss_id) + sizeof(SS.status);
        bzero(&SS, size_ss_mini);
        recv(NMS_socket, &SS, size_ss_mini, 0);
        if (SS.ss_id == -1)
        {
            printf("File Not Found (ERROR CODE %d)\n",FNF);
            close(NMS_socket);
            goto end;
        }
        else if (SS.ss_id == -2)
        {
            printf("Given Source PATH doesn't exist\n");
            close(NMS_socket);
            goto end;
        }
        else if (SS.ss_id == -3)
        {
            printf("Given Destination PATH doesn't exist\n");
            close(NMS_socket);
            goto end;
        }
        if (strcmp(Commands[0], "CREATE") == 0)
        {
            int ack = ReceiveAck(NMS_socket);
            if (ack == 0)
            {
                printf("Failed to Create file (ERROR CODE %d)\n",FCF);
            }
            close(NMS_socket);
        }
        else if (strcmp(Commands[0], "DELETE") == 0)
        {
            int ack = ReceiveAck(NMS_socket);
            if (ack == 0)
            {
                printf("Failed to Delete file (ERROR CODE %d)\n",FDF);
            }
            close(NMS_socket);
        }
        else if (strcmp(Commands[0], "COPY") == 0)
        {
            int ack =ReceiveAck(NMS_socket);
            if(!ack){
                printf("Failed to Copy file (ERROR CODE %d)\n",CFF);
            }
            close(NMS_socket);
        }

        // Connection to SSn (READ, WRITE, FILEINFO)

        else
        {
            int ack;
            printf("Client: %d ip: %s\n", SS.client_port, SS.ip_address);
            int ClientSocket = createSocket(SS.client_port, SS.ip_address);
            int selector = 2;
            Packet.CliSock = ClientSocket;
            char *start = strchr(Packet.Path + 1, '/');
            strcpy(Packet.Path, start + 1);
            size_t size_pack = sizeof(Packet.Type) + sizeof(Packet.Path) + sizeof(Packet.Dest_path) + sizeof(Packet.CliSock);
            send(ClientSocket, &selector, sizeof(selector), 0);
            send(ClientSocket, &Packet, size_pack, 0);
            if (strcmp(Commands[0], "READ") == 0)
            {
                char Data[MAX_DATA];
                int flag = 0;
                char buf[1024];
                while (1)
                {
                    bzero(buf, sizeof(buf));
                    recv(ClientSocket, buf, sizeof(buf), 0);
                    if (strncmp(buf, "STOP", 4) == 0)
                    {
                        flag = 1;
                        break;
                    }
                    printf("%s\n", buf);
                }
                printf("\n");
                if (flag)
                    printf("READ Successfull\n");
                ack = ReceiveAck(ClientSocket);
                if(!ack){
                    printf("Failed to READ file (ERROR CODE %d)\n",FRF);
                }
            }

            else if (strcmp(Commands[0], "WRITE") == 0)
            {
                printf("Give Input to Write in %s: \n", Commands[1]);
                char Data[MAX_DATA];
                fgets(Data, sizeof(Data), stdin);
                Data[strlen(Data) - 1] = '\0';
                send(ClientSocket, &Data, sizeof(Data), 0);
                ack = ReceiveAck(ClientSocket);
                if (!ack)
                {
                    printf("Failed to Write file (ERROR CODE %d)\n",FWF);
                }
                
            }

            else if (strcmp(Commands[0], "FILEINFO") == 0)
            {
                struct stat file_stat;
                struct tm dater;
                recv(ClientSocket, &file_stat, sizeof(file_stat), 0);
                printf("\nFile access: ");
                if (file_stat.st_mode & S_IWUSR)
                    printf("write ");
                if (file_stat.st_mode & S_IRUSR)
                    printf("read ");
                if (file_stat.st_mode & S_IXUSR)
                    printf("execute");
                printf("\nFile size: %ld", file_stat.st_size);
                dater = *(gmtime(&file_stat.st_ctime));
                printf("\nCreated on: %d-%d-%d %d:%d:%d", dater.tm_mday, dater.tm_mon, dater.tm_year + 1900, dater.tm_hour, dater.tm_min, dater.tm_sec);

                dater = *(gmtime(&file_stat.st_mtime));
                printf("\nModified on: %d-%d-%d %d:%d:%d", dater.tm_mday, dater.tm_mon, dater.tm_year + 1900, dater.tm_hour, dater.tm_min, dater.tm_sec);
                printf("\n");
            }
            // ClientSocket = createSocket(6789,ipAddress);

            close(ClientSocket);
            send(NMS_socket, &ack, sizeof(ack), 0);
            close(NMS_socket);
        }

    end:
    }
    return 0;
}