#include "data.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
struct list head;
static DataList tail;
extern pthread_mutex_t freelock;
extern int ack;
#define ERROR_ACTION(X)                                   \
    if (X == -1)                                          \
    {                                                     \
        printf("error %s:%d", strerror(errno), __LINE__); \
        exit(1);                                          \
    }
void listInit(void)
{
    head.body = malloc(sizeof(struct node));
    head.body->sensor = "head node";
    head.next = &head;
    tail = &head;
}
DataNode newNode(void)
{
    DataNode node = (DataNode)malloc(sizeof(struct node));
    return node;
}
DataNode findNode(char *name)
{
    DataList p = &head;
    int i = 0;
    for (p = p->next; p != &head; p = p->next)
    {
        //printf("%d%d%d%d%d%d\n",i,i,i,i,i,i);
        i++;
        if (strcmp(name, p->body->clientName) == 0)
        {
            return p->body;
        }
    }
    return NULL;
}
void addNode(DataNode node)
{
    DataList p;
    struct list *newList = malloc(sizeof(struct list));
    newList->body = node;
    newList->next = &head;
    tail->next = newList;
    tail = newList;
}
int addSensorInfomation(char *name, char *sensor)
{
    DataNode node;
    if ((node = findNode(name)) != NULL)
    {
        node->sensor = sensor;
        return 1;
    }
    else
    {
        node = newNode();
        node->clientName = name;
        node->sensor = sensor;
        addNode(node);
        return 0;
    }
}
void deleteNodeByName(char *name)
{
    //puts("deldeldeldeldeldeldeldeldeldeldeldel");
    DataList p, q;
    DataNode node;
    //ERROR_ACTION(pthread_mutex_lock(&freelock));
    if ((node = findNode(name)) != NULL)
    {
        for (p = &head, q = head.next; q != &head; p = p->next, q = q->next)
        {
            if (strcmp(q->body->clientName, name) == 0)
            {
                if (q == tail)
                {
                    tail = p;
                }
                p->next = q->next;
                break;
            }
        }
        if (q != &head)
        {
            ack = 1;
            free(q->body->clientName);
            free(q->body->sensor);
            free(q);
        }
    }
    //ERROR_ACTION(pthread_mutex_unlock(&freelock));
}