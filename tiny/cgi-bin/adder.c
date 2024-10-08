/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
#include "csapp.h"
#include <string.h>

int main(void) {
    char *buf, *p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1 = -1, n2 = -1;

    /* Extract the two arguments */
    if ((buf = getenv("QUERY_STRING")) != NULL) {
        p = strchr(buf, '&');
        *p = '\0';
        strcpy(arg1, buf);
        strcpy(arg2, p + 1);
        n1 = atoi(arg1);
        n2 = atoi(arg2);
    }

    if (strstr(getenv("CONTENT_TYPE"), "html") != NULL) {
        /* Make the response body */
        sprintf(content, "QUERY_STRING=%s", buf);
        sprintf(content, "Welcome to add.com: ");
        sprintf(content, "%sThe Internet addition portal. \r\n", content);
        sprintf(content, "<p>%sThe answer is: %d + %d = %d\r\n</p>", content, n1, n2, n1 + n2);
        sprintf(content, "%sThanks for visiting!\r\n", content);

        /*Generate the HTTP response */
        printf("Connection: close\r\n");
        printf("Content-length: %d\r\n", (int)strlen(content));
        printf("Content-type: text/html\r\n\r\n");
        printf("%s", content);
    } else {
        /* Make the response body */
        sprintf(content, "{\"result\": %d}\r\n", n1 + n2);
        /*Generate the HTTP response */
        printf("Connection: close\r\n");
        printf("Content-length: %d\r\n", (int)strlen(content));
        printf("Content-type: application/json\r\n\r\n");
        printf("%s", content);
    }

    fflush(stdout);
    exit(0);
}
