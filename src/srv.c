#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 3001
#define BUFFER_SIZE 8192
#define MAX_KEY_LENGTH 100
#define MAX_FILE_CONTENT_LENGTH 8192

// Key-Value Pair Structure
typedef struct {
    char key[MAX_KEY_LENGTH];
    char base64Content[MAX_FILE_CONTENT_LENGTH];
} KeyValuePair;

// Function to store a key and Base64-encoded content in the database
void put(const char *key, const char *base64Content) {
    if (strlen(key) >= MAX_KEY_LENGTH) {
        printf("Error: Key length exceeds the maximum allowed.\n");
        return;
    }

    if (strlen(base64Content) >= MAX_FILE_CONTENT_LENGTH) {
        printf("Error: Base64 content exceeds the maximum allowed size.\n");
        return;
    }

    const char *home = getenv("HOME");
    if (!home) {
        printf("Error: HOME environment variable not set.\n");
        return;
    }

    char dbPath[256];
    snprintf(dbPath, sizeof(dbPath), "%s/.eros/eros.db", home);

    FILE *db = fopen(dbPath, "a");
    if (!db) {
        perror("Failed to open eros.db file");
        return;
    }

    KeyValuePair kvp;
    strncpy(kvp.key, key, MAX_KEY_LENGTH);
    strncpy(kvp.base64Content, base64Content, MAX_FILE_CONTENT_LENGTH);

    fwrite(&kvp, sizeof(KeyValuePair), 1, db);
    fclose(db);

    printf("PUT: Stored key-value: %s -> Base64 Content (length %ld)\n", key, strlen(base64Content));
}

// Function to get the Base64 content by key
void get(const char *key, int clientSocket) {
    const char *home = getenv("HOME");
    if (!home) {
        printf("Error: HOME environment variable not set.\n");
        return;
    }

    char dbPath[256];
    snprintf(dbPath, sizeof(dbPath), "%s/.eros/eros.db", home);

    FILE *db = fopen(dbPath, "r");
    if (!db) {
        perror("Failed to open eros.db file");
        return;
    }

    KeyValuePair kvp;
    int found = 0;

    while (fread(&kvp, sizeof(KeyValuePair), 1, db)) {
        if (strncmp(kvp.key, key, MAX_KEY_LENGTH) == 0) {
            send(clientSocket, kvp.base64Content, strlen(kvp.base64Content), 0);
            found = 1;
            break;
        }
    }

    if (!found) {
        char *notFoundMsg = "GET: Key not found\n";
        send(clientSocket, notFoundMsg, strlen(notFoundMsg), 0);
    }

    fclose(db);
}

// Function to delete a key-value pair
void delete(const char *key) {
    const char *home = getenv("HOME");
    if (!home) {
        printf("Error: HOME environment variable not set.\n");
        return;
    }

    char dbPath[256];
    char tempDbPath[256];
    snprintf(dbPath, sizeof(dbPath), "%s/.eros/eros.db", home);
    snprintf(tempDbPath, sizeof(tempDbPath), "%s/.eros/temp.db", home);

    FILE *db = fopen(dbPath, "r");
    FILE *temp = fopen(tempDbPath, "w");
    if (!db || !temp) {
        perror("Error opening files");
        return;
    }

    KeyValuePair kvp;
    int found = 0;

    while (fread(&kvp, sizeof(KeyValuePair), 1, db)) {
        if (strncmp(kvp.key, key, MAX_KEY_LENGTH) == 0) {
            found = 1;
            printf("DELETE: Key %s and associated Base64 content deleted\n", kvp.key);
        } else {
            fwrite(&kvp, sizeof(KeyValuePair), 1, temp);  // Write other records to temp file
        }
    }

    fclose(db);
    fclose(temp);

    remove(dbPath);
    rename(tempDbPath, dbPath);

    if (!found) {
        printf("DELETE: Key not found: %s\n", key);
    }
}

// Command handler for each client
void *handle_client(void *arg) {
    int clientSocket = *(int *)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = read(clientSocket, buffer, sizeof(buffer));
        if (bytes_read <= 0) {
            break;  // Client closed connection
        }

        // Variables for command, key, and content
        char command[10], key[100];
        char *base64Content;

        // Parse the command and key first
        sscanf(buffer, "%s %s", command, key);

          // Find the position of the base64Content, which starts after the second space
          base64Content = strchr(buffer, ' ') + 1;  // Skip command
          base64Content = strchr(base64Content, ' ') + 1;  // Skip key

          // Now base64Content points to the rest of the string, which is the Base64 content
          if (strcmp(command, "PUT") == 0) {
              printf("PUT Command: Key: %s, Base64 Content (truncated): %.20s...\n", key, base64Content);
              put(key, base64Content);  // Store the Base64-encoded content
          } else if (strcmp(command, "GET") == 0) {
              printf("GET Command: Key: %s\n", key);
              get(key, clientSocket);  // Return Base64 content to the client
          } else if (strcmp(command, "DELETE") == 0) {
              printf("DELETE Command: Key: %s\n", key);
              delete(key);  // Delete the key-value pair
          } else {
              printf("Unknown command: %s\n", buffer);
          }

        char *response = "Command processed.\n";
        send(clientSocket, response, strlen(response), 0);
    }

    close(clientSocket);
    return NULL;
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    // Create socket
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    // Bind socket
    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(serverSocket, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is running on port %d...\n", PORT);

    while (1) {
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &addrLen);
        if (clientSocket < 0) {
            perror("Accept failed");
            continue;
        }

        // Handle each client in a separate pthread
        pthread_t clientThread;
        if (pthread_create(&clientThread, NULL, handle_client, &clientSocket) != 0) {
            perror("Failed to create thread");
        }
        pthread_detach(clientThread);  // Detach thread to free resources after it's done
    }

    close(serverSocket);
    return 0;
}
