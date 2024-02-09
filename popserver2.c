#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_BUFFER_SIZE 1024
#define MAX_SUBJECT_SIZE 100
#define MAX_LENGTH 50

int pos_response(const char *message, int clientSocket);
int neg_response(const char *message, int clientSocket);
void read_response(int clientSocket, char *full_buff);
int authorisation_check(int clientSocket, char *username, char *password);
void readMail(int clientSocket, const char *username);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int my_port = atoi(argv[1]);

    // Create socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    struct sockaddr_in serverAddr;
    struct sockaddr_in clientAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(my_port);

    // Bind the socket
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("Error binding socket");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(serverSocket, 5) == -1)
    {
        perror("Error listening");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    printf("POP3 Mail server listening on port %d...\n", my_port);
    while (1)
    {
        // Accept incoming connection
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSocket == -1)
        {
            perror("Error accepting connection");
            continue;
        }

        // Fork a new process to handle incoming mail
        pid_t pid = fork();

        if (pid == -1)
        {
            perror("Error forking process");
            close(clientSocket);
            continue;
        }

        if (pid == 0)
        {
            // In child process (R)
            close(serverSocket); // Close listening socket in child process

            // char response[MAX_BUFFER_SIZE];

            // Prepare the response to successful connection
            // snprintf(response, sizeof(response), "POP3 server ready");

            int t = pos_response("POP3 server ready", clientSocket);

            // while(!t) t = pos_response("POP3 server ready", clientSocket);

            // memset(response, 0, sizeof(response));

            char username[100] = "";
            char password[100] = "";
            int proceed = authorisation_check(clientSocket, username, password);
            printf("username: %s\n", username);
            printf("password: %s\n", password);
            if (!proceed)
            {
                close(clientSocket);
                exit(EXIT_SUCCESS);
            }
            else
            {
                // readMail(clientSocket, username);
                printf("Authorisation successful\n");
            }
            close(clientSocket);
            exit(EXIT_SUCCESS);
        }
        else
        {
            // In parent process, close client socket and continue accepting
            close(clientSocket);
        }
    }

    return 0;
}

int pos_response(const char* message, int clientSocket) {
    char temp_buff[100];
    char response[MAX_BUFFER_SIZE];

    // Prepare the positive response
    snprintf(response, sizeof(response), "+OK %s\r\n", message);

    int commandLength = strlen(response);
    int bytesSent = 0;
    int chunkSize;

    while (bytesSent < commandLength)
    {
        // Clear the temp_buff
        memset(temp_buff, 0, sizeof(temp_buff));

        // Determine the size of the next chunk
        chunkSize = commandLength - bytesSent;
        if (chunkSize > sizeof(temp_buff) - 1)
        {
            chunkSize = sizeof(temp_buff) - 1;
        }

        // Copy the next chunk into temp_buff and null-terminate it
        memcpy(temp_buff, &response[bytesSent], chunkSize);
        temp_buff[chunkSize] = '\0';

        // Send the chunk
        if (send(clientSocket, temp_buff, strlen(temp_buff), 0) == -1)
        {
            perror("Error sending positive response");
            return 0;
        }

        // Update bytesSent
        bytesSent += chunkSize;

        // Break if the last chunk sent ends with "\r\n"
        if (chunkSize >= 2 && strcmp(&temp_buff[chunkSize - 2], "\r\n") == 0)
        {
            break;
        }
    }

    // Clear the response
    printf("S : %s\n", response);
    memset(response, 0, sizeof(response));
    return 1;
}

int neg_response(const char* message, int clientSocket) {
    char temp_buff[100];
    char response[MAX_BUFFER_SIZE];

    // Prepare the negative response
    snprintf(response, sizeof(response), "-ERR %s\r\n", message);

    int commandLength = strlen(response);
    int bytesSent = 0;
    int chunkSize;

    while (bytesSent < commandLength)
    {
        // Clear the temp_buff
        memset(temp_buff, 0, sizeof(temp_buff));

        // Determine the size of the next chunk
        chunkSize = commandLength - bytesSent;
        if (chunkSize > sizeof(temp_buff) - 1)
        {
            chunkSize = sizeof(temp_buff) - 1;
        }

        // Copy the next chunk into temp_buff and null-terminate it
        memcpy(temp_buff, &response[bytesSent], chunkSize);
        temp_buff[chunkSize] = '\0';

        // Send the chunk
        if (send(clientSocket, temp_buff, strlen(temp_buff), 0) == -1)
        {
            perror("Error sending negative response");
            return 0;
        }

        // Update bytesSent
        bytesSent += chunkSize;

        // Break if the last chunk sent ends with "\r\n"
        if (chunkSize >= 2 && strcmp(&temp_buff[chunkSize - 2], "\r\n") == 0)
        {
            break;
        }
    }

    // Clear the response
    printf("S : %s\n", response);
    memset(response, 0, sizeof(response));
    return 1;
}

void read_response(int clientSocket, char *full_buff) {
    int index = 0;
    while (1)
    {
        char c;
        int result = recv(clientSocket, &c, 1, 0);
        if (result <= 0)
        {
            perror("Error reading server response");
            return;
        }

        full_buff[index] = c;
        index++;

        // Ensure the buffer is null-terminated
        full_buff[index] = '\0';

        // Break if the full buffer ends with "\r\n"
        if (index >= 2 && full_buff[index - 2] == '\r' && full_buff[index - 1] == '\n')
        {
            break;
        }
    }
    printf("C : %s\n", full_buff);
}

int authorisation_check(int clientSocket, char *username, char *password)
{
    char response[MAX_BUFFER_SIZE];

    // Read the response from the client
    char full_buff[MAX_BUFFER_SIZE] = {0};
    
    read_response(clientSocket, full_buff);

    if (strncmp(full_buff, "USER", 4) == 0)
    {
        char *token = strtok(full_buff, " ");
        token = strtok(NULL, " ");
        strcpy(username, token);
        // printf("username: %s\n", username);
    }
    else
    {
        memset(full_buff, 0, sizeof(full_buff));
        printf("Invalid command\n");
        return 0;
    }
    memset(full_buff, 0, sizeof(full_buff));


// Checking if the username exists in the file
    FILE *file = fopen("user.txt", "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file: user.txt\n");
        return 0;
    }

    char line[256];
    char file_username[256], file_password[256];
    int username_found = 0;

    // Loop to find the username and corresponding password
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%255s %255s", file_username, file_password); // Read the username and password from the line

        printf("file_username: %s\n", file_username);
        printf("%s\n", username);
        if (strcmp(username, file_username) == 0) {
            username_found = 1;
            break;
        }
    }

    if(!username_found) {
        neg_response("Username not found", clientSocket);
        return 0;
    }
    else{
        // Send the positive response of user found
        snprintf(response, sizeof(response), "Username %s mailbox exists", username);
        pos_response(response, clientSocket);
        memset(response, 0, sizeof(response));

        // Read the response from the client
        memset(full_buff, 0, sizeof(full_buff));
        read_response(clientSocket, full_buff);

        if(strcmp(full_buff, "PASS") == 0) {
            char *token = strtok(full_buff, " ");
            token = strtok(NULL, " ");
            strcpy(password, token);
            printf("password: %s\n", password);
        }
        else {
            memset(full_buff, 0, sizeof(full_buff));
            printf("Invalid command\n");
            return 0;
        }

        // Checking if the password matches the username
        if (strcmp(password, file_password) == 0) {
            pos_response("Authorisation successful", clientSocket);
            memset(full_buff, 0, sizeof(full_buff));
            fclose(file);
            return 1;
        } else {
            neg_response("Password does not match", clientSocket);
            memset(full_buff, 0, sizeof(full_buff));
            fclose(file);
            return 0;
        }
    }
}
