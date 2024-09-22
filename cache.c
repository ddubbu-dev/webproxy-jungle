#include "csapp.h"
#include <stdio.h>
#include <stdlib.h>

#define METHOD_MAX_LEN 8

typedef struct RequestInfo {
    char method[METHOD_MAX_LEN];
    char *path;
} RequestInfo;

typedef struct CacheNode {
    RequestInfo *req_p;
    char *res_p;
    struct CacheNode *prev;
    struct CacheNode *next;
} CacheNode;

typedef struct DoublyLinkedList {
    CacheNode *head;
    int size;
} dll;

dll *newDll() {
    dll *new_dll = (dll *)calloc(1, sizeof(dll));
    CacheNode *head_node = (CacheNode *)calloc(1, sizeof(CacheNode));
    new_dll->head = head_node;
    head_node->next = head_node->prev = head_node;
    new_dll->size = 0;
    return new_dll;
}

CacheNode *createNode(RequestInfo *req_p, char *res_p) {
    CacheNode *node = (CacheNode *)calloc(1, sizeof(CacheNode));
    node->req_p = req_p;
    node->res_p = res_p;
    node->prev = NULL;
    node->next = NULL;

    return node;
}

// 앞에 최신정보 넣기
void pushFront(dll *dll, RequestInfo *req_p, char *res_p) {
    CacheNode *new_node = createNode(req_p, res_p);

    new_node->next = dll->head->next;
    dll->head->next->prev = new_node;
    dll->head->next = new_node;
    new_node->prev = dll->head;

    dll->size++;
    printf("[push front] method=%s, path=%s\n", new_node->req_p->method, new_node->req_p->path);
    printDll(dll);
}

// 뒤에 빼기
void popBack(dll *dll) {
    if (dll->size == 0) {
        printf("List is empty, cannot pop.\n");
        return -1;
    }

    CacheNode *pop_node = dll->head->prev;

    dll->head->prev = pop_node->prev;
    pop_node->prev->next = dll->head;
    printf("[pop back] method=%s, path=%s\n", pop_node->req_p->method, pop_node->req_p->path);
    printDll(dll);

    free(pop_node);
    dll->size--;
}

CacheNode *search(dll *dll, RequestInfo *req_p) {
    CacheNode *find_node = dll->head->next;

    while (find_node != dll->head) {
        if (find_node->req_p->method == req_p->method && find_node->req_p->path == req_p->path) {
            printf("[search] method=%s, path=%s\n", find_node->req_p->method, find_node->req_p->path);
            return find_node;
        }
        find_node = find_node->next;
    }
    printf("[search] method=%s, path=%s\n", find_node->req_p->method, find_node->req_p->path);

    return NULL;
}

void findAndMoveFront(dll *dll, RequestInfo *req_p) {
    CacheNode *move_node = search(dll, req_p);
    move_node->prev->next = move_node->next;
    move_node->next->prev = move_node->prev;

    move_node->next = dll->head->next;
    dll->head->next->prev = move_node;
    dll->head->next = move_node;
    move_node->prev = dll->head;
    printDll(dll);
}

void printDll(dll *dll) {
    printf("[앞] ");
    if (dll->size == 0) {
        printf("[]\n");
        return;
    }

    CacheNode *now = dll->head->next;
    while (now != dll->head->prev) {
        printf("(%s, %s) ←→ ", now->req_p->method, now->req_p->path);
        now = now->next;
    }
    printf("(%s, %s) ←→ \n\n", now->req_p->method, now->req_p->path);
}

void deleteList(dll *dll) {
    CacheNode *delete_node = dll->head->next;
    while (delete_node != dll->head) {
        CacheNode *next_node = delete_node->next;
        free(delete_node);
        delete_node = next_node;
    }
    free(dll->head);
    free(dll);
}

int main() {
    dll *dll = newDll();

    RequestInfo *req_p = (RequestInfo *)malloc(sizeof(RequestInfo)); // action1
    strcpy(req_p->method, "GET");
    req_p->path = "/home.html";
    pushFront(dll, req_p, "RESPONSE1");

    // pointer 이기에 새로 생성하지 않으면 덮어씌워짐
    req_p = (RequestInfo *)malloc(sizeof(RequestInfo)); // action2
    RequestInfo *saved_req_p = req_p;
    strcpy(req_p->method, "POST");
    req_p->path = "/sample.txt";
    pushFront(dll, req_p, "RESPONSE2");

    req_p = (RequestInfo *)malloc(sizeof(RequestInfo)); // action3
    strcpy(req_p->method, "OPTION");
    req_p->path = "/sample33.txt";
    pushFront(dll, req_p, "RESPONSE3");

    findAndMoveFront(dll, saved_req_p);
    popBack(dll);
    return 0;
}