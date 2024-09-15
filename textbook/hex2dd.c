/**
 *
 * [컴파일 및 링크]
 * $ cd textbook
 * $ gcc -I.. hex2dd.c ../csapp.c -o hex2dd.exe
 *
 * [실행 방법]
 * `$ ./hex2dd.exe 0x400`
 */

#include "csapp.h"

int main(int argc, char **argv) {
    uint16_t hex_host_byte;
    struct in_addr inaddr;
    char result_ip[MAXBUF];

    // 16진수 형식으로 커맨드 인자 읽어오기
    if (sscanf(argv[1], "%hx", &hex_host_byte) != 1) {
        fprintf(stderr, "Invalid hex number\n");
        exit(EXIT_FAILURE);
    }
    inaddr.s_addr = htons(hex_host_byte); // short = 16bit | (host byte order) to (network byte order)

    // IPv4 주소로 변환
    if (!inet_ntop(AF_INET, &inaddr, result_ip, MAXBUF)) {
        fprintf(stderr, "error: inet_ntop\n");
        exit(EXIT_FAILURE);
    }

    printf("IPv4 Address: %s\n", result_ip);
    exit(EXIT_SUCCESS);
}
