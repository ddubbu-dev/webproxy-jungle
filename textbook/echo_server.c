#include "csapp.h"

void echo(int connfd);

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    // Open_listenfd
    // [1] getaddrinfo : host, port 정보 기반으로
    // [2] socket      : socket fd 생성 (listenfd)
    // [3] bind        : bind the descriptor to the address
    // [4] listen      : 능동소켓에서 듣기(수동) 소켓으로 전환
    int listenfd = Open_listenfd(argv[1]);

    int connfd;
    socklen_t clientaddr_size;
    struct sockaddr_storage clientaddr;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    while (1) { // 서버 계속 실행
        clientaddr_size = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientaddr_size);                                     // [5] Connection Request
        Getnameinfo((SA *)&clientaddr, clientaddr_size, client_hostname, MAXLINE, client_port, MAXLINE, 0); // 소켓 주소 구조체를 대응되는 호스트, 서비스 스트링으로 변환
        printf("Connected to client (%s %s)\n", client_hostname, client_port);
        echo(connfd); // get request and send response
        Close(connfd);
    }
    exit(0);
}

void echo(int connfd) {
    size_t n;
    rio_t rio;
    Rio_readinitb(&rio, connfd); // rio 구조체를 connfd와 연결된 소켓으로 초기화

    char clientRequest[MAXLINE];
    while ((n = Rio_readlineb(&rio, clientRequest, MAXLINE)) != 0) { // [6] read request
        printf("server received %d bytes\n", (int)n);                //
        Rio_writen(connfd, clientRequest, n);                        // [7] send response (echo)
    }
}
