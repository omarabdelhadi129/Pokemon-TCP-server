#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

// Server port
#define SERVER_PORT 80
// String lengths
#define MAX_LENGTH_OF_LINE 100
#define MAX_USER_INPUT 25
#define MAX_NUMTYPE_POKEMON 18

// Data structure container
typedef struct dataContainerType
{
    FILE *mainFile;
    FILE *createFile;
    char userType[25];
    char **storedResults;
    int returnedLines;
} dataContainer;

//Forward refrence
void displayMenu();
void *save_thread(void *arg);
void *search_thread(void *arg);

//Global counter for number of quiries completed
int NUMBER_OF_QUERIES = 0;
int NUMBER_OF_UNSAVED_SEARCHES = 0;

int main()
{
    // Variables for socket and server
    int serverSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int status, addrSize, bytesReceived, returnFromPrime;
    fd_set readfds, writefds;
    char buffer[30];
    char *response;

    // Pokemon File to be opened
    FILE *file;
    // File to save to
    FILE *fileToSave;
    // User input to be taken
    char userInput[MAX_USER_INPUT];
    //Threads that will carry out operations
    pthread_t thread1, thread2;
    //Array of containers data max 18 (18 types of pokemons)
    dataContainer containers[MAX_NUMTYPE_POKEMON];

    //Ask user for file name
    printf("Please enter file name you would like to open: ");
    while (1)
    {
        scanf("%s", userInput);

        if (strcmp(userInput, "q") == 0)
        {
            printf("[Program quit successfuly]\n");
            exit(1);
        }

        file = fopen(userInput, "r");
        // Checking if file opened successfuly
        if (!file)
        {
            printf("\nPokemon file is not found. Please enter the name of the file again or enter q to quit: ");
            continue;
        }
        else
        {
            break;
        }
    }

    // Create the server socket
    serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (serverSocket < 0)
    {
        printf("*** SERVER ERROR: Could not open socket.\n");
        exit(-1);
    }

    // Setup the server address
    memset(&serverAddr, 0, sizeof(serverAddr)); // zeros the struct
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons((unsigned short)SERVER_PORT);

    // Bind the server socket
    status = bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (status < 0)
    {
        printf("*** SERVER ERROR: Could not bind socket.\n");
        exit(-1);
    }

    // Wait for clients now
    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);
        FD_ZERO(&writefds);
        FD_SET(serverSocket, &writefds);
        status = select(FD_SETSIZE, &readfds, &writefds, NULL, NULL);
        if (status == 0)
        {
            // Timeout occurred, no client ready
        }
        else if (status < 0)
        {
            printf("*** SERVER ERROR: Could not select socket.\n");
            exit(-1);
        }
        else
        {
            addrSize = sizeof(clientAddr);
            bytesReceived = recvfrom(serverSocket, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &addrSize);
            if (bytesReceived > 0)
            {
                buffer[bytesReceived] = '\0';
                printf("SERVER: Received client request: %s\n", buffer);
            }

            //Option A
            if (strcmp(buffer, "a") == 0)
            {
                response = "Please enter the type of pokemon you want to search for: ";
                printf("SERVER: Asking client for type of pokemon...\n");
                sendto(serverSocket, response, strlen(response), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
                bytesReceived = recvfrom(serverSocket, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &addrSize);
                if (bytesReceived > 0)
                {
                    buffer[bytesReceived] = '\0';
                    printf("SERVER: Received client request: %s\n", buffer);
                }

                //Preforming the search
                containers[NUMBER_OF_UNSAVED_SEARCHES].mainFile = file;
                strcpy(containers[NUMBER_OF_UNSAVED_SEARCHES].userType, buffer);
                pthread_create(&thread1, NULL, search_thread, &containers[NUMBER_OF_UNSAVED_SEARCHES]);
                pthread_join(thread1, NULL);
            }

            //Option B
            if (strcmp(buffer, "b") == 0)
            {
                while (1)
                {

                    response = "Please enter the name of the file you want to save to : ";
                    printf("SERVER: Asking client for file name...\n");
                    sendto(serverSocket, response, strlen(response), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
                    bytesReceived = recvfrom(serverSocket, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &addrSize);
                    if (bytesReceived > 0)
                    {
                        buffer[bytesReceived] = '\0';
                        printf("SERVER: Received client request: %s\n", buffer);
                    }

                    fileToSave = fopen(buffer, "a");
                    //Checking if file opened correctly
                    if (!fileToSave)
                    {
                        printf("Could not open fie please try again");
                        continue;
                    }

                    //Preforming save operation
                    containers[0].createFile = fileToSave;
                    pthread_create(&thread2, NULL, save_thread, &containers);
                    pthread_join(thread2, NULL);
                    NUMBER_OF_UNSAVED_SEARCHES = 0;
                    break;
                }
            }

            // Option C
            if (strcmp(buffer, "c") == 0)
            {
                // Sending number of quiries over to client
                printf("SERVER: Sending exit query to client\n");
                sprintf(buffer, "%d", NUMBER_OF_QUERIES);
                sendto(serverSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
                break;
            }

            response = "Query successfuly carried out";

            //Send back response to the client
            printf("SERVER: Sending success message to client...\n");
            sendto(serverSocket, response, strlen(response), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
        }
    }

    // Close socket and file
    close(serverSocket);
    fclose(file);
    printf("SERVER: Shutting down.\n");

    return EXIT_SUCCESS;
}

void *search_thread(void *arg)
{
    //Casting the void pointer
    dataContainer *data = (dataContainer *)arg;
    // To get the size of the array where we will store results
    int numberOfLines = 0;
    //Pointers to char arrays in order to store the lines
    char *line = (char *)malloc(sizeof(char) * MAX_LENGTH_OF_LINE);
    char *line2 = (char *)malloc(sizeof(char) * MAX_LENGTH_OF_LINE);
    char *token;

    // First itteration to get the number of results to create an array
    rewind(data->mainFile);
    while (fgets(line, MAX_LENGTH_OF_LINE, data->mainFile))
    {
        token = strtok(line, ",");

        for (int j = 0; j < 3; j++)
        {
            if (strcmp(token, data->userType) == 0)
            {
                numberOfLines++;
            }
            token = strtok(NULL, ",");
        }
    }

    // Check if no results and then exit the thread else create the array with the size
    if ((data->storedResults = (char **)malloc(sizeof(char *) * numberOfLines)) == NULL || numberOfLines == 0)
    {
        return NULL;
    }

    // To create the char arrays inside the main array
    for (int i = 0; i < numberOfLines; i++)
    {
        data->storedResults[i] = (char *)malloc(sizeof(char) * MAX_LENGTH_OF_LINE);
    }

    //Setting the total number of search results
    data->returnedLines = numberOfLines;

    // Counter for array
    int counter = 0;

    // Second iteration in order to save the results to the array
    rewind(data->mainFile);
    while (fgets(line, MAX_LENGTH_OF_LINE, data->mainFile))
    {
        //Copying the original line to keep a copy
        strcpy(line2, line);

        token = strtok(line, ",");

        for (int j = 0; j < 3; j++)
        {
            if (strcmp(token, data->userType) == 0)
            {
                strcpy(data->storedResults[counter], line2);
                counter++;
            }
            token = strtok(NULL, ",");
        }
    }

    //Updating global counters
    NUMBER_OF_QUERIES++;
    NUMBER_OF_UNSAVED_SEARCHES++;

    free(line);
    free(line2);
    line2 = NULL;
    line = NULL;

    return NULL;
}

void *save_thread(void *arg)
{
    //Casting void pointer
    dataContainer *data = (dataContainer *)arg;
    //Storing the file pointer in a variable
    FILE *file = data[0].createFile;
    //Printing the header
    fprintf(file, "#,Name,Type 1,Type 2,Total,HP,Attack,Defense,Sp. Atk,Sp. Def,Speed,Generation,Legendary\n");

    if (NUMBER_OF_UNSAVED_SEARCHES == 0)
    {
        fclose(file);
        return NULL;
    }

    //Go through the array of structures
    for (int i = 0; i < NUMBER_OF_UNSAVED_SEARCHES; i++)
    {
        for (int j = 0; j < data[i].returnedLines; j++)
        {
            fprintf(file, "%s", data[i].storedResults[j]);
            free(data[i].storedResults[j]);
            data[i].storedResults[j] = NULL;
        }
        free(data[i].storedResults);
        data[i].storedResults = NULL;
    }
    fprintf(file, "\n");
    fclose(file);

    NUMBER_OF_QUERIES++;
    return NULL;
}
