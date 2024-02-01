#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_BUFFER_SIZE 10240

// Function to send mail using SMTP protocol
void sendMail(const char* serverIP, int smtpPort);

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server_IP> <smtp_port> <pop3_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char* serverIP = argv[1];
    int smtpPort = atoi(argv[2]);
    int pop3Port = atoi(argv[3]);

    char username[256];
    char password[256];

    printf("Enter username: ");
    scanf("%s", username);

    printf("Enter password: ");
    scanf("%s", password);

    while (1) {
        int option;

        printf("\nOptions:\n");
        printf("1. Manage Mail\n");
        printf("2. Send Mail\n");
        printf("3. Quit\n");

        printf("Enter your choice (1-3): ");
        scanf("%d", &option);

        switch (option) {
            case 1:
                // Manage Mail: Implement logic to show stored mails
                printf("Not implemented yet.\n");
                break;

            case 2:
                // Send Mail: Implement logic to send mail using SMTP
                sendMail(serverIP, smtpPort);
                break;

            case 3:
                // Quit
                printf("Quitting the program.\n");
                exit(EXIT_SUCCESS);

            default:
                printf("Invalid option. Please choose again.\n");
                break;
        }
    }

    return 0;
}
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

    // Loop until the entire message is received
    while ((bytesRead = read(clientSocket, buffer + totalBytesRead, sizeof(buffer) - totalBytesRead - 1)) > 0) {
        totalBytesRead += bytesRead;
        // Check if the message is complete (you need some condition based on your protocol)
        if (strstr(buffer, "\0") != NULL) {
            break;  // Message complete, exit the loop
        }

        // Ensure there is space for the null terminator
        if (totalBytesRead >= sizeof(buffer) - 1) {
            printf("\n%d\n",totalBytesRead);
            fprintf(stderr, "Message too large for buffer\n");
            close(clientSocket);
            exit(EXIT_FAILURE);
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
void extractField(const char *input, const char *fieldName, char *result) {
    // Find the position of the field header
    const char *fieldHeader = strstr(input, fieldName);
    
    if (fieldHeader != NULL) {
        // Move the pointer to the start of the field value
        const char *fieldStart = fieldHeader + strlen(fieldName);
        
        // Find the end of the field value
        const char *fieldEnd = strpbrk(fieldStart, "\n");
        
        if (fieldEnd != NULL) {
            // Calculate the length of the field value
            size_t fieldLength = fieldEnd - fieldStart;
            
            // Extract the field value
            strncpy(result, fieldStart, fieldLength);
            result[fieldLength] = '\0';
        }
    }
}
void sendMail(const char* serverIP, int smtpPort) {
    // Create socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(smtpPort);
    if (inet_pton(AF_INET, serverIP, &(serverAddr.sin_addr)) <= 0) {
        perror("Invalid server IP address");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    // Connect to the SMTP server
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Error connecting to server");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    char mail[MAX_BUFFER_SIZE] = "";
    char buffer[MAX_BUFFER_SIZE];
    printf("Enter the mail in the specified format (end with a single fullstop line):\n");

    // Read mail input until a line with a single fullstop is encountered
    while (1) {
        fgets(buffer, sizeof(buffer), stdin);
    // Check if the last two characters are ".\n"
        if (strcmp(buffer, ".\n") == 0) {
            break;
        }
        strcat(mail, buffer);

    }
    mail[strlen(mail)]='\0';
    printf("mail: %s",mail);
    // const char *mailContent = "From: agupta@10.5.20.22\nTo: ag@10.5.21.32\nSubject: This week’s lab assignment\nThe course has just started.\nThe students are eager to do assignments (ha ha ha!!)\nLet us give them something.";

    // Check the format of From, To, and Subject fields
    char from[256], to[256], subject[256], body[1024];

    extractField(mail, "From:", from);
    extractField(mail, "To:", to);
    // extractField(mail, "Subject:", subject);

    const char *subjectHeader = strstr(mail, "Subject:");
    if (subjectHeader != NULL) {
        // Move the pointer to the start of the subject
        const char *subjectStart = subjectHeader + strlen("Subject:");
        // Find the end of the subject (until the end of the line)
        const char *subjectEnd = strchr(subjectStart, '\n');

        if (subjectEnd != NULL) {
            // Calculate the length of the subject
            size_t subjectLength = subjectEnd - subjectStart;

            // Extract the subject
            strncpy(subject, subjectStart, subjectLength);
            subject[subjectLength] = '\0';


            // Extract the mail body from the line after the subject
            const char *bodyStart = subjectEnd + 1;
            strncpy(body, bodyStart, sizeof(body) - 1);
            body[sizeof(body)-1]='\0';
        }
    }

  
    printf("From: %s\n", from);
    printf("To: %s\n", to);
    printf("Subject: %s\n", subject);
    printf("body: %s\n", body);

    // if (sscanf(mail, "From: %[^\n]\nTo: %[^\n]\nSubject: %[^\n]", from, to, subject) != 3 ||
    //     strchr(from, '@') == NULL || strchr(to, '@') == NULL) {
    //     // Incorrect format
    //     printf("Incorrect format. Please enter the mail again.\n");
    //     close(clientSocket);
    //     return;
    // }

    bzero(buffer,sizeof(buffer));
    // Read the initial response from the server
    if (read(clientSocket, buffer, sizeof(buffer)) == -1) {
        perror("Error reading server response");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    // Print the initial response
    printf("S: %s\n", buffer);
    bzero(buffer,sizeof(buffer));

    // Send HELO command
    char command[MAX_BUFFER_SIZE];
    snprintf(command, sizeof(command), "HELO %s\n", serverIP);
    sendCommand(clientSocket,command);
    // Read the response to HELO command
    
    recvResponse(clientSocket,buffer);

    // Print the response to HELO command
    printf("S: %s\n", buffer);

    // Implement the rest of the SMTP commands (MAIL, RCPT, DATA, QUIT) following the example
    // You need to read user input and send the corresponding SMTP commands
    // Read the responses from the server and print them

    // Close the client socket
    close(clientSocket);
}

