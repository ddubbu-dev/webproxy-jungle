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
    RequestInfo req_p;
    char *res_p;
    int res_size;
    struct CacheNode *prev;
    struct CacheNode *next;
} CacheNode;

typedef struct DoublyLinkedList {
    CacheNode *head;
    int size;
} DLL;

// 밖으로 보낼 함수만
DLL *createDoublyLinkedList();
CacheNode *search(DLL *, RequestInfo);
void pushFront(DLL *dll, RequestInfo req_p, char *res_p, int res_size);
void moveFront(DLL *dll, CacheNode *move_node);

// 미방출할 함수들
CacheNode *createNode(RequestInfo req_p, char *res_p, int res_size);
void popBack(DLL *dll);
CacheNode *search(DLL *dll, RequestInfo req_p);
void findAndMoveFront(DLL *dll, RequestInfo req_p);
void printDll(DLL *dll);
void deleteList(DLL *dll);