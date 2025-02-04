#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_BUFFER_SIZE 1024
#define MAX_SUBJECT_SIZE 100

// Function to handle incoming mail
void handleMail(int clientSocket, const char* username, const char* domain);

// Function to create user subdirectory if it doesn't exist
void createUserDirectory(const char* username);
void sendCommand(int clientSocket,char* command)
{
    if (write(clientSocket, command, strlen(command)) == -1) {
        perror("Error sending HELO command");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }
}
void recvResponse(int clientSocket,char* buffer)
{
ssize_t bytesRead;
    size_t totalBytesRead = 0;
        bzero(buffer,MAX_BUFFER_SIZE);

    // Loop until the entire message is received
    while ((bytesRead = recv(clientSocket, buffer + totalBytesRead, MAX_BUFFER_SIZE - totalBytesRead , 0)) > 0) {
        totalBytesRead += bytesRead;
        // Check for end of message condition
        if (strstr(buffer, "\0") != NULL) {
            break;
        }

    }

    if (bytesRead == -1) {
        perror("Error reading server response");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    // Null-terminate the received data
    buffer[totalBytesRead] = '\0';
        // printf("Received message: %s\n", buffer);

}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int my_port = atoi(argv[1]);

    // Create socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(my_port);

    // Bind the socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Error binding socket");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverSocket, 5) == -1) {
        perror("Error listening");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    printf("Mail server listening on port %d...\n", my_port);

    while (1) {
        // Accept incoming connection
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        if (clientSocket == -1) {
            perror("Error accepting connection");
            continue;
        }

        // Fork a new process to handle incoming mail
        pid_t pid = fork();

        if (pid == -1) {
            perror("Error forking process");
            close(clientSocket);
            continue;
        }

        if (pid == 0) {
            // In child process (R)
            close(serverSocket);  // Close listening socket in child process
            handleMail(clientSocket, "user", "iitkgp.edu");
            close(clientSocket);
            exit(EXIT_SUCCESS);
        } else {
            // In parent process, close client socket and continue accepting
            close(clientSocket);
        }
    }

    return 0;
}

void handleMail(int clientSocket, const char* username, const char* domain) {
    time_t rawTime;
    struct tm *info;

    // Get current time
    time(&rawTime);
    info = localtime(&rawTime);
    
    // Read and handle incoming mail
    ssize_t bytesRead;

    char buffer[MAX_BUFFER_SIZE];
    createUserDirectory(username);

    // Send acknowledgment on connection
    sprintf(buffer,"220 <%s> Service ready\0",domain);
    sendCommand(clientSocket,buffer);
    printf("S: %s\n",buffer);
    // Read the HELO message from the client
    recvResponse(clientSocket,buffer);
        // Print the client's HELO message

    printf("C: %s\n",buffer);


    // Send a response to the client (you may need to customize this)
    char response[MAX_BUFFER_SIZE];
    sprintf(response,"250 OK %s\0",buffer);
    sendCommand(clientSocket,response);
    printf("S: %s\n",response);
    recvResponse(clientSocket,buffer);
     char *start = strstr(buffer, "<");
     char *end = strchr(start, '>');
    size_t enclosedStringLength = end - start - 1;
    // Copy the enclosed string to the output buffer
    char from[MAX_BUFFER_SIZE];
    strncpy(from, start + 1, enclosedStringLength);
    from[enclosedStringLength] = '\0';
    printf("from %s \n",from);
    sprintf(response,"250 <%s> ... Sender ok\0",from);
    sendCommand(clientSocket,response);
    printf("S: %s\n",response);
    // ----------Recipient-------
    recvResponse(clientSocket,buffer);
    start = strstr(buffer, ": ");
     end = strchr(start, '\0');
    enclosedStringLength = end - start - 1;
    // Copy the enclosed string to the output buffer
    char to[MAX_BUFFER_SIZE];
    strncpy(to, start + 1, enclosedStringLength);
    to[enclosedStringLength] = '\0';
    sprintf(response,"250 root... Recipient ok\0");
    sendCommand(clientSocket,response);
    printf("S: %s\n",response);

    // ----DATA---
    recvResponse(clientSocket,buffer);
    printf("C: %s\n",buffer);
    sprintf(response,"354 Enter mail, end with \".\" on a line by itself\0");
    sendCommand(clientSocket,response);
    printf("S: %s\n",response);
    // --------RECEVING MAIL--------
    const char* stop=".";
    while(1)
    {
    // printf("in loop:\n");
    recvResponse(clientSocket,buffer);
    if(strcmp(buffer,stop)==0)break;
    printf("%s\n",buffer);
    }
    
}

void createUserDirectory(const char* username) {
    // Create user subdirectory if it doesn't exist
    char directoryPath[256];
    snprintf(directoryPath, sizeof(directoryPath), "./%s", username);

    struct stat st = {0};
    if (stat(directoryPath, &st) == -1) {
        mkdir(directoryPath, 0700);
    }
}
