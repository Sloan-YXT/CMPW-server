#pragma once
#ifndef DATA
struct node
{
    char *sensor;
    char *clientName;
};
typedef struct node *DataNode;
struct list
{
    struct node *body;
    struct list *next;
};
typedef struct list *DataList;
int addSensorInfomation(char *name, char *sensor);
DataNode findNode(char *name);
void deleteNodeByName(char *name);
void listInit(void);
#define DATA
#endif
