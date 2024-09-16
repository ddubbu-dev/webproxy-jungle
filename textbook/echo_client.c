#include "csapp.h"

/**
 * main함수가 CLI 인자를 받을 때 사용하는 매개변수
 * - argc (argument count) : 항상 1 이상, 실행파일 이름 자체가 첫번째 인자로 포함되기 때문
 * - argv (argument vector) : 인자들의 배열
 */
int main(int argc, char **argv) {
    rio_t rio; // 읽기 버퍼 관리 구조체
    /**
     * Q1. 읽기 버퍼란?
     * A1. 파일이나 스트림에서 읽어들인 데이터를 임시로 저장하는 메모리
     *
     * Q2. 임시로 저장하는 이유?
     * A2. 디스크 I/O는 상대적으로 느리기 때문에 여러 바이트를 한번에 읽어서 애플리케이션이 더 빠르게 처리 가능
     *
     * Q3. unbuffered 방식을 원한다면?
     * A3. rio_readn 함수를 사용하면 됨. 지금은 rio_readlineb
     */

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    // Open_clientfd
    // [1] getaddrinfo : host, port 정보 기반으로
    // [2] socket      : socket fd 생성
    // [3] connect     : host 서버에 연결
    char *host = argv[1], *port = argv[2];
    int socketfd = Open_clientfd(host, port);

    char clinetRequest[MAXLINE];  // 사용자 입력
    char serverResponse[MAXLINE]; // 서버로부터의 응답을 저장할 버퍼

    Rio_readinitb(&rio, socketfd);                                  // rio 구조체를 socketfd와 연결된 소켓으로 초기화
    while (Fgets(clinetRequest, MAXLINE, stdin) != NULL) {          // (표준 입력, stdin) 한줄씩 사용자 입력 읽기
        Rio_writen(socketfd, clinetRequest, strlen(clinetRequest)); // [4] client request
        Rio_readlineb(&rio, serverResponse, MAXLINE);               // [5] server response (최대 MAXLINE 한 줄 응답 기대)
        Fputs(serverResponse, stdout);                              // (표준 출력, stdout)
    }
    Close(socketfd); // [6] 연결 종료
    exit(0);
}