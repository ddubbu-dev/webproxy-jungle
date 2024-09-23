#include "cache.h"
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
                                    "Firefox/10.0.3\r\n";

void action(int client_proxy_fd);
void update_tiny_info(char *uri, char *path, char *hostname, char *port);
void *thread(void *vargp);

DLL *dll;

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN);

    dll = newDll();

    int listenfd = Open_listenfd(argv[1]);
    int *connfdp;
    pthread_t tid;

    struct sockaddr_storage clientaddr;
    socklen_t clientlen;
    while (1) {
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Pthread_create(&tid, NULL, thread, connfdp);
    }

    return 0;
}

void *thread(void *vargp) {
    int connfd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    action(connfd);
    Close(connfd);
    return NULL;
}

RequestInfo *req_p;
void action(int client_proxy_fd) {
    char client_request[MAXBUF], method[MAXBUF], uri[MAXBUF], version[MAXBUF];
    char path[MAXBUF], hostname[MAXBUF], port[MAXBUF];

    printf("===========[Client --(Request)--> Proxy]===========\n");
    rio_t client_rio;
    Rio_readinitb(&client_rio, client_proxy_fd);

    int readn = Rio_readlineb(&client_rio, client_request, MAXBUF); // client --(req)-> proxy > tiny
    sscanf(client_request, "%s %s %s", method, uri, version);

    if (strcmp(method, "GET") != 0) { //  (simple) http request 유효성 체크
        char err_msg[MAXLINE] = "NOT VALID request: Allow only [GET] method\r\n\r\n";
        Rio_writen(client_proxy_fd, err_msg, strlen(err_msg)); // TODO: HTTP err ststus
        return;
    }

    update_tiny_info(uri, path, hostname, port); // uri parse를 통한 정보 업데이트

    char http_first_line[MAXBUF];
    sprintf(http_first_line, "%s %s %s\r\n", method, path, version);

    printf("===========[Proxy Cache Check]===========\n");

    // cache 존재 여부 체크: YES
    // Yes: 캐시 사용
    RequestInfo *req_p = (RequestInfo *)malloc(sizeof(RequestInfo));
    strcpy(req_p->method, method);
    req_p->path = path;

    CacheNode *cache_node = search(dll, req_p);

    if (cache_node != NULL) {
        char content_length_buf[MAXBUF];
        sprintf(content_length_buf, "Content-length: %d\r\n", cache_node->res_size);

        printf("///////////// (시작) cache hit ////////////////\n");
        printf("[cache][res]%s", http_first_line);
        printf("[cache][res]%s", "\r\n");
        printf("[cache][res]Proxy-Connection: close\r\n");
        printf("[cache][res]%s\r\n", content_length_buf);
        printf("[cache][res] %s\n", cache_node->res_p);

        Rio_writen(client_proxy_fd, "HTTP/1.1 200 OK", strlen("HTTP/1.1 200 OK"));
        Rio_writen(client_proxy_fd, "Proxy-Connection: close\r\n", strlen("Proxy-Connection: close"));
        Rio_writen(client_proxy_fd, content_length_buf, strlen(content_length_buf));
        Rio_writen(client_proxy_fd, "\r\n", strlen("\r\n")); // 헤더 끝 표시
        Rio_writen(client_proxy_fd, cache_node->res_p, cache_node->res_size);
        printf("///////////// (끝) cache hit ////////////////\n");
        moveFront(dll, cache_node);
        return;
    }

    printf("===========[Proxy  --(Request)-->  Tiny]===========\n");
    int proxy_tiny_fd = Open_clientfd(hostname, port);
    if (proxy_tiny_fd <= 0) {
        printf("Tiny 서버가 꺼졌어요. 확인해주세요\n");
        return;
    }

    // Cache check No: 요청 새로 보내기 > 캐시 저장
    Rio_writen(proxy_tiny_fd, http_first_line, strlen(http_first_line)); // client > proxy --(req)-> tiny
    printf("[req] %s", http_first_line);

    while ((readn = Rio_readlineb(&client_rio, client_request, MAXBUF)) > 0) { // client --(req)-> proxy > tiny
        Rio_writen(proxy_tiny_fd, client_request, readn);                      // client > proxy --(req)-> tiny

        if (strcmp(client_request, "\r\n") == 0) {
            break;
        }
        printf("[req] %s", client_request);
    }
    printf("===========[Proxy  <--(Response)--  Tiny]===========\n");
    printf("===========[Client <--(Response)-- Proxy]===========\n");
    int content_length = 0;

    char server_response_header[MAXBUF];
    rio_t proxy_rio;
    Rio_readinitb(&proxy_rio, proxy_tiny_fd);

    // header 읽기
    // TODO: res_p에서 header 분리되면서 발생한 이슈
    while ((readn = Rio_readlineb(&proxy_rio, server_response_header, MAXBUF)) > 0) { // client > proxy <-(res)-- tiny
        Rio_writen(client_proxy_fd, server_response_header, readn);                   // client <-(res)-- proxy < tiny
        printf("[res] %s", server_response_header);

        // 헤더 끝 확인
        if (strcmp(server_response_header, "\r\n") == 0) {
            break; // 헤더 읽기 끝!
        }

        if (strncmp(server_response_header, "Content-length:", 15) == 0) {
            sscanf(server_response_header, "Content-length: %d\r\n", &content_length);
        }
    }
    // body 읽기
    char *server_response_body = (char *)malloc(content_length + 1);
    server_response_body[content_length] = '\0'; // NULL 종료 추가

    Rio_readnb(&proxy_rio, server_response_body, content_length);
    Rio_writen(client_proxy_fd, server_response_body, content_length); // client <-(res)-- proxy < tiny
    printf("[res] %s\n", server_response_body);
    printf("===========[끝]===========\n");

    req_p = (RequestInfo *)malloc(sizeof(RequestInfo));
    strcpy(req_p->method, method);
    req_p->path = path;
    pushFront(dll, req_p, server_response_body, content_length);
}

void update_tiny_info(char *uri, char *path, char *hostname, char *port) {
    // 포트가 있을 경우만 포트 추출
    if (strstr(uri, ":")) {
        sscanf(uri, "http://%99[^:]:%9[^/]/", hostname, port);
    } else {
        sscanf(uri, "http://%99[^/]/", hostname);
    }
    sscanf(uri, "http://%*[^/]%s", path); // TODO: path (/ 없을때 기본값) 설정 필요함
    printf("[update_tiny_info] hostname=%s, port=%s, path=%s\n", hostname, port, path);
}