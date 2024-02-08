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
#include <arpa/inet.h>
#include <fcntl.h>


#define MAX_BUFFER_SIZE 1024
#define MAX_SUBJECT_SIZE 100

// Function to handle incoming mail
void handleMail(int clientSocket, const char *domain);

// Function to create user subdirectory if it doesn't exist
int createUserDirectoryandAppend(const char *username, const char *full_buff_from, const char *full_buff_to, const char *full_buff_subject, const char *full_buff_message);


#define MAX_LENGTH 50

// Function to create user directories
void create_user_directories(const char *username) {
    char user_directory[MAX_LENGTH];
    snprintf(user_directory, sizeof(user_directory), "./%s", username);
    
    // Create subdirectory for each user
    struct stat st = {0};
    if (stat(user_directory, &st) == -1)
    {
        mkdir(user_directory, 0700);
    }

}



void lockMailbox(char* mailbox) {
    int lockFile = open(mailbox, O_CREAT | O_RDWR, 0666);
    if (lockFile == -1) {
        perror("Error opening lock file");
        exit(EXIT_FAILURE);
    }

    if (flock(lockFile, LOCK_EX) == -1) {
        perror("Error locking mailbox resources");
        close(lockFile);
        exit(EXIT_FAILURE);
    }
}

void unlockMailbox(char* mailbox) {
    int lockFile = open(mailbox, O_CREAT | O_RDWR, 0666);
    if (lockFile == -1) {
        perror("Error opening lock file");
        exit(EXIT_FAILURE);
    }

    if (flock(lockFile, LOCK_UN) == -1) {
        perror("Error unlocking mailbox resources");
        close(lockFile);
        exit(EXIT_FAILURE);
    }
    close(lockFile);
}



int authenticate(char* user, char*pass)
{
    FILE *file;
    char line[MAX_LENGTH];
    char username[MAX_LENGTH];
    char password[MAX_LENGTH];
    int code=0;
    // Open the file for reading
    file = fopen("user.txt", "r");
    
    if (file == NULL) {
        printf("Error: user.txt not found.\n");
        return;
    }

    // Read user information from file
    while (fgets(line, sizeof(line), file) != NULL) {
        // Split line into username and password
        sscanf(line, "%s %s", username, password);
        if (strcmp(username, user) == 0  )
        {
            if(strcmp(password, pass) == 0 )
            {
            printf("\nAuthenticated\n");
            return 1;
            }
            else code=2;
            
        }
        

        // Create user directory
    }

    // Close the file
    fclose(file);
    return code;
}

// Function to create user directories

void createUserDirectory()
{
    FILE *file;
    char line[MAX_LENGTH];
    char username[MAX_LENGTH];
    char password[MAX_LENGTH];
    
    // Open the file for reading
    file = fopen("user.txt", "r");
    
    if (file == NULL) {
        printf("Error: user.txt not found.\n");
        return;
    }

    // Read user information from file
    while (fgets(line, sizeof(line), file) != NULL) {
        // Split line into username and password
        sscanf(line, "%s %s", username, password);
        
        // Create user directory
        create_user_directories(username);
    }

    // Close the file
    fclose(file);
}


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

    printf("POP3 server listening on port %d...\n", my_port);
    createUserDirectory();


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
            handlePOP3(clientSocket, "iitkgp.edu");
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
void sendMessage(int clientSocket,char* response)
{
     int commandLength = strlen(response);
    int bytesSent = 0;
    int chunkSize;
    char temp_buff[100];

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
            perror("Error sending connection acknowledgment");
            return;
        }

        // Print the sent chunk
        // printf\("C: %s\n", temp_buff\);

        // Update bytesSent
        bytesSent += chunkSize;

        // Break if the last chunk sent ends with "\r\n"
        if (chunkSize >= 2 && strcmp(&temp_buff[chunkSize - 2], "\r\n") == 0)
        {
            break;
        }
    }

}
void recvMessage(int clientSocket,char* full_buff)
{
      memset(full_buff, 0, sizeof(full_buff));
    int index=0;
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
}
void handlePOP3(int clientSocket, const char *domain)
{
    // Read and handle incoming mail
    int index = 0;
    printf("client accepted... \n");
    char response[MAX_BUFFER_SIZE];
    // Prepare the response
    snprintf(response, sizeof(response), "+OK POP3 server ready\r\n", domain);

    sendMessage(clientSocket,response);
    // Read the USER message from the client
    char full_buff[MAX_BUFFER_SIZE] = {0};

    // printf("C: %s\n", full_buff);
    memset(response, 0, sizeof(response));

    //--------------------------------------------------------------------------------------------
    recvMessage(clientSocket,full_buff);
    index = 0;
    // Check if the client has quit
    if (strncmp(full_buff, "QUIT", 4) == 0)
    {
        fprintf(stderr, "Received QUIT command from client: %s\n", full_buff);
        return;
    }

    // Check the full response
    if (strncmp(full_buff, "USER: ", 6) != 0)
    {
        fprintf(stderr, "Error: Unexpected response from client: %s\n", full_buff);
        return;
    }
    // Extract and store the sender email from full_buff
    char user[100];
    char *user_start = strstr(full_buff, "USER: ");
    char *user_end = strstr(full_buff, "\r");
    if (user_start != NULL && user_end != NULL)
    {
        int user_length = user_end - user_start - 6;

        // Ensure length does not exceed the size of username - 1
        if (user_length > sizeof(user) - 1)
        {
            user_length = sizeof(user) - 1;
        }
        strncpy(user, user_start + 6, user_length);
        user[user_length] = '\0';
        
    }

    //--------------------------------------------------------------------------------------------
    memset(response, 0, sizeof(response));
    // Prepare the response
    snprintf(response, sizeof(response), "+OK\r\n", domain);

    sendMessage(clientSocket,response);

    //--------------------------------------------------------------------------------------------
    recvMessage(clientSocket,full_buff);
    if (strncmp(full_buff, "QUIT", 4) == 0)
    {
        fprintf(stderr, "Received QUIT command from client: %s\n", full_buff);
        return;
    }

    // Check the full response
    if (strncmp(full_buff, "PASS: ", 6) != 0)
    {
        fprintf(stderr, "Error: Unexpected response from client: %s\n", full_buff);
        return;
    }

    // Find the start and end positions of the target_usr_mail
    char pass[100] = {0}; // Initialize all elements to 0
    char *start = strstr(full_buff, "PASS: ");
    char *end = strchr(start, '\r');
    if (start != NULL && end != NULL)
    {
        // Calculate the length of the target_usr_mail
        int length = end - (start + 6);

        // Ensure length does not exceed the size of PASSWORD - 1
        if (length > sizeof(pass) - 1)
        {
            length = sizeof(pass) - 1;
        }

        // Extract the target_usr_mail and store it in the username variable
        strncpy(pass, start + 6, length);
        pass[length] = '\0'; // Ensure username is null-terminated

        
    }
    printf("Username: %s| password: %s\n",user,pass);
    memset(response, 0, sizeof(response));
    char directoryPath[256];
    snprintf(directoryPath, sizeof(directoryPath), "./%s", user);
    struct stat st = {0};
    if (stat(directoryPath, &st) == -1)
    {
    snprintf(response, sizeof(response), "-ERR No such user\r\n");
    // flag=1;
    }
    else
    {
        int res=authenticate(user,pass);
        if(res==2)
            snprintf(response, sizeof(response), "-ERR Username and Password did not match\r\n");
        else if(res==0) 
            snprintf(response, sizeof(response), "-ERR Username not found\r\n");
        else if(res==1)
            snprintf(response, sizeof(response), "+OK\r\n");

    } 



    char mailboxPath[MAX_BUFFER_SIZE];  // Adjust this according to your directory structure
    sprintf(mailboxPath, "./%s/mymailbox", user);

    lockMailbox(mailboxPath);

    // Open mailbox file and read emails
    FILE *mailboxFile = fopen(mailboxPath, "r");
    if (mailboxFile == NULL) {
        perror("Error opening mailbox file");
        memset(response, 0, sizeof(response));
        snprintf(response, sizeof(response), "-ERR Mailbox could not be opened\r\n");
        sendMessage(clientSocket,response);
        close(clientSocket);
        exit(EXIT_FAILURE);
    }
    sendMessage(clientSocket,response);

    printf("end of pop3 server\n");

    
    close(clientSocket);
    fclose(mailboxFile);
    // Unlock mailbox resources
    unlockMailbox(mailboxPath);

    
}

int createUserDirectoryandAppend(const char *username, const char *full_buff_from, const char *full_buff_to, const char *full_buff_subject, const char *full_buff_message)
{
    // Create user subdirectory if it doesn't exist
    char directoryPath[256];
    snprintf(directoryPath, sizeof(directoryPath), "./%s", username);

    struct stat st = {0};
    if (stat(directoryPath, &st) == -1)
    {
        // mkdir(directoryPath, 0700);
        return 0;
    }

    // Create a file named "mymailbox" within the user subdirectory
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "./%s/mymailbox", username);

    // Open the file for appending
    FILE *file = fopen(filePath, "a");
    if (file == NULL)
    {
        fprintf(stderr, "Error opening file: %s\n", filePath);
        return 0;
    }

    // Get the current time
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    // Write the message to the file
    fprintf(file, "%s", full_buff_from);
    fprintf(file, "%s", full_buff_to);
    fprintf(file, "%s", full_buff_subject);
    fprintf(file, "Received: %02d:%02d:%02d %02d/%02d/%04d\n", t->tm_hour, t->tm_min, t->tm_sec, t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);
    fprintf(file, "%s\n\n", full_buff_message);

    // Close the file
    fclose(file);
    return 1;
}
