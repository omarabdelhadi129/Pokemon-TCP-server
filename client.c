#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 80

//Forward refrence
void displayMenu();
void option3(int number);
void dontDisplayMenu();

int main()
{
    int clientSocket, addrSize, bytesReceived, numQuiries;
    struct sockaddr_in clientAddr;

    char inStr[80];  // stores user input from keyboard
    char buffer[80]; // stores sent and received data

    // Create socket
    clientSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (clientSocket < 0)
    {
        printf("*** CLIENT ERROR: Could open socket.\n");
        exit(-1);
    }

    // Setup address
    memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
    clientAddr.sin_port = htons((unsigned short)SERVER_PORT);

    inStr[0] = "f";
    // Go into loop to commuincate with server now
    while (1)
    {
        // Get a command from the user
        if ((strcmp(inStr, "a") == 0) || (strcmp(inStr, "b") == 0))
        {
            dontDisplayMenu();
        }
        else
        {
            displayMenu();
        }

        if (inStr == NULL)
        {
            displayMenu();
        }

        scanf("%s", inStr);

        // Send command string to server
        strcpy(buffer, inStr);
        printf("CLIENT: Sending \"%s\" to server.\n", buffer);
        sendto(clientSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));

        // //Checking if user wants to quit
        // if ((strcmp(inStr, "c") == 0))
        // {
        //     break;
        // }

        // Get response from server
        addrSize = sizeof(clientAddr);
        bytesReceived = recvfrom(clientSocket, buffer, 80, 0, (struct sockaddr *)&clientAddr, &addrSize);
        buffer[bytesReceived] = 0;
        // printf("CLIENT: Got back response \"%s\" from server.\n", buffer);

        if ((strcmp(inStr, "c") == 0))
        {
            numQuiries = atoi(buffer);
            break;
        }

        printf("CLIENT: Got back response \"%s\" from server.\n", buffer);
    }

    // Close socket and print exit message
    option3(numQuiries);
    close(clientSocket);
    printf("CLIENT: Shutting down.\n");

    return EXIT_SUCCESS;
}

void displayMenu()
{
    printf("CLIENT: Please choose one of the following options:\n");
    printf("Enter [a] for Type Search\nEnter [b] to Save results\nEnter [c] to Exit the program\n");
}

void option3(int number)
{
    printf("--------------------------------\n");
    printf("Total number of quiries carried out: %d\n", number);
    printf("Here are all the files in the current directory after creation of files: \n");
    system("ls");
}

void dontDisplayMenu()
{
}