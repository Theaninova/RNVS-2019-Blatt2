/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define PORT "3490"  // the port users will be connecting to
#define MAXSIZE 1000
#define BACKLOG 10     // how many pending connections queue will hold

#define FILE_MODE_READONLY "r"

void sigchld_handler(int s) {
    (void) s; // quiet unused variable warning

    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}


size_t getQuote(const char *file_name, char** result) {

    FILE *file_pointer;
    file_pointer = fopen(file_name, FILE_MODE_READONLY);
    if (file_pointer == NULL) {
        printf("Could not open file %s", file_name);
        exit(1);
    }

    fseek(file_pointer, 0, SEEK_END);
    size_t file_size = (size_t) ftell(file_pointer);
    fseek(file_pointer, 0, SEEK_SET);

    int quotes_count = 0;
    for (char current_char = (char) getc(file_pointer); current_char != EOF; current_char = (char) getc(file_pointer)) {
        if (current_char == '\n') {
            quotes_count++;
        }
    }
    fclose(file_pointer);
    file_pointer = fopen(file_name, FILE_MODE_READONLY);

    srand(time(0));
    int random_line_index = rand() % quotes_count;
    size_t line_length = 0;
    for (int i = 0; i < random_line_index
        && getline(result, &line_length, file_pointer) != -1; i++) { /*noop*/ }

    fclose(file_pointer);

    return line_length;
}


int main(void) {
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while (1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *) &their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);


        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            char* quote = NULL;
            size_t send_length = getQuote("quotes.txt", &quote);
            if (send(new_fd, quote, send_length, 0) == -1)
                perror("send");
            close(new_fd);
            exit(0);
        }

        close(new_fd);  // parent doesn't need this
    }

    return 0;
}

