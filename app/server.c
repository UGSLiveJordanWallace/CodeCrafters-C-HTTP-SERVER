#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <pthread.h>

char* ok_response = "HTTP/1.1 200 OK\r\n\r\n";
char* not_found_404 = "HTTP/1.1 404 Not Found\r\n\r\n";

void handle_client_connection(int client_fd);

typedef struct {
    char* method;
    char* path;
    char* version;
    char* host;
    char* accept;
    char* user_agent;
} HTTP_Header;

int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	// You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");

	// Uncomment this block to pass the first stage

	int server_fd;
    socklen_t client_addr_len;
	struct sockaddr_in client_addr;


	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}

	// Since the tester restarts your program quite often, setting SO_REUSEADDR
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}

	struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
		.sin_port = htons(4221),
		.sin_addr = { htonl(INADDR_ANY) },
	};

	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}

	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}

	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);
    printf("Listening\n");

    fd_set current_sockets, ready_sockets;
    FD_ZERO(&current_sockets);
    FD_SET(server_fd, &current_sockets);

    for (;;) {
        ready_sockets = current_sockets;

        if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0) {
            printf("Failed To Load Clients");
            break;
        }

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &ready_sockets)) {
                if (i == server_fd) {
                    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
                    FD_SET(client_fd, &current_sockets);
                } else {
                    handle_client_connection(i);
                    FD_CLR(i, &current_sockets);
                }
            }
        }

    }

    close(server_fd);
	return 0;
}

HTTP_Header parse_header(char* req) {
    printf("\nHeader Parser Logs\n");
    printf("------------------\n");
    HTTP_Header header;

    header.method = strtok(req, " ");
    header.path = strtok(NULL, " ");
    header.version = strtok(NULL, "\r\n");
    printf("Method: %s Path: %s Version: %s\n", header.method, header.path, header.version);

    char* token = strtok(NULL, " ");
    while (token != NULL) {
        if (strncmp(token, "\nHost", 5) == 0) {
            header.host = strtok(NULL, "\r\n");
            printf("Header-Host: %s\n", header.host);
        }
        if (strncmp(token, "\nAccept", 7) == 0) {
            header.accept = strtok(NULL, "\r\n");
            printf("Header-Accept: %s\n", header.accept);
        }
        if (strncmp(token, "\nUser-Agent", 11) == 0) {
            header.user_agent = strtok(NULL, "\r\n");
            printf("Header-UserAgent: %s\n", header.user_agent);
        }

        token = strtok(NULL, " ");
    }
    printf("------------------\n");

    return header;
}

void handle_client_connection(int client_fd) {
    char req[1024]; 
    read(client_fd, req, sizeof(req));
    HTTP_Header header = parse_header(req);

    if (strcmp(header.method, "GET") == 0) {
        char custom_response[1024]; 

        if (strcmp(header.path, "/") == 0) {
            send(client_fd, ok_response, strlen(ok_response), 0);
            printf("Client Connection:\n Method: %s Path: %s\n", header.method, header.path);
        } else if (strcmp(header.path, "/user-agent") == 0) {
            sprintf(custom_response, 
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: %lu\r\n\r\n%s", strlen(header.user_agent), header.user_agent);
            send(client_fd, custom_response, strlen(custom_response), 0);
            printf("Client Connection:\n Method: %s Path: %s Version: %s Host: %s Agent: %s\n", header.method, header.path, header.version, header.host, header.user_agent);
        } else if (strncmp(header.path, "/echo", 5) == 0) {
            strtok(header.path, "/");
            char* slug = strtok(NULL, "/");
            sprintf(custom_response, 
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: %lu\r\n\r\n%s", strlen(slug), slug);
            send(client_fd, custom_response, strlen(custom_response), 0);
            printf("Client Connection:\n Method: %s\nPath: %s\n", header.method, header.path);
        } else {
            send(client_fd, not_found_404, strlen(not_found_404), 0);
        }
    }

    close(client_fd);
}
