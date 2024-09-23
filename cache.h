#include "csapp.h"
#define METHOD_MAX_LEN 8

typedef struct RequestInfo {
    // [확장성 대응] request 식별을 위한 속성이 늘어날 것을 대비
    char method[METHOD_MAX_LEN];
    char path[MAXLINE];
} RequestInfo;

typedef struct ResponseInfo {
    char *header; // malloc, free 필요
    char *body;   // malloc, free 필요
    int body_size;
} ResponseInfo;

typedef struct CacheNode {
    RequestInfo req;
    ResponseInfo res;
    struct CacheNode *prev;
    struct CacheNode *next;
} CacheNode;

typedef struct DoublyLinkedList {
    CacheNode *head;
    int size;
} DLL;

DLL *createDoublyLinkedList();
CacheNode *search(DLL *dll, RequestInfo req);
void pushFront(DLL *dll, RequestInfo req, ResponseInfo res);
void moveFront(DLL *dll, CacheNode *move_node);
