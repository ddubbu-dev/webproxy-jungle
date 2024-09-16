/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * ⭐ 눈여겨볼 점 : socket interface에서 http 통신으로 어떻게 바뀌는지
 * - 요청/응답 과정에서 (method, uri) 등의 정보가 설정됨
 * - 응답시 header, body 포맷이 존재함
 */
#include "csapp.h"
#define ROOT_URI '/'

void doit(int fd);
void read_requesthdrs(rio_t *rp);
void update_info_from_uri(char *uri, char *filename, char *cgiargs, int *is_static);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void response_clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

// textbook/echo_server.c 와 유사
int main(int argc, char **argv) {
    if (argc != 2) { // cli args : <exe_file_name> <port>
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
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
    char hostname[MAXLINE], port[MAXLINE];
    while (1) { // 서버 계속 실행
        clientaddr_size = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientaddr_size);                       // [5] Connection Request
        Getnameinfo((SA *)&clientaddr, clientaddr_size, hostname, MAXLINE, port, MAXLINE, 0); // 소켓 주소 구조체를 대응되는 호스트, 서비스 스트링으로 변환
        printf("Accepted connection from (%s, %s)\n", hostname, port);
        doit(connfd); // serve (static/dynamic) content
        Close(connfd);
    }
}

void doit(int fd) {
    /* Read request line and headers */
    char http_info[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    rio_t rio;
    Rio_readinitb(&rio, fd);                 // rio 구조체를 connfd와 연결된 소켓으로 초기화
    Rio_readlineb(&rio, http_info, MAXLINE); // Read request line
    printf("Request headers: %s\n", http_info);
    sscanf(http_info, "%s %s %s", method, uri, version); // http_info 분해
    if (strcasecmp(method, "GET")) {                     // GET일 경우 0 반환
        response_clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
        return;
    }
    read_requesthdrs(&rio);

    /* Parse URI and Update */
    int is_static = 0;
    char filename[MAXLINE], cgiargs[MAXLINE];
    update_info_from_uri(uri, filename, cgiargs, &is_static);

    struct stat file_state;
    if (stat(filename, &file_state) < 0) { // 파일 상태 및 정보 획득
        response_clienterror(fd, filename, "404", "Not found", "Tiny couldn't find this file");
        return;
    }

    if (is_static) {
        if (!(S_ISREG(file_state.st_mode)) || !(S_IRUSR & file_state.st_mode)) {
            // S_ISREG: 일반 파일인지
            // S_IRUSR & file_state.st_mode: 읽기 권한 비트 추출
            response_clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't read the file");
            return;
        }

        serve_static(fd, filename, file_state.st_size);
    } else {
        if (!(S_ISREG(file_state.st_mode)) || !(S_IXUSR & file_state.st_mode)) {
            // S_ISREG: 일반 파일인지
            // S_IXUSR & file_state.st_mode: 실행 권한 비트 추출

            response_clienterror(fd, filename, "403", "Forbidden", "Tiny couldn't run the CGI program");
            return;
        }
        serve_dynamic(fd, filename, cgiargs);
    }
}

void response_clienterror(int fd, char *cause, char *status_code, char *shortmsg, char *longmsg) {
    char res_header[MAXLINE], body[MAXBUF];

    // Build HTTP 응답 body
    sprintf(body, "<html><title>Tiny Error</title>");
    sprintf(body,
            "%s<body bgcolor="
            "ffffff"
            ">\r\n",
            body);
    sprintf(body, "%s%s: %s\r\n", body, status_code, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    // Build HTTP 응답 res_header
    sprintf(res_header, "HTTP/1.0 %s %s\r\n", status_code, shortmsg);
    Rio_writen(fd, res_header, strlen(res_header));
    sprintf(res_header, "Content-type: text/html\r\n");
    sprintf(res_header, "Content-length: %d\r\n\r\n", (int)strlen(body)); // body 먼저 생성 필요

    /**
     * 응답 결과 확인 방법
     * res_header: [개발자도구] > [네트워크] > [Headers]
     * body : [개발자도구] > [네트워크] > [Response]
     * */
    Rio_writen(fd, res_header, strlen(res_header));
    Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp) {
    /**
     * [request res_header 예시]
     * Connection
     * Pragma
     * Cache-Control
     * User-Agent
     * Accept
     * Referer
     * Accept-Encoding
     * Accept-Language
     */

    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n")) { // 개행 만나기 전까지 반복
        Rio_readlineb(rp, buf, MAXLINE);
        printf("%s", buf);
    }

    return;
}

void update_info_from_uri(char *uri, char *filename, char *cgiargs, int *is_static) {
    char *ptr;

    if (!strstr(uri, "cgi-bin")) { // Serve static content
        *is_static = 1;

        // 초기화
        strcpy(cgiargs, "");
        strcpy(filename, "."); // 현재 작업디렉토리
        strcat(filename, uri); // . + {uri}

        if (uri[strlen(uri) - 1] == ROOT_URI)
            strcat(filename, "home.html");

    } else { // Serve Dynamic content
        *is_static = 0;

        printf("uri=%s\n", uri);
        ptr = index(uri, '?'); // query string 시작 지점
        if (ptr) {
            strcpy(cgiargs, ptr + 1);
            *ptr = '\0'; // 문자열 종료 문자로 바꿔 URI에서 query string 제외
        } else
            strcpy(cgiargs, "");

        strcpy(filename, ".");
        strcat(filename, uri);
    }
}

void serve_static(int fd, char *filename, int filesize) {
    int srcfd;
    char *srcp, filetype[MAXLINE], res_header[MAXBUF];

    /* Send response headers to clinet */
    get_filetype(filename, filetype);
    sprintf(res_header, "HTTP/1.0 200 OK\r\n");
    sprintf(res_header, "%sServer: Tiny Web Server\r\n", res_header);
    sprintf(res_header, "%sConnection: close\r\n", res_header);
    sprintf(res_header, "%sContent-length: %d\r\n", res_header, filesize);
    sprintf(res_header, "%sContent-type: %s\r\n\r\n", res_header, filetype);
    Rio_writen(fd, res_header, strlen(res_header));
    printf("Response headers:\n");
    printf("%s", res_header);

    /* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0);                        // open file
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); // file을 메모리에 직접 매핑
    Close(srcfd);                                               // close file
    Rio_writen(fd, srcp, filesize);                             // 응답
    Munmap(srcp, filesize);                                     // mmap으로 매핑된 메모리 영역 해제
}

/**
 * get_filetypye - Derive file type from filename
 */
void get_filetype(char *filename, char *filetype) {
    if (strstr(filename, ".html"))
        strcpy(filetype, "text/html"); // filetype = "text/html" 할당 느낌
    else if (strstr(filename, ".gif"))
        strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
        strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
        strcpy(filetype, "image/jpg");
    else if (strstr(filename, ".mp4"))
        strcpy(filetype, "video/mp4");
    else
        strcpy(filetype, "text/plain");
}

void serve_dynamic(int fd, char *filename, char *cgiargs) {
    /**
     * [예시]
     * 1. HTTP GET http://{ip_v4}:8000/cgi-bin/adder?params1&params2
     * 2. CGI 스크립트 adder 호출 (querystring 1&2 전달) 및 응답
     */

    char res_header[MAXLINE], *emptylist[] = {NULL};

    /* HTTP response - Header 응답*/
    sprintf(res_header, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, res_header, strlen(res_header));
    sprintf(res_header, "Server:Tiny Web Server\r\n");
    Rio_writen(fd, res_header, strlen(res_header));

    /* HTTP response - Body 응답 (CGI 활용)*/

    if (Fork() == 0) {
        /* Child */
        /* Real server would set all CGI vars here */
        setenv("QUERY_STRING", cgiargs, 1);   // (마지막 인자, 1): overwrite
        Dup2(fd, STDOUT_FILENO);              // CGI program 출력을 client에 바로 응답
        Execve(filename, emptylist, environ); // Run CGI program
    }
    Wait(NULL); /* Parent waits for and reaps child */
}