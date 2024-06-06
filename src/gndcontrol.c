// This is the main program for the air traffic ground control server.

// The job of this module is to set the system up and then turn over control
// to the airs_protocol module which will handle the actual communication
// protocol between clients (airplanes) and the server.

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "airplane.h"
#include "airplanelist_control.h"
#include "airs_protocol.h"
#include "queue_manager.h"
#include "util.h"

#define SERV_HOST "localhost"
#define SERV_PORT "8080"
#define EXIT_STR "EXIT"

int create_listener(char *service);
void *handle_conn(void *args);

/************************************************************************
 * Part 1 main: Only 1 airplane, doing I/O via stdin and stdout.
 */

int main(int argc, char *args[]) {
    pthread_t thread;
    int sock_fd = create_listener(SERV_PORT);

    if (sock_fd < 0) {
        fprintf(stderr, "Server setup failed.\n");
        exit(1);
    }

    create_planelist();
    
    if(pthread_create(&thread, NULL, queue_han_thread, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }
    pthread_detach(thread);

    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int comm_fd;

    char *lineptr = NULL;
    size_t linesize = 0;

    struct pollfd poll_fds[2];
    poll_fds[0].fd = sock_fd;
    poll_fds[0].events = POLLIN;

    poll_fds[1].fd = 0;
    poll_fds[1].events = POLLIN;

    while (1) {
        poll(poll_fds, 2, 5);

        if (poll_fds[0].revents & POLLIN) {
            if ((comm_fd = accept(sock_fd, (struct sockaddr *)&client_addr,
                                  &client_addr_len)) >= 0) {
                if (pthread_create(&thread, NULL, handle_conn, &comm_fd) != 0) {
                    perror("pthread_create");
                    exit(1);
                }
                printf("Got connection from %s (Client: %ld)\n",
                       inet_ntoa(((struct sockaddr_in *)&client_addr)->sin_addr),
                       thread);

                pthread_detach(thread);
            }
        }

        if (poll_fds[1].revents & POLLIN) {
            getline(&lineptr, &linesize, stdin);
            lineptr = trim(lineptr);
            if (strcmp(lineptr, EXIT_STR) == 0) {
                free(lineptr);
                break;
            }
        }
    }

    destroy_planelist();
    close(sock_fd);

    return 0;
}

int create_listener(char *service) {
    int sock_fd;
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    int optval = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    struct addrinfo *result;
    int rval;
    if ((rval = getaddrinfo(NULL, service, &hints, &result))) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(rval));
        close(sock_fd);
        return -1;
    }

    int bret = bind(sock_fd, result->ai_addr, result->ai_addrlen);
    freeaddrinfo(result);
    result = NULL;

    if (bret < 0) {
        perror("bind");
        close(sock_fd);
        return -1;
    }

    int lret = listen(sock_fd, 128);
    if (lret < 0) {
        perror("listen");
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

void *handle_conn(void *args) {
    int in_fd = *(int *)args;
    int out_fd = -1;
    FILE *fp_send = NULL;
    FILE *fp_recv = NULL;
    char *lineptr = NULL;
    size_t linesize = 0;
    airplane *plane = malloc(sizeof(airplane));

    if ((out_fd = dup(in_fd)) < 0) {
        perror("dup");
        exit(1);
    }

    if ((fp_send = fdopen(out_fd, "a")) == NULL) {
        perror("fdopen");
        exit(1);
    }

    if ((fp_recv = fdopen(in_fd, "r")) == NULL) {
        perror("fdopen");
        exit(1);
    }

    setvbuf(fp_send, NULL, _IOLBF, 0);
    setvbuf(fp_recv, NULL, _IOLBF, 0);

    airplane_init(plane, fp_send, fp_recv);

    while (plane->state != PLANE_DONE) {
        if (getline(&lineptr, &linesize, fp_recv) < 0) {
            break;
        }
        docommand(plane, lineptr);
    }

    close(in_fd);
    close(out_fd);
    planelist_remove(plane);
    free(plane);
    free(lineptr);
    plane = NULL;
    lineptr = NULL;
    fclose(fp_send);
    fclose(fp_recv);

    printf("Client %ld disconnected\n", pthread_self());

    return 0;
}
