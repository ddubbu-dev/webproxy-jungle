#include "csapp.h"

/**
 * main함수가 CLI 인자를 받을 때 사용하는 매개변수
 * - argc (argument count) : 항상 1 이상, 실행파일 이름 자체가 첫번째 인자로 포함되기 때문
 * - argv (argument vector) : 인자들의 배열
 */
int main(int argc, char **argv) {
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }
    host = argv[1];
    port = argv[2];

    clientfd = Open_clientfd(host, port); // returns a socket file descriptor

    Rio_readinitb(&rio, clientfd);

    while (Fgets(buf, MAXLINE, stdin) != NULL) {
        Rio_writen(clientfd, buf, strlen(buf));
        Rio_readlineb(&rio, buf, MAXLINE);
        Fputs(buf, stdout);
    }
    Close(clientfd);
    exit(0);
}