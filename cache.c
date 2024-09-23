#include "csapp.h"
#include <stdio.h>
#include <stdlib.h>

#include "cache.h"

DLL *createDoublyLinkedList() {
    DLL *new_dll = (DLL *)calloc(1, sizeof(DLL));
    CacheNode *head_node = (CacheNode *)calloc(1, sizeof(CacheNode));

    new_dll->head = head_node;
    head_node->next = head_node->prev = head_node;
    new_dll->size = 0;
    return new_dll;
}

CacheNode *createNode(RequestInfo req_p, char *res_p, int res_size) {
    CacheNode *node = (CacheNode *)calloc(1, sizeof(CacheNode));

    // request 복사
    node->req_p = req_p; // fyi. 구조체를 직접 할당 시 복사본이 만들어지기 때문에 외부에서 req_p가 변경되어도 괜찮다.

    // response 복사
    node->res_p = (char *)malloc(res_size);
    memcpy(node->res_p, res_p, res_size);
    node->res_size = res_size;

    // TODO: response info 구조체 만들기
    // TODO: response header 추가하기

    node->prev = NULL;
    node->next = NULL;

    return node;
}

// [LRU] 앞에 최신정보 넣기
void pushFront(DLL *dll, RequestInfo req_p, char *res_p, int res_size) {
    CacheNode *new_node = createNode(req_p, res_p, res_size);

    new_node->next = dll->head->next;
    dll->head->next->prev = new_node;
    dll->head->next = new_node;
    new_node->prev = dll->head;

    dll->size++;
    printf("[push front] method=%s, path=%s\n", new_node->req_p.method, new_node->req_p.path);
    printDll(dll);

    // TODO: 개수 세서 오버시 popBack
}

// [LRU] 뒤에서 과거정보 제거
void popBack(DLL *dll) {
    if (dll->size == 0) {
        printf("List is empty, cannot pop.\n");
        return -1;
    }

    CacheNode *pop_node = dll->head->prev;

    dll->head->prev = pop_node->prev;
    pop_node->prev->next = dll->head;
    printf("[pop back] method=%s, path=%s\n", pop_node->req_p.method, pop_node->req_p.path);
    printDll(dll);

    free(pop_node->res_p);
    free(pop_node);
    dll->size--;

    // TODO: 제거 후 모든 속성 free
}

CacheNode *search(DLL *dll, RequestInfo req_info) {
    if (dll == NULL || dll->head == NULL) {
        printf("DLL or head is NULL\n");
        return NULL;
    }

    CacheNode *find_node = dll->head->next;

    while (find_node != dll->head) {
        if (!strcmp(find_node->req_p.method, req_info.method) && !strcmp(find_node->req_p.path, req_info.path)) {
            printf("[search][yes] method=%s, path=%s\n", find_node->req_p.method, find_node->req_p.path);
            return find_node;
        }
        find_node = find_node->next;
    }

    printf("[search][no] method=%s, path=%s\n", req_info.method, req_info.path);
    return NULL;
}

void moveFront(DLL *dll, CacheNode *move_node) {
    move_node->prev->next = move_node->next;
    move_node->next->prev = move_node->prev;

    move_node->next = dll->head->next;
    dll->head->next->prev = move_node;
    dll->head->next = move_node;
    move_node->prev = dll->head;
    printDll(dll);
}

void findAndMoveFront(DLL *dll, RequestInfo req_p) {
    CacheNode *move_node = search(dll, req_p);
    move_node->prev->next = move_node->next;
    move_node->next->prev = move_node->prev;

    move_node->next = dll->head->next;
    dll->head->next->prev = move_node;
    dll->head->next = move_node;
    move_node->prev = dll->head;
    printDll(dll);
}

void printDll(DLL *dll) {
    printf("[앞] ");
    if (dll->size == 0) {
        printf("[]\n");
        return;
    }

    CacheNode *now = dll->head->next;
    while (now != dll->head->prev) {
        printf("(%s, %s) ←→ ", now->req_p.method, now->req_p.path);
        now = now->next;
    }
    printf("(%s, %s) ←→ \n\n", now->req_p.method, now->req_p.path);
}

void deleteList(DLL *dll) {
    CacheNode *delete_node = dll->head->next;
    while (delete_node != dll->head) {
        CacheNode *next_node = delete_node->next;
        free(delete_node);
        delete_node = next_node;
    }
    free(dll->head);
    free(dll);
}

// fyi: 주석 풀려면 req_p 포인터 구조체가 아니라, 일반 값으로 캐스팅 필요
// int main() {// test
// DLL *dll = newDll();

// RequestInfo req_p; // action1
// strcpy(req_p.method, "GET");
// req_p.path = "/home.html";
// pushFront(dll, &req_p, "RESPONSE1");

// // pointer 이기에 새로 생성하지 않으면 덮어씌워짐
// req_p = (RequestInfo *)malloc(sizeof(RequestInfo)); // action2
// RequestInfo *saved_req_p = req_p;
// strcpy(req_p.method, "POST");
// req_p.path = "/sample.txt";
// pushFront(dll, &req_p, "RESPONSE2");

// req_p = (RequestInfo *)malloc(sizeof(RequestInfo)); // action3
// strcpy(req_p.method, "OPTION");
// req_p.path = "/sample33.txt";
// pushFront(dll, &req_p, "RESPONSE3");

// findAndMoveFront(dll, saved_req_p);
// popBack(dll);
// return 0;
// }
