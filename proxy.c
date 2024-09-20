#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
                                    "Firefox/10.0.3\r\n";

#define HTTP_VERSION "HTTP/1.0"

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
    // HTTP 시작라인에서 (e.g tiny server) 타겟 호스트 분리하기
    char req_client_to_proxy[MAXBUF];
    char method[MAXBUF], url[MAXBUF], version[MAXBUF];
    char tiny_protocal[MAXBUF] = "http", tiny_hostname[MAXBUF], uri[MAXBUF] = "";
    int tiny_port;
    char tiny_port_str[MAXBUF];

    rio_t client_rio;
    Rio_readinitb(&client_rio, client_fd);
    Rio_readlineb(&client_rio, req_client_to_proxy, MAXBUF); // (req) HTTP 시작라인 읽기

    // sscanf로 호스트, 포트, URI 분리
    sscanf(req_client_to_proxy, "%s %s %s\r\n", method, url, version);
    int splitted_cnt = sscanf(url, "http://%99[^:]:%d%99[^\n]", tiny_hostname, &tiny_port, uri);

    if (splitted_cnt < 3) { // URI가 없으면 빈 문자열 처리
        uri[0] = '/';
    }

    sprintf(tiny_port_str, "%d", tiny_port);

    printf("Hostname: %s\n", tiny_hostname);
    printf("Port: %d\n", tiny_port);
    printf("URI: %s\n", uri);

    printf("\n\n================ [PROXY][REQUEST][TO_TINY] ================\n\n");
    int proxy_fd = Open_clientfd(tiny_hostname, tiny_port_str); // proxy <-> tiny
    char http_start_line[MAXBUF];
    sprintf(http_start_line, "%s %s %s\r\n", method, uri, HTTP_VERSION);
    Rio_writen(proxy_fd, http_start_line, strlen(http_start_line));
    printf("req: %s", http_start_line);

    // Rio_writen(proxy_fd, "Host: localhost\r\n", strlen("Host: localhost\r\n"));
    // Rio_writen(proxy_fd, "Accept: */*\r\n", strlen("Accept: */*\r\n"));

    int readn;
    while (readn = Rio_readlineb(&client_rio, req_client_to_proxy, MAXBUF) != 0) {                 // client > (request) > proxy > tiny
        if (strncmp(req_client_to_proxy, "Proxy-Connection:", strlen("Proxy-Connection:")) == 0) { // 헤더 수정
            strcpy(req_client_to_proxy, "Connection: close\r\n");                                  // TODO: value는 뒤에 것 파싱해서 넣어줄 필요 있음
        }

        Rio_writen(proxy_fd, req_client_to_proxy, strlen(req_client_to_proxy)); // client > proxy > (request) > tiny
        printf("req: %s", req_client_to_proxy);

        if (strcmp(req_client_to_proxy, "\r\n") == 0) { // TODO: 요청 끝 맞는지 확인 필요
            break;
        }
    }

    printf("\n\n================ [PROXY][RESPONSE][FROM_TINY] ================\n\n");
    rio_t proxy_rio;
    Rio_readinitb(&proxy_rio, proxy_fd);
    char res_proxy_from_tiny[MAXBUF] = "";

    while (readn = Rio_readlineb(&proxy_rio, res_proxy_from_tiny, MAXBUF) != 0) {      // clinet < proxy < (response) < tiny
        if (strncmp(req_client_to_proxy, "Connection:", strlen("Connection:")) == 0) { // 헤더 수정
            strcpy(req_client_to_proxy, "Proxy-Connection: close\r\n");                // TODO: value는 뒤에 것 파싱해서 넣어줄 필요 있음
        }

        Rio_writen(client_fd, res_proxy_from_tiny, MAXBUF); // client < (response) < proxy < tiny
        printf("res: %s", res_proxy_from_tiny);
    }
    // Rio_writen(client_fd, "\r\n\r\n", strlen("\r\n\r\n")); // client < (response) < proxy < tiny
    Close(proxy_fd);
    printf("\n\n================ [PROXY][END] ================\n\n");
    printf("Close Connection..");
}

// tiny 응답 처리 시
// (v) 1. Header Proxy-Connection 떼고 Connection 로 넣어주기
// 2. content-length 만큼 준다.