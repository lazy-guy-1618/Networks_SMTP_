    while ((bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';  // Null-terminate the received data

        // Extract information from the received mail
        char subject[MAX_SUBJECT_SIZE + 1];
        sscanf(buffer, "Subject: %[^\n]", subject);

        // Create the user subdirectory
        createUserDirectory(username);

        // Create the mailbox entry
        char mailboxEntry[MAX_BUFFER_SIZE];
        snprintf(mailboxEntry, sizeof(mailboxEntry),
                 "From: %s@%s\nTo: %s@%s\nSubject: %s\nReceived: %04d-%02d-%02d : %02d : %02d\n%s\n.",
                 username, domain, username, domain, subject,
                 info->tm_year + 1900, info->tm_mon + 1, info->tm_mday, info->tm_hour, info->tm_min, buffer);

        // Append the entry to the mailbox file
        char mailboxPath[256];
        snprintf(mailboxPath, sizeof(mailboxPath), "./%s/mymailbox", username);

        FILE *mailboxFile = fopen(mailboxPath, "a");
        if (mailboxFile == NULL) {
            perror("Error opening mymailbox file");
            return;
        }

        fputs(mailboxEntry, mailboxFile);
        fclose(mailboxFile);

        // Acknowledge receipt to the client
        const char* ackMessage = "Mail received successfully!\n";
        write(clientSocket, ackMessage, strlen(ackMessage));
    }

