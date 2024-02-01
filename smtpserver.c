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
    if (write(clientSocket, buffer, strlen(buffer)) == -1) {
        perror("Error sending acknowledgment");
        return;
    }
    printf("S: %s\n",buffer);
    bzero(buffer,sizeof(buffer));
    // Read the HELO message from the client
    if (read(clientSocket, buffer, sizeof(buffer)) == -1) {
        perror("Error reading client HELO message");
        return;
    }
        // Print the client's HELO message

    printf("C: %s\n",buffer);


    // Send a response to the client (you may need to customize this)
     char response[MAX_BUFFER_SIZE];
    sprintf(response,"250 OK %s",buffer);

    if (write(clientSocket, response, strlen(response)) == -1) {
        perror("Error sending response to client");
        return;
    }
        printf("S: %s\n",response);


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
