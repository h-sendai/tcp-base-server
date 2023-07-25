#include <sys/time.h>

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "my_signal.h"
#include "my_socket.h"
#include "logUtil.h"
#include "get_num.h"

int debug = 0;

int child_proc(int connfd, int bufsize)
{
    fprintfwt(stderr, "server: start\n");

    unsigned char *buf = malloc(bufsize);
    if (buf == NULL) {
        err(1, "malloc for buf");
    }

    for ( ; ; ) {
        int n = write(connfd, buf, bufsize);
        if (n < 0) {
            if (errno == ECONNRESET) {
                fprintfwt(stderr, "server: connection reset by client (ECONNRESET)\n");
                break; /* exit from for ( ; ; ) loop */
            }
            if (errno == EPIPE) {
                fprintfwt(stderr, "server: connection reset by client (EPIPE)\n");
                break; /* exit from for ( ; ; ) loop */
            }
            else {
                err(1, "write");
            }
        }
        sleep(1);
    }

    return 0;
}

void sig_chld(int signo)
{
    pid_t pid;
    int   stat;

    while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        ;
    }
    return;
}

int usage(void)
{
    char *msg = "Usage: server [-p port (1234)] [-b bufsize (8k)]\n"
"-p port: port number (default 1234)\n"
"-b:      bufsize (default 8k)\n";

    fprintf(stderr, "%s", msg);

    return 0;
}

int main(int argc, char *argv[])
{
    int port = 1234;
    pid_t pid;
    struct sockaddr_in remote;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int listenfd;

    long bufsize = 8*1024;

    int c;
    while ( (c = getopt(argc, argv, "b:dhp:")) != -1) {
        switch (c) {
            case 'b':
                bufsize = get_num(optarg);
                break;
            case 'd':
                debug += 1;
                break;
            case 'h':
                usage();
                exit(0);
            case 'p':
                port = strtol(optarg, NULL, 0);
                break;
            default:
                break;
        }
    }
    argc -= optind;
    argv += optind;

    my_signal(SIGCHLD, sig_chld);
    my_signal(SIGPIPE, SIG_IGN);

    listenfd = tcp_listen(port);
    if (listenfd < 0) {
        errx(1, "tcp_listen");
    }

    for ( ; ; ) {
        int connfd = accept(listenfd, (struct sockaddr *)&remote, &addr_len);
        if (connfd < 0) {
            err(1, "accept");
        }
        
        pid = fork();
        if (pid == 0) { // child process
            if (close(listenfd) < 0) {
                err(1, "close listenfd");
            }
            child_proc(connfd, bufsize);
            exit(0);
        }
        else { // parent process
            if (close(connfd) < 0) {
                err(1, "close for accept socket of parent");
            }
        }
    }
        
    return 0;
}
