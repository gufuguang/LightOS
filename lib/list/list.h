#ifndef __LIB_LIST_H
#define __LIB_LIST_H
#include "stdint.h"

#define MEMBER_OFFSET(struct_type, member) (int)(&((struct_type*)0)->member)
#define MEMBER2ENTRY(struct_type, struct_member_name, member_ptr) \
            (struct_type*)((int)member_ptr - MEMBER_OFFSET(struct_type, struct_member_name))

typedef struct list_node {
    struct list_node* prev;
    struct list_node* next;
}LIST_NODE;

typedef struct list {
    LIST_NODE head;
    LIST_NODE tail;
}LIST;

typedef bool (function)(LIST_NODE*, int arg);

void list_init(LIST*);
void list_add_before(LIST_NODE* before, LIST_NODE* node);
void list_add_head(LIST* list_head, LIST_NODE* node);
void list_add_tail(LIST* list_tail, LIST_NODE* node);
void list_iterate(LIST* list_head);
void list_remove(LIST_NODE* node);
LIST_NODE* list_delete(LIST* list_head);
bool list_empty(LIST* list_head);
uint32_t list_len(LIST* list_head);
LIST_NODE* list_traversal(LIST* list_head, function func, int arg);
bool list_find(LIST* list_head, LIST_NODE* node);

#endif