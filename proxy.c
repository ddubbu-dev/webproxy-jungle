#include "csapp.h"
#define DEV_TINY_PORT "7000"

void doit(int fd);
int main(int argc, char **argv) {
    if (argc != 2) {
        printf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int listenfd = Open_listenfd(argv[1]);
    int connfd;
    socklen_t clientaddr_size;
    struct sockaddr_storage clientaddr;
    char hostname[MAXLINE], port[MAXLINE];
    while (1) { // 서버 계속 실행
        clientaddr_size = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientaddr_size); // clinet <-> proxy
        Getnameinfo((SA *)&clientaddr, clientaddr_size, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd);
        Close(connfd);
    }
}

/**
 * client > proxy > tiny
 */
void doit(int client_fd) {
    // char *tiny_host = "127.0.0.1", *tiny_port = getenv("TINY_PORT"); // prd
    char *tiny_host = "127.0.0.1", *tiny_port = DEV_TINY_PORT; // dev

    rio_t client_rio;
    Rio_readinitb(&client_rio, client_fd);
    int readn;
    char req_client_to_proxy[MAXBUF] = "";
    int proxy_fd = Open_clientfd(tiny_host, tiny_port); // proxy <-> tiny

    printf("\n\n================ [PROXY][REQUEST][TO_TINY] ================\n\n");
    // HTTP 시작라인에서 (e.g tiny server) 타겟 호스트 분리하기
    char method[MAXBUF], url[MAXBUF], version[MAXBUF], uri[MAXBUF];
    Rio_readlineb(&client_rio, req_client_to_proxy, MAXBUF); // (req) HTTP 시작라인 읽기
    sscanf(req_client_to_proxy, "%s %s%s %s", method, url, version);
    sscanf(url, "http://%*[^/]%s", uri);

    char http_start_line[MAXBUF];
    sprintf(http_start_line, "%s %s %s\r\n", method, uri, version);
    Rio_writen(proxy_fd, http_start_line, strlen(http_start_line));
    printf("req: %s", http_start_line);

    while (readn = Rio_readlineb(&client_rio, req_client_to_proxy, MAXBUF) != 0) { // client > (request) > proxy > tiny
        Rio_writen(proxy_fd, req_client_to_proxy, strlen(req_client_to_proxy));    // client > proxy > (request) > tiny
        printf("req: %s", req_client_to_proxy);

        if (strcmp(req_client_to_proxy, "\r\n") == 0) { // TODO: 요청 끝 맞는지 확인 필요
            break;
        }
    }
    printf("\n\n================ [PROXY][RESPONSE][FROM_TINY] ================\n\n");
    rio_t proxy_rio;
    Rio_readinitb(&proxy_rio, proxy_fd);
    char res_proxy_from_tiny[MAXBUF] = "";

    while (readn = Rio_readlineb(&proxy_rio, res_proxy_from_tiny, MAXBUF) != 0) { // clinet < proxy < (response) < tiny
        Rio_writen(client_fd, res_proxy_from_tiny, MAXBUF);                       // client < (response) < proxy < tiny
        printf("res: %s", res_proxy_from_tiny);
    }
    Close(proxy_fd);
    printf("\n\n================ [PROXY][END] ================\n\n");
    printf("Close Connection..");
}
