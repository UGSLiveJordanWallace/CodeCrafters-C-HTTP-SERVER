#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

char* ok_response = "HTTP/1.1 200 OK\r\n\r\n";
char* not_found_404 = "HTTP/1.1 404 Not Found\r\n\r\n";

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

	int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
	printf("Client connected\n");

    char req[1024]; 
    read(client_fd, req, sizeof(req));
    char* method = strtok(req, " ");
    char* path = strtok(NULL, " ");
    char* version = strtok(NULL, "\r\n");
    strtok(NULL, " ");
    char* host = strtok(NULL, "\r\n");
    strtok(NULL, " ");
    char* accept = strtok(NULL, "\r\n");
    strtok(NULL, " ");
    char* user_agent = strtok(NULL, "\r\n");
    printf("Method: %s\nPath: %s\nVersion: %s\nHost: %s\nAccept: %s\nAgent: %s\n", method, path, version, host, accept, user_agent);

    if (strcmp(method, "GET") == 0) {
        char custom_response[1024]; 

        if (strcmp(path, "/") == 0) {
            send(client_fd, ok_response, strlen(ok_response), 0);
            close(client_fd);
            close(server_fd);
            printf("Method: %s\nPath: %s\n", method, path);
            return 0;
        }

        if (strcmp(path, "/user-agent") == 0) {
            sprintf(custom_response, 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: %lu\r\n\r\n%s", strlen(user_agent), user_agent);
            send(client_fd, custom_response, strlen(custom_response), 0);
            close(client_fd);
            close(server_fd);
            printf("Method: %s\nPath: %s\nVersion: %s\nHost: %s\nAccept: %s\nAgent: %s\n", method, path, version, host, accept, user_agent);
            return 0;
        }

        char* base = strtok(path, "/");
        char* slug = strtok(NULL, "/");

        if (strcmp(base, "echo") == 0) {
            sprintf(custom_response, 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: %lu\r\n\r\n%s", strlen(slug), slug);
            send(client_fd, custom_response, strlen(custom_response), 0);
            close(client_fd);
            close(server_fd);
            printf("Method: %s\nPath: %s\n", method, path);
            return 0;
        }
        
        send(client_fd, not_found_404, strlen(not_found_404), 0);
        close(client_fd);
        close(server_fd);
    }

	return 0;
}
