#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
int main(void)
{
    alarm(0);alarm(0);alarm(0);
    sleep(5);
{printf("max:%d       min:%d\n",sched_get_priority_max(SCHED_RR),sched_get_priority_min(SCHED_RR));}
}