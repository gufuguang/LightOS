// add by gufuguang 2021/2/16

#include "list.h"
#include "interrupt.h"

/*************************************************************************
 *  初始化双向链表节点
*************************************************************************/
void list_init(LIST* list_node)
{
    list_node->head.prev = NULL;
    list_node->head.next = &list_node->tail;
    list_node->tail.prev = &list_node->head;
    list_node->tail.next = NULL;
}
/*************************************************************************
 *  在before节点之前插入一个节点
*************************************************************************/
void list_add_before(LIST_NODE* before, LIST_NODE* node)
{
    INTR_STATUS old_tatus = intr_disable();

    before->prev->next = node;
    node->prev = before->prev;
    node->next = before;
    before->prev = node;

    intr_set_status(old_tatus);
}
/*************************************************************************
 *  在before节点之前插入一个节点
*************************************************************************/
void list_add_head(LIST* list_head, LIST_NODE* node)
{
    list_add_before(list_head->head.next, node);
}
/*************************************************************************
 *  在尾部插入一个节点
*************************************************************************/
void list_add_tail(LIST* list_tail, LIST_NODE* node)
{
    list_add_before(&list_tail->tail, node);
}
/*************************************************************************
 *  删除一个节点
*************************************************************************/
void list_remove(LIST_NODE* node)
{
    INTR_STATUS old_status = intr_disable();

    node->prev->next = node->next;
    node->next->prev = node->prev;

    intr_set_status(old_status);
}
/*************************************************************************
 *  删除链表中第一个节点
*************************************************************************/
LIST_NODE* list_delete(LIST* list_head)
{
    LIST_NODE* node = list_head->head.next;
    list_remove(node);
    
    return node;
}
/*************************************************************************
 *  查找节点是否在链表中
*************************************************************************/
bool list_find(LIST* list_head, LIST_NODE* obj_node)
{
    LIST_NODE* node = list_head->head.next;

    while(node != &list_head->tail)
    {
        if(node == obj_node) {
            return true;
        }
        node = node->next;
    }

    return false;
}
/*************************************************************************
 *  查找符合条件的节点
*************************************************************************/
LIST_NODE* list_traversal(LIST* list_head, function func, int arg)
{
    LIST_NODE* node = list_head->head.next; 

    if(list_empty(list_head))
    {
        return NULL;
    }

    while(node != &list_head->tail)
    {
        if(func(node, arg)) 
        {
            return node;
        }
        node = node->next;
    }

    return NULL;
}
/*************************************************************************
 *  计算链表中的节点个数
*************************************************************************/
uint32_t list_len(LIST* list_head)
{
    LIST_NODE* node = list_head->head.next;
    uint32_t len = 0;

    while(node != &list_head->tail)
    {
        len++;
        node = node->next;
    }

    return len;
}
/*************************************************************************
 *  判断链表是否为空
*************************************************************************/
bool list_empty(LIST* list_head)
{
    return (list_head->head.next == &list_head->tail ? true : false);
}