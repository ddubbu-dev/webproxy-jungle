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

    char path[MAXBUF], hostname[MAXBUF], port[MAXBUF];
    update_tiny_info(uri, path, hostname, port); // uri parse를 통한 정보 업데이트

    printf("===========[Proxy  --(Request)-->  Tiny]===========\n");
    int proxy_tiny_fd = Open_clientfd(hostname, port);
    char http_first_line[MAXBUF];
    sprintf(http_first_line, "%s %s %s\r\n", method, path, version);

    // 1. cache 존재 여부 체크
    // Yes: 캐시 사용
    RequestInfo *req_p = (RequestInfo *)malloc(sizeof(RequestInfo));
    strcmp(req_p->method, method);
    req_p->path = path;

    CacheNode *cache_node = search(dll, req_p);

    if (cache_node != NULL) {
        Rio_writen(proxy_tiny_fd, cache_node->res_p, strlen(cache_node->res_p));
        moveFront(dll, cache_node);
        return;
    }

    // No: 요청 새로 보내기 > 캐시 저장
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
    char *res_p = (char *)malloc(MAXBUF * sizeof(char)); // 메모리 할당
    res_p[0] = '\0';                                     // 빈 문자열로 초기화

    char server_response[MAXBUF];
    rio_t proxy_rio;
    Rio_readinitb(&proxy_rio, proxy_tiny_fd);
    while ((readn = Rio_readlineb(&proxy_rio, server_response, MAXBUF)) > 0) { // client > proxy <-(res)-- tiny
        Rio_writen(client_proxy_fd, server_response, readn);                   // client <-(res)-- proxy < tiny
        strcat(res_p, server_response);
        printf("[res] %s", server_response);
    }

    req_p = (RequestInfo *)malloc(sizeof(RequestInfo));
    pushFront(dll, req_p, res_p);
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