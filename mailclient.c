#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#define MAX_BUFFER_SIZE 10240

// Function to send mail using SMTP protocol
void sendMail(const char *serverIP, int smtpPort)
{

    // Ensure the usr input is valid before making a connection to the server
    char mail[MAX_BUFFER_SIZE] = "";
    char buffer[MAX_BUFFER_SIZE];
    printf("Enter the mail in the specified format (end with a single fullstop line):\n");

    // Read mail input until a line with a single fullstop is encountered
    while (1)
    {
        fgets(buffer, sizeof(buffer), stdin);
        // Check if the last two characters are ".\n"
        if (strcmp(buffer, ".\n") == 0)
        {
            break;
        }
        strcat(mail, buffer);
    }
    mail[strlen(mail)] = '\0';

    // Check the format of From, To, and Subject fields
    char from[256], to[256], subject[256], body[1024];

    extractField(mail, "From: ", from);
    extractField(mail, "To: ", to);
    extractField(mail, "Subject: ", subject);
    extractMessage(mail, body);

    if (!isValidEmail(from) || !isValidEmail(to) || !isValidSubject(subject))
    {
        printf("Incorrect format\n");
        return;
    }

    char *fromPos = strstr(mail, "From: ");
    char *toPos = strstr(mail, "To: ");
    char *subjectPos = strstr(mail, "Subject: ");

    if (fromPos && toPos && subjectPos)
    {
        if (!(fromPos < toPos && toPos < subjectPos))
        {
            printf("Incorrect format\n");
            return;
        }
    }
    else
    {
        printf("Incorrect format\n");
        return;
    }
    // ----------------------------------------------------------------

    // Create socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1)
    {
        perror("Error creating socket");
        return;
    }

    // Set up server address structure
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(smtpPort);
    if (inet_pton(AF_INET, serverIP, &(serverAddr.sin_addr)) <= 0)
    {
        perror("Invalid server IP address");
        close(clientSocket);
        return;
    }

    // Connect to the SMTP server
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("Error connecting to server");
        close(clientSocket);
        return;
    }

    // ----------------------------------------------------------------
    int index = 0;
    // Read the initial response from the server
    char temp_buff[100];
    char full_buff[MAX_BUFFER_SIZE] = {0}; // Increase size as needed

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
    index = 0;

    // Check the full response
    if (strncmp(full_buff, "220 ", 4) != 0)
    {
        fprintf(stderr, "Error: Unexpected response from server: %s\n", full_buff);
        send(clientSocket, "QUIT\r\n", 6, 0);
        close(clientSocket);
        return;
    }

    // Print the full response
    // printf("S: %s\n", full_buff);

    // Extract the domain name
    char domainName[256];
    if (sscanf(full_buff, "220 <%255[^>]> Service Ready", domainName) != 1)
    {
        fprintf(stderr, "Error: Could not extract domain name from server response: %s\n", full_buff);
        send(clientSocket, "QUIT\r\n", 6, 0);
        close(clientSocket);
        return;
    }

    // Print the initial response and the extracted domain name
    // printf("S: %s\n", full_buff);
    // printf("Extracted domain name: %s\n", domainName);
    memset(full_buff, 0, sizeof(full_buff));

    // ----------------------------------------------------------------

    // Send HELO command
    char command[MAX_BUFFER_SIZE];
    snprintf(command, sizeof(command), "HELO %s\r\n", domainName);

    int commandLength = strlen(command);
    int bytesSent = 0;
    int chunkSize;

    while (bytesSent < commandLength)
    {

        memset(temp_buff, 0, sizeof(temp_buff));

        // Determine the size of the next chunk
        chunkSize = commandLength - bytesSent;
        if (chunkSize > sizeof(temp_buff) - 1)
        {
            chunkSize = sizeof(temp_buff) - 1;
        }

        // Copy the next chunk into temp_buff and null-terminate it
        memcpy(temp_buff, &command[bytesSent], chunkSize);
        temp_buff[chunkSize] = '\0';

        // Send the chunk
        if (send(clientSocket, temp_buff, strlen(temp_buff), 0) == -1)
        {
            perror("Error sending HELO command");
            send(clientSocket, "QUIT\r\n", 6, 0);
            close(clientSocket);
            return;
        }

        // Update bytesSent
        bytesSent += chunkSize;

        // Break if the last chunk sent ends with "\r\n"
        if (chunkSize >= 2 && strcmp(&temp_buff[chunkSize - 2], "\r\n") == 0)
        {
            break;
        }
    }
    bytesSent = 0;
    chunkSize = 0;

    // Read the response to HELO command
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
    index = 0;

    // Check the full response
    if (strncmp(full_buff, "250 ", 4) != 0)
    {
        fprintf(stderr, "Error: Unexpected response from server: %s\n", full_buff);
        send(clientSocket, "QUIT\r\n", 6, 0);
        close(clientSocket);
        return;
    }

    // Print the full response
    // printf("S: %s\n", full_buff);
    memset(full_buff, 0, sizeof(full_buff));
    memset(command, 0, sizeof(command));

    // ----------------------------------------------------------------

    // Send MAIL FROM command
    snprintf(command, sizeof(command), "MAIL FROM: <%s>\r\n", from);

    commandLength = strlen(command);

    while (bytesSent < commandLength)
    {

        memset(temp_buff, 0, sizeof(temp_buff));

        // Determine the size of the next chunk
        chunkSize = commandLength - bytesSent;
        if (chunkSize > sizeof(temp_buff) - 1)
        {
            chunkSize = sizeof(temp_buff) - 1;
        }

        // Copy the next chunk into temp_buff and null-terminate it
        memcpy(temp_buff, &command[bytesSent], chunkSize);
        temp_buff[chunkSize] = '\0';

        // Send the chunk
        if (send(clientSocket, temp_buff, strlen(temp_buff), 0) == -1)
        {
            perror("Error sending MAIL FROM command");
            send(clientSocket, "QUIT\r\n", 6, 0);
            close(clientSocket);
            return;
        }

        // Update bytesSent
        bytesSent += chunkSize;

        // Break if the last chunk sent ends with "\r\n"
        if (chunkSize >= 2 && strcmp(&temp_buff[chunkSize - 2], "\r\n") == 0)
        {
            break;
        }
    }
    bytesSent = 0;
    chunkSize = 0;

    // Read the response to MAIL FROM command
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
    index = 0;

    // Check the full response
    if (strncmp(full_buff, "250 ", 4) != 0)
    {
        fprintf(stderr, "Error: Unexpected response from server: %s\n", full_buff);
        send(clientSocket, "QUIT\r\n", 6, 0);
        close(clientSocket);
        return;
    }

    // Print the full response
    // printf("S: %s\n", full_buff);
    memset(full_buff, 0, sizeof(full_buff));
    memset(command, 0, sizeof(command));

    // ----------------------------------------------------------------

    // Send RCPT TO command
    snprintf(command, sizeof(command), "RCPT TO: %s\r\n", to);

    commandLength = strlen(command);

    while (bytesSent < commandLength)
    {

        memset(temp_buff, 0, sizeof(temp_buff));

        // Determine the size of the next chunk
        chunkSize = commandLength - bytesSent;
        if (chunkSize > sizeof(temp_buff) - 1)
        {
            chunkSize = sizeof(temp_buff) - 1;
        }

        // Copy the next chunk into temp_buff and null-terminate it
        memcpy(temp_buff, &command[bytesSent], chunkSize);
        temp_buff[chunkSize] = '\0';

        // Send the chunk
        if (send(clientSocket, temp_buff, strlen(temp_buff), 0) == -1)
        {
            perror("Error sending RCPT TO command");
            send(clientSocket, "QUIT\r\n", 6, 0);
            close(clientSocket);
            return;
        }

        // Update bytesSent
        bytesSent += chunkSize;

        // Break if the last chunk sent ends with "\r\n"
        if (chunkSize >= 2 && strcmp(&temp_buff[chunkSize - 2], "\r\n") == 0)
        {
            break;
        }
    }
    bytesSent = 0;
    chunkSize = 0;

    // Read the response to RCPT TO command
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
    index = 0;

    // Check the full response
    if (strncmp(full_buff, "550 ", 4) == 0)
    {
        fprintf(stderr, "Error: %s\n", full_buff);
        send(clientSocket, "QUIT\r\n", 6, 0);
        close(clientSocket);
        return;
    }
    else if (strncmp(full_buff, "250 ", 4) != 0)
    {
        fprintf(stderr, "Error: Unexpected response from server: %s\n", full_buff);
        send(clientSocket, "QUIT\r\n", 6, 0);
        close(clientSocket);
        return;
    }

    // Print the full response
    // printf("S: %s\n", full_buff);
    memset(full_buff, 0, sizeof(full_buff));
    memset(command, 0, sizeof(command));

    // ----------------------------------------------------------------

    // Send DATA command
    snprintf(command, sizeof(command), "DATA\r\n");

    commandLength = strlen(command);

    while (bytesSent < commandLength)
    {

        memset(temp_buff, 0, sizeof(temp_buff));

        // Determine the size of the next chunk
        chunkSize = commandLength - bytesSent;
        if (chunkSize > sizeof(temp_buff) - 1)
        {
            chunkSize = sizeof(temp_buff) - 1;
        }

        // Copy the next chunk into temp_buff and null-terminate it
        memcpy(temp_buff, &command[bytesSent], chunkSize);
        temp_buff[chunkSize] = '\0';

        // Send the chunk
        if (send(clientSocket, temp_buff, strlen(temp_buff), 0) == -1)
        {
            perror("Error sending DATA command");
            send(clientSocket, "QUIT\r\n", 6, 0);
            close(clientSocket);
            return;
        }

        // Update bytesSent
        bytesSent += chunkSize;

        // Break if the last chunk sent ends with "\r\n"
        if (chunkSize >= 2 && strcmp(&temp_buff[chunkSize - 2], "\r\n") == 0)
        {
            break;
        }
    }
    bytesSent = 0;
    chunkSize = 0;

    // Read the response to DATA command
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
    index = 0;

    // Check the full response
    if (strncmp(full_buff, "354 ", 4) != 0)
    {
        fprintf(stderr, "Error: Unexpected response from server: %s\n", full_buff);
        send(clientSocket, "QUIT\r\n", 6, 0);
        close(clientSocket);
        return;
    }

    // Print the full response
    // printf("S: %s\n", full_buff);
    memset(full_buff, 0, sizeof(full_buff));
    memset(command, 0, sizeof(command));

    // ----------------------------------------------------------------

    // Send mail content
    // Send FROM
    char temp_buff_small[100];
    char temp_command[MAX_BUFFER_SIZE];

    // Prepare the temp_command
    snprintf(temp_command, sizeof(temp_command), "From: %s\r\n", from);

    commandLength = strlen(temp_command);

    while (bytesSent < commandLength)
    {
        // Clear the temp_buff_small
        memset(temp_buff_small, 0, sizeof(temp_buff_small));

        // Determine the size of the next chunk
        chunkSize = commandLength - bytesSent;
        if (chunkSize > sizeof(temp_buff_small) - 1)
        {
            chunkSize = sizeof(temp_buff_small) - 1;
        }

        // Copy the next chunk into temp_buff_small and null-terminate it
        memcpy(temp_buff_small, &temp_command[bytesSent], chunkSize);
        temp_buff_small[chunkSize] = '\0';

        // Send the chunk
        if (send(clientSocket, temp_buff_small, strlen(temp_buff_small), 0) == -1)
        {
            perror("Error sending FROM command");
            send(clientSocket, "QUIT\r\n", 6, 0);
            close(clientSocket);
            return;
        }

        // Update bytesSent
        bytesSent += chunkSize;

        // Break if the last chunk sent ends with "\r\n"
        if (chunkSize >= 2 && strcmp(&temp_buff_small[chunkSize - 2], "\r\n") == 0)
        {
            break;
        }
    }
    bytesSent = 0;
    chunkSize = 0;
    memset(temp_command, 0, sizeof(temp_command));

    // printf("From sent successfully\n");
    // Send TO
    snprintf(temp_command, sizeof(temp_command), "To: %s\r\n", to);

    commandLength = strlen(temp_command);

    while (bytesSent < commandLength)
    {
        // Clear the temp_buff_small
        memset(temp_buff_small, 0, sizeof(temp_buff_small));

        // Determine the size of the next chunk
        chunkSize = commandLength - bytesSent;
        if (chunkSize > sizeof(temp_buff_small) - 1)
        {
            chunkSize = sizeof(temp_buff_small) - 1;
        }

        // Copy the next chunk into temp_buff_small and null-terminate it
        memcpy(temp_buff_small, &temp_command[bytesSent], chunkSize);
        temp_buff_small[chunkSize] = '\0';

        // Send the chunk
        if (send(clientSocket, temp_buff_small, strlen(temp_buff_small), 0) == -1)
        {
            perror("Error sending TO command");
            send(clientSocket, "QUIT\r\n", 6, 0);
            close(clientSocket);
            return;
        }

        // Update bytesSent
        bytesSent += chunkSize;

        // Break if the last chunk sent ends with "\r\n"
        if (chunkSize >= 2 && strcmp(&temp_buff_small[chunkSize - 2], "\r\n") == 0)
        {
            break;
        }
    }
    bytesSent = 0;
    chunkSize = 0;
    memset(temp_command, 0, sizeof(temp_command));
    // printf("To sent successfully\n");

    // Send SUBJECT
    snprintf(temp_command, sizeof(temp_command), "Subject: %s\r\n", subject);

    commandLength = strlen(temp_command);

    while (bytesSent < commandLength)
    {
        // Clear the temp_buff_small
        memset(temp_buff_small, 0, sizeof(temp_buff_small));

        // Determine the size of the next chunk
        chunkSize = commandLength - bytesSent;
        if (chunkSize > sizeof(temp_buff_small) - 1)
        {
            chunkSize = sizeof(temp_buff_small) - 1;
        }

        // Copy the next chunk into temp_buff_small and null-terminate it
        memcpy(temp_buff_small, &temp_command[bytesSent], chunkSize);
        temp_buff_small[chunkSize] = '\0';

        // Send the chunk
        if (send(clientSocket, temp_buff_small, strlen(temp_buff_small), 0) == -1)
        {
            perror("Error sending SUBJECT command");
            send(clientSocket, "QUIT\r\n", 6, 0);
            close(clientSocket);
            return;
        }

        // Update bytesSent
        bytesSent += chunkSize;

        // Break if the last chunk sent ends with "\r\n"
        if (chunkSize >= 2 && strcmp(&temp_buff_small[chunkSize - 2], "\r\n") == 0)
        {
            break;
        }
    }
    bytesSent = 0;
    chunkSize = 0;
    memset(temp_command, 0, sizeof(temp_command));
    // printf("Subject sent successfully\n");

    // Send BODY
    char *line = strtok(body, "\n");
    while (line != NULL)
    {
        // Prepare the command
        snprintf(temp_command, sizeof(temp_command), "%s\r\n", line);

        commandLength = strlen(temp_command);

        while (bytesSent < commandLength)
        {
            // Clear the temp_buff_small
            memset(temp_buff_small, 0, sizeof(temp_buff_small));

            // Determine the size of the next chunk
            chunkSize = commandLength - bytesSent;
            if (chunkSize > sizeof(temp_buff_small) - 1)
            {
                chunkSize = sizeof(temp_buff_small) - 1;
            }

            // Copy the next chunk into temp_buff_small and null-terminate it
            memcpy(temp_buff_small, &temp_command[bytesSent], chunkSize);
            temp_buff_small[chunkSize] = '\0';

            // Send the chunk
            if (send(clientSocket, temp_buff_small, strlen(temp_buff_small), 0) == -1)
            {
                perror("Error sending BODY line");
                send(clientSocket, "QUIT\r\n", 6, 0);
                close(clientSocket);
                return;
            }

            // Update bytesSent
            bytesSent += chunkSize;

            // Break if the last chunk sent ends with "\r\n"
            if (chunkSize >= 2 && strcmp(&temp_buff_small[chunkSize - 2], "\r\n") == 0)
            {
                break;
            }
        }
        bytesSent = 0;
        chunkSize = 0;

        line = strtok(NULL, "\n");
    }
    memset(temp_command, 0, sizeof(temp_command));
    // printf("Body sent successfully\n");
    // ----------------------------------------------------------------

    // Send end of transmission
    snprintf(command, sizeof(command), ".\r\n");

    commandLength = strlen(command);

    while (bytesSent < commandLength)
    {

        memset(temp_buff, 0, sizeof(temp_buff));

        // Determine the size of the next chunk
        chunkSize = commandLength - bytesSent;
        if (chunkSize > sizeof(temp_buff) - 1)
        {
            chunkSize = sizeof(temp_buff) - 1;
        }

        // Copy the next chunk into temp_buff and null-terminate it
        memcpy(temp_buff, &command[bytesSent], chunkSize);
        temp_buff[chunkSize] = '\0';

        // Send the chunk
        if (send(clientSocket, temp_buff, strlen(temp_buff), 0) == -1)
        {
            perror("Error sending DATA command");
            send(clientSocket, "QUIT\r\n", 6, 0);
            close(clientSocket);
            return;
        }

        // Update bytesSent
        bytesSent += chunkSize;

        // Break if the last chunk sent ends with "\r\n"
        if (chunkSize >= 2 && strcmp(&temp_buff[chunkSize - 2], "\r\n") == 0)
        {
            break;
        }
    }
    bytesSent = 0;
    chunkSize = 0;

    // printf("End of transmission sent successfully\n");

    // Read the response to end of transmission
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
    index = 0;

    // Check the full response
    if (strncmp(full_buff, "250 ", 4) != 0)
    {
        fprintf(stderr, "Error: Unexpected response from server: %s\n", full_buff);
        send(clientSocket, "QUIT\r\n", 6, 0);
        close(clientSocket);
        return;
    }

    // Print the full response
    // printf("S: %s\n", full_buff);
    memset(full_buff, 0, sizeof(full_buff));
    memset(command, 0, sizeof(command));

    // ----------------------------------------------------------------

    // Send QUIT command
    snprintf(command, sizeof(command), "QUIT\r\n");

    commandLength = strlen(command);

    while (bytesSent < commandLength)
    {

        memset(temp_buff, 0, sizeof(temp_buff));

        // Determine the size of the next chunk
        chunkSize = commandLength - bytesSent;
        if (chunkSize > sizeof(temp_buff) - 1)
        {
            chunkSize = sizeof(temp_buff) - 1;
        }

        // Copy the next chunk into temp_buff and null-terminate it
        memcpy(temp_buff, &command[bytesSent], chunkSize);
        temp_buff[chunkSize] = '\0';

        // Send the chunk
        if (send(clientSocket, temp_buff, strlen(temp_buff), 0) == -1)
        {
            perror("Error sending QUIT command");
            close(clientSocket);
            return;
        }

        // Update bytesSent
        bytesSent += chunkSize;

        // Break if the last chunk sent ends with "\r\n"
        if (chunkSize >= 2 && strcmp(&temp_buff[chunkSize - 2], "\r\n") == 0)
        {
            break;
        }
    }
    bytesSent = 0;
    chunkSize = 0;

    // printf("QUIT sent successfully\n");


    // Read the response to QUIT command
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
    index = 0;
    // Print the full response
    // printf("S: %s\n", full_buff);
    memset(command, 0, sizeof(command));

    // Check the full response
    if (strncmp(full_buff, "221 ", 4) == 0)
    {
        printf("Mail sent successfully\n");
        memset(full_buff, 0, sizeof(full_buff));
        close(clientSocket);
        return;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <server_IP> <smtp_port> <pop3_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *serverIP = argv[1];
    int smtpPort = atoi(argv[2]);
    int pop3Port = atoi(argv[3]);

    char username[256];
    char password[256];

    printf("Enter username: ");
    scanf("%s", username);

    printf("Enter password: ");
    scanf("%s", password);

    while (1)
    {
        int option;

        printf("\nOptions:\n");
        printf("1. Manage Mail\n");
        printf("2. Send Mail\n");
        printf("3. Quit\n");

        printf("Enter your choice (1-3): ");
        scanf("%d", &option);

        switch (option)
        {
        case 1:
            // Manage Mail: Implement logic to show stored mails
            accessMailbox(serverIP,pop3Port,username,password);
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

// ----------------------------------------------------------------

void extractField(char *mail, char *field, char *output)
{
    char *line = strstr(mail, field);
    if (line)
    {
        char *start = line + strlen(field);
        char *end = strchr(start, '\n');
        if (end)
        {
            strncpy(output, start, end - start);
            output[end - start] = '\0'; // Null terminate the copied string
        }
    }
}

void extractMessage(char *mail, char *output)
{
    char *start = strstr(mail, "Subject: ");
    if (start)
    {
        start = strchr(start, '\n'); // Find the end of the Subject line
        if (start)
        {
            start += 1; // Skip the newline character
            char *end = strstr(start, "\n.\n");
            if (end)
            {
                strncpy(output, start, end - start);
                output[end - start] = '\0'; // Null terminate the copied string
            }
            else
            {
                // If there's no line with just '.', copy everything till the end
                strcpy(output, start);
            }
        }
    }
}

int isValidNumber(char *str)
{
    int len = strlen(str);
    for (int i = 0; i < len; i++)
    {
        if (!isdigit(str[i]))
        {
            return 0;
        }
    }
    return 1;
}

int isValidIP(char *ip)
{
    int num, dots = 0;
    char *ptr;
    char *ip_copy = strdup(ip); // Make a copy of the string

    if (ip_copy == NULL)
        return 0;

    ptr = strtok(ip_copy, ".");

    if (ptr == NULL)
        return 0;

    while (ptr)
    {
        /* after parsing string, it must contain only digits */
        if (!isValidNumber(ptr))
            return 0;

        num = atoi(ptr);

        /* check for valid IP */
        if (num >= 0 && num <= 255)
        {
            /* parse remaining string */
            ptr = strtok(NULL, ".");
            if (ptr != NULL)
                ++dots;
        }
        else
            return 0;
    }

    /* valid IP string must contain 3 dots */
    if (dots != 3)
        return 0;
    free(ip_copy); // Free the copied string
    return 1;
}

int isValidEmail(char *email)
{
    char *at = strchr(email, '@');
    if (at == NULL)
    {
        return 0;
    }
    char *domain = at + 1;
    if (isValidIP(domain))
    {
        return 1;
    }
    else
    {
        char *dot = strchr(domain, '.');
        if (dot == NULL || strchr(dot + 1, '.') != NULL)
        {
            return 0;
        }
        return 1;
    }
}

int isValidSubject(char *subject)
{
    return strlen(subject) <= 50;
}
void recvMessage(int clientSocket,char* full_buff)
{
    int index = 0;
    memset(full_buff, 0, sizeof(full_buff));

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
    full_buff[index-2]='\0';
    full_buff[index-1]='\0';
    index = 0;
        // printf("Received: %s\n",full_buff);

}
void sendMessage(int clientSocket,char* command)
{
      int commandLength = strlen(command);
    int bytesSent = 0;
    int chunkSize;
        char temp_buff[100];


    while (bytesSent < commandLength)
    {

        memset(temp_buff, 0, sizeof(temp_buff));

        // Determine the size of the next chunk
        chunkSize = commandLength - bytesSent;
        if (chunkSize > sizeof(temp_buff) - 1)
        {
            chunkSize = sizeof(temp_buff) - 1;
        }

        // Copy the next chunk into temp_buff and null-terminate it
        memcpy(temp_buff, &command[bytesSent], chunkSize);
        temp_buff[chunkSize] = '\0';

        // Send the chunk
        if (send(clientSocket, temp_buff, strlen(temp_buff), 0) == -1)
        {
            perror("Error sending HELO command");
            send(clientSocket, "QUIT\r\n", 6, 0);
            close(clientSocket);
            return;
        }

        // Update bytesSent
        bytesSent += chunkSize;

        // Break if the last chunk sent ends with "\r\n"
        if (chunkSize >= 2 && strcmp(&temp_buff[chunkSize - 2], "\r\n") == 0)
        {
            break;
        }
    }
}
// ----------------------------------------------------------------
void deleteMail(int clientSocket,int eno)
{
        char command[100],full_buff[40];
        memset(command, 0, sizeof(command));
        snprintf(command, sizeof(command), "DELE %d\r\n",eno);
        sendMessage(clientSocket,command);
        
       
        recvMessage(clientSocket,full_buff);
        if(strncmp(full_buff, "-ERR", 4)==0)
        {
            fprintf(stderr,"Error: %s",full_buff);
        }
        else if(strncmp(full_buff, "+OK", 3)==0)
        {
            printf("Message Deleted\n");
        }
        else
        {
            fprintf(stderr,"Unexpected response: %s",full_buff);

        }

}
void retrieveMail(int clientSocket,int eno)
{
        char command[100],full_buff[MAX_BUFFER_SIZE];
        memset(command, 0, sizeof(command));
        snprintf(command, sizeof(command), "RETR %d\r\n",eno);
        sendMessage(clientSocket,command);
        do
        {
        recvMessage(clientSocket,full_buff);
        if(strncmp(full_buff, "-ERR", 4)==0)
        {
            fprintf(stderr,"Error: %s",full_buff);
        }
        else if(strlen(full_buff)>1 && strncmp(full_buff, "+OK", 3)!=0)
        {
            printf("%s\n",full_buff);
        }
        } while (strncmp(full_buff, ".", 1) != 0);

}
void accessMailbox(const char *serverIP, int pop3port,char* username,char* password)
{


    // Create socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1)
    {
        perror("Error creating socket");
        return;
    }

    // Set up server address structure
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(pop3port);
    if (inet_pton(AF_INET, serverIP, &(serverAddr.sin_addr)) <= 0)
    {
        perror("Invalid server IP address");
        close(clientSocket);
        return;
    }

    // Connect to the SMTP server
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("Error connecting to server");
        close(clientSocket);
        return;
    }
    // Read the initial response from the server
    char temp_buff[100];
    char full_buff[MAX_BUFFER_SIZE] = {0}; // Increase size as needed
    char command[MAX_BUFFER_SIZE];

    // ----------------------------------------------------------------
    recvMessage(clientSocket,full_buff);
     if (strncmp(full_buff, "-ERR", 4) == 0)
    {
        fprintf(stderr, "Error: %s\n", full_buff);
        send(clientSocket, "QUIT\r\n", 6, 0);
        close(clientSocket);
        return;
    }
    else if (strncmp(full_buff, "+OK", 3) != 0)
    {
        fprintf(stderr, "Error: Unexpected response from server: %s\n", full_buff);
        send(clientSocket, "QUIT\r\n", 6, 0);
        close(clientSocket);
        return;
    }

    snprintf(command, sizeof(command), "USER: %s\r\n", username);
    sendMessage(clientSocket,command);
    // ----------------------------------------------
    recvMessage(clientSocket,full_buff);
    if (strncmp(full_buff, "-ERR", 4) == 0)
    {
        fprintf(stderr, "Error: %s\n", full_buff);
        send(clientSocket, "QUIT\r\n", 6, 0);
        close(clientSocket);
        return;
    }
    else if (strncmp(full_buff, "+OK", 3) != 0)
    {
        fprintf(stderr, "Error: Unexpected response from server: %s\n", full_buff);
        send(clientSocket, "QUIT\r\n", 6, 0);
        close(clientSocket);
        return;
    }

    memset(command, 0, sizeof(command));

    // ----------------------------------------------------------------

   snprintf(command, sizeof(command), "PASS: %s\r\n", password);
    sendMessage(clientSocket,command);
    // ----------------------------------------------
    recvMessage(clientSocket,full_buff);
    if (strncmp(full_buff, "-ERR", 4) == 0)
    {
        fprintf(stderr, "Error: %s\n", full_buff);
        send(clientSocket, "QUIT\r\n", 6, 0);
        close(clientSocket);
        return;
    }
    else if (strncmp(full_buff, "+OK", 3) != 0)
    {
        fprintf(stderr, "Error: Unexpected response from server: %s\n", full_buff);
        send(clientSocket, "QUIT\r\n", 6, 0);
        close(clientSocket);
        return;
    }
    int eno=0;
    int numEmails=0;
    while(1)
    {
        memset(command, 0, sizeof(command));
        snprintf(command, sizeof(command), "NOOP\r\n");
        sendMessage(clientSocket,command);
        numEmails=0;
        do
        {
        recvMessage(clientSocket,full_buff);
        if(strcmp(full_buff,".")!=0 && strncmp(full_buff, "+OK", 3)!=0)
        {
            printf("%s\n",full_buff);
            numEmails++;
        }
        } while (strncmp(full_buff, ".", 1) != 0);
        // printf("Number of emails: %d \n",numEmails);
        printf("Enter mail no. to see: ");
        scanf("%d",&eno);

        while(eno<-1 || eno==0 || eno>numEmails)
        {
        printf("Mail no. out of range, give again");
        scanf("%d",&eno);
        }
        if(eno==-1)break;

        retrieveMail(clientSocket,eno);
        char c=getchar();
        if(c=='d')
        {
            deleteMail(clientSocket,eno);
        }
        

    }

    // printf("end of pop3 client\n");
}
