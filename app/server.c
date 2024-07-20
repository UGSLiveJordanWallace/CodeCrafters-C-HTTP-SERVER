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

char* ok_response_200 = "HTTP/1.1 200 OK\r\n\r\n";
char* not_found_404 = "HTTP/1.1 404 Not Found\r\n\r\n";
char* file_created_201 = "HTTP/1.1 201 Created\r\n\r\n";

void handle_client_connection(int client_fd, int argc, char* argv[]);

typedef struct {
    char* method;
    char* path;
    char* version;

    char* host;
    char* accept;
    char* user_agent;
    char* content_type;
    int content_length;
    char* body;
} HTTP_Header;

int main(int argc, char* argv[]) {
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

	int connection_backlog = 10;
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
                    handle_client_connection(i, argc, argv);
                    FD_CLR(i, &current_sockets);
                }
            }
        }

    }

    close(server_fd);
	return 0;
}

const char* get_file_contents(char* filename, const char* dir) {

    char* full_path = (char*)calloc(strlen(dir) + strlen(filename), sizeof(char));
    strncpy(full_path, dir, strlen(dir));
    strncat(full_path, filename, strlen(filename));

    printf("Getting File %s From Path: %s\nFull Path: %s\n", filename, dir, full_path);

    FILE* fs = fopen(full_path, "r");
    if (fs == NULL) {
        return NULL;
    }

    fseek(fs, 0, SEEK_END);
    uint16_t size = ftell(fs);
    fseek(fs, 0, SEEK_SET); 
    char* out = (char*)calloc(size, sizeof(char));
    fread(out, size, 1, fs);

    free(full_path);
    fclose(fs);
    return out;
}

int create_file(const char* dir, const char* filename, char* file_contents) {
    char* full_path = (char*)calloc(strlen(dir) + strlen(filename), sizeof(char));
    strncpy(full_path, dir, strlen(dir));
    strncat(full_path, filename, strlen(filename));

    printf("Creating File %s From Path: %s\nFull Path: %s\n", filename, dir, full_path);

    FILE* file = fopen(full_path, "w");
    if (file == NULL) {
        return -1;
    }

    fprintf(file, "%s", file_contents);
    fclose(file);
    return 0;
}

void parse_header(HTTP_Header* header, char req[1024]) {
    printf("\n-----------------\n");
    printf("Header Parser Logs\n");
    printf("------------------\n");
    char copy[1024];
    strncpy(copy, req, 1024);

    header->method = strtok(req, " ");
    header->path = strtok(NULL, " ");
    header->version = strtok(NULL, "\r\n");
    printf("Method: %s Path: %s Version: %s\n", header->method, header->path, header->version);

    char* token = strtok(NULL, " ");
    while (token != NULL) {
        if (strncmp(token, "\nHost", 5) == 0) {
            header->host = strtok(NULL, "\r\n");
            printf("Header-Host: %s\n", header->host);
        }
        if (strncmp(token, "\nAccept", 7) == 0) {
            header->accept = strtok(NULL, "\r\n");
            printf("Header-Accept: %s\n", header->accept);
        }
        if (strncmp(token, "\nUser-Agent", 11) == 0) {
            header->user_agent = strtok(NULL, "\r\n");
            printf("Header-UserAgent: %s\n", header->user_agent);
        }
        if (strncmp(token, "\nContent-Type", 13) == 0) {
            header->content_type = strtok(NULL, "\r\n");            
            printf("Header-ContentType: %s\n", header->content_type);
        }
        if (strncmp(token, "\nContent-Length", 15) == 0) {
            header->content_length = atoi(strtok(NULL, "\r\n"));            
            printf("Header-ContentLength: %d\n", header->content_length);
        }

        token = strtok(NULL, " ");
    }

    if (header->content_type) {
        char* body = strstr(copy, "\r\n\r\n");
        header->body = (char*)(calloc)(strlen(body), sizeof(char));
        memmove(header->body, strtok(body, "\r\n\r\n"), strlen(body));
        printf("Found Content: %s\n", header->body);
    }

    printf("------------------\n");
}

void handle_client_connection(int client_fd, int argc, char* argv[]) {
    char req[1024] = ""; 
    char response[1024] = ""; 
    read(client_fd, req, sizeof(req));
    printf("\nFULL Request: %s\n", req);
    HTTP_Header* header = (HTTP_Header*)calloc(1, sizeof(HTTP_Header));
    parse_header(header, req);

    if (strcmp(header->method, "GET") == 0) {
        if (strcmp(header->path, "/") == 0) {
            send(client_fd, ok_response_200, strlen(ok_response_200), 0);
            printf("Client Connection:\n Method: %s Path: %s\n", header->method, header->path);
        } else if (strcmp(header->path, "/user-agent") == 0) {
            sprintf(response, 
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: %lu\r\n\r\n%s", strlen(header->user_agent), header->user_agent);
            send(client_fd, response, strlen(response), 0);
            printf("Client Connection:\n Method: %s Path: %s Version: %s Host: %s Agent: %s\n", header->method, header->path, header->version, header->host, header->user_agent);
        } else if (strncmp(header->path, "/echo", 5) == 0) {
            strtok(header->path, "/");
            char* slug = strtok(NULL, "/");
            sprintf(response, 
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: %lu\r\n\r\n%s", strlen(slug), slug);
            send(client_fd, response, strlen(response), 0);
            printf("Client Connection:\n Method: %s\nPath: %s\n", header->method, header->path);
        } else if (strncmp(header->path, "/files", 6) == 0 && argc > 1 && strcmp(argv[1], "--directory") == 0) {
            strtok(header->path, "/");
            char* filename = strtok(NULL, "/");
            const char* file_contents = get_file_contents(filename, argv[2]);
            if (file_contents == NULL) {
                send(client_fd, not_found_404, strlen(not_found_404), 0);
            } else {
                sprintf(response, 
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/octet-stream\r\n"
                        "Content-Length: %lu\r\n\r\n%s", strlen(file_contents), file_contents);
                send(client_fd, response, strlen(response), 0);
                printf("Client Connection:\n Method: %s\nPath: %s\n", header->method, header->path);
            }
        } else {
            send(client_fd, not_found_404, strlen(not_found_404), 0);
        }
    } else if (strcmp(header->method, "POST") == 0 && argc > 1 && strcmp(argv[1], "--directory") == 0) {
        if (strncmp(header->path, "/files", 6) == 0) {
            strtok(header->path, "/");
            char* filename = strtok(NULL, "/");
            if (create_file(argv[2], filename, header->body) == 0) {
                memset(response, '\0', 1);
                strncpy(response, file_created_201, strlen(file_created_201));
                send(client_fd, response, strlen(response), 0);
                printf("Client Connection:\n Method: %s\nPath: %s\n", header->method, header->path);
            } else {
                send(client_fd, not_found_404, strlen(not_found_404), 0);
            }
        }
    }

    close(client_fd);
    free(header);
}
