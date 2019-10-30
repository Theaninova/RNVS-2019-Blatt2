#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAXDATASIZE 3 // max Data_length


// get the IPv4 or IPv6 address
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr); //IPv4
    }
    return &(((struct sockaddr_in6 *) sa)->sin6_addr); //IPv6
}

int main(int argc, char *argv[]) {
    int socket_fd;
    char buffer[MAXDATASIZE];
    struct addrinfo hints, *server_info, *pointer;
    int rv;
    char s[INET6_ADDRSTRLEN];

    //error testing
    if (argc != 3) {
        perror("Error: please enter -> client hostname port\n");
        exit(1);
    }

    //define hints
    memset(&hints, 0, sizeof hints); //empty hint
    hints.ai_family = AF_UNSPEC; //IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; //TCP Protocol

    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &server_info)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    for (pointer = server_info; pointer != NULL; pointer = pointer->ai_next) {
        if ((socket_fd = socket(pointer->ai_family, pointer->ai_socktype,
                                pointer->ai_protocol)) == -1) {
            perror("Error: creating client socket");
            continue;
        }
        if (connect(socket_fd, pointer->ai_addr, pointer->ai_addrlen) == -1) {
            perror("Error: no connection for client");
            close(socket_fd);
            continue;
        }
        break;
    }
    if (pointer == NULL) {
        fprintf(stderr, "Error: client failed to connect\n");
        return 2;
    }

    inet_ntop(pointer->ai_family, get_in_addr((struct sockaddr *) pointer->ai_addr),
              s, sizeof s);
    printf("connecting to %s\n", s);

    freeaddrinfo(server_info);

    int total_number_of_bytes = 0;
    int number_of_bytes;
    char quote[1000];

    while ((number_of_bytes = recv(socket_fd, buffer, MAXDATASIZE - 1, 0)) > 0) {
        memcpy(quote + total_number_of_bytes, buffer, number_of_bytes);
        total_number_of_bytes += number_of_bytes;
        /*
        else if ((number_of_bytes = recv(socket_fd, buffer, MAXDATASIZE - 1, 0)) == -1) {
            perror("Error: fail to receive");
            exit(1);
        } */
    }
    quote[total_number_of_bytes] = '\0';

    printf("client: received '%s'\n", quote);

    close(socket_fd);

    return 0;
}

