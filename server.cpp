#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/types.h>
#define __USE_POSIX
//#define __USE_BSD
#include <signal.h>
#include <setjmp.h>
#include <hash_map>
#include <stack>
#include <string>
#include <atomic>
#include <nlohmann/json.hpp>
#include <iostream>
#include <iomanip>
#include <list>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <map>
#include <sys/select.h>
#include "database.h"
#define portAgraph 6667
#define portAdata 6666
#define portBdata 7777
#define portBgraph 7778
#define portTick 6668
#define portBother 7779
#define MESSAE_LENGTH 1000
#define REQUEST_LENGTH 1000
#define ANUM 10
#define BNUM 1000000
#define BCBEFORE "not verified yet"
#define BBBEFORE "not connect yet"
#define STORE_HOUR 3
#define DEBUG
#ifdef DEBUG
#define DEBUG(X)                              \
    do                                        \
    {                                         \
        printf("debug:%d,%s\n", __LINE__, X); \
    } while (0)
#else
#define DEBUG(X)
#endif
#define ERROR_ACTION(X)                                   \
    if ((X) == -1)                                        \
    {                                                     \
        printf("error %s:%d", strerror(errno), __LINE__); \
        exit_database();                                  \
        exit(1);                                          \
    }
using namespace std;
using namespace nlohmann;
string user_dir, graph_dir;
int gepfd[4];
static int signum;
time_t clock_after;
using namespace std;
struct epoll_event a_data_event[ANUM];
struct epoll_event a_graph_event[ANUM];
struct epoll_event b_connect_event[BNUM];
//stack<int> freeV;
template <class T>
class WrapStack
{
private:
    pthread_mutex_t dlock;
    stack<T> vdata;

public:
    WrapStack()
    {
        pthread_mutex_init(&dlock, NULL);
    }
    ~WrapStack()
    {
        pthread_mutex_destroy(&dlock);
    }
    auto push(int dat)
    {
        return vdata.push(dat);
    }
    auto pop(void)
    {
        return vdata.pop();
    }
    auto top(void)
    {
        return vdata.top();
    }
    void lock(void)
    {
        pthread_mutex_lock(&dlock);
    }
    void unlock(void)
    {
        pthread_mutex_unlock(&dlock);
    }
    auto empty(void)
    {
        return vdata.empty();
    }
};
WrapStack<int> freeV;

struct Num
{
private:
    static int curA;
    static int curB;
    pthread_mutex_t lockA;
    pthread_mutex_t lockB;

public:
    void increaseA()
    {
        pthread_mutex_lock(&lockA);
        curA++;
        pthread_mutex_unlock(&lockA);
    }
    void decreaseA()
    {
        pthread_mutex_lock(&lockA);
        curA--;
        pthread_mutex_unlock(&lockA);
    }
    void increaseB()
    {
        pthread_mutex_lock(&lockB);
        curB++;
        pthread_mutex_unlock(&lockB);
    }
    void decreaseB()
    {
        pthread_mutex_lock(&lockB);
        curB--;
        pthread_mutex_unlock(&lockB);
    }
    Num()
    {
        curA = 0;
        curB = 0;
        pthread_mutex_init(&lockA, NULL);
        pthread_mutex_init(&lockB, NULL);
    }
    ~Num()
    {
        pthread_mutex_destroy(&lockA);
        pthread_mutex_destroy(&lockB);
    }
};
int Num::curA;
int Num::curB;
class BNodeInfo
{
public:
    int fd_data;
    int fd_graph;
    int fd_other;
    struct sockaddr_in clientData;
    string client_name;
    string board_name;
};
template <class V>
class WrapList
{
private:
    pthread_mutex_t dlock;
    list<V> data;

public:
    WrapList()
    {
        pthread_mutex_init(&dlock, NULL);
    }
    ~WrapList()
    {
        pthread_mutex_destroy(&dlock);
    }
    auto push_back(V dat)
    {
        return data.push_back(dat);
    }
    auto back()
    {
        return data.back();
    }
    auto begin()
    {
        return data.begin();
    }
    auto end()
    {
        return data.end();
    }
    auto pop_back()
    {
        return data.push_back();
    }
    auto remove(V dat)
    {
        return data.remove(dat);
    }
    auto erase(decltype(data.begin()) dat)
    {
        return data.erase(dat);
    }
    void lock(void)
    {
        pthread_mutex_lock(&dlock);
    }
    void unlock(void)
    {
        pthread_mutex_unlock(&dlock);
    }
};
class ANodeInfo
{
public:
    int fd_data;
    int fd_graph;
    int fd_tick;
    int vcode;
    struct sockaddr_in client_data, client_graph, client_tick;
    string message;
    string name;
    string position;
    string temp;
    string humi;
    string work_dir;
    unsigned int wood_time = 0;
    // ANodeInfo *data_node;
    ANodeInfo *gdata_node;
    WrapList<BNodeInfo *> connection;
};
bool operator==(const ANodeInfo &a, const ANodeInfo &b)
{
    return a.name == b.name;
}
template <class KEY, class VALUE>
class WrapMap
{
private:
    pthread_mutex_t dlock;
    unordered_map<KEY, VALUE> nodes;

public:
    WrapMap()
    {
        //cout << "debug:wrap hash_map init" << endl;
        pthread_mutex_init(&dlock, NULL);
    }
    ~WrapMap()
    {
        //cout << "debug:wrap hash_map destroyed" << endl;
        pthread_mutex_destroy(&dlock);
    }
    auto find(string key)
    {
        //cout << "debug:find" << endl;
        return nodes.find(key);
    }
    auto insert(pair<string, ANodeInfo *> &dat)
    {
        //cout << "debug:insert" << endl;
        return nodes.insert(dat);
    }
    auto emplace(string key, ANodeInfo *val)
    {
        //cout << "debug:emplace" << endl;
        return nodes.emplace(key, val);
    }
    auto erase(string key)
    {
        //cout << "debug:erase" << endl;
        return nodes.erase(key);
    }
    auto begin()
    {
        //cout << "debug:begin" << endl;
        return nodes.begin();
    }
    auto end()
    {
        //cout << "debug:end" << endl;
        return nodes.end();
        //cout << "debug:after end" << endl;
    }
    auto count(string key)
    {
        //cout << "debug:count" << endl;
        //pthread_mutex_lock(&lock);
        return nodes.count(key);
        //pthread_mutex_unlock(&lock);
    }
    auto size()
    {
        //cout << "debug:size" << endl;
        return nodes.size();
    }
    void lock(void)
    {
        //cout << "debug:lock" << endl;
        pthread_mutex_lock(&dlock);
    }
    void unlock(void)
    {
        //cout << "debug:unlock" << endl;
        pthread_mutex_unlock(&dlock);
    }
};
string randomName(void)
{
    char tmp[20];
    time_t time_now = time(NULL);
    sprintf(tmp, "%ld", time_now);
    return tmp;
}
WrapMap<string, ANodeInfo *> nodesA;
WrapList<ANodeInfo> *nodesATick;
//不同板子的连接共享一份数据是不行的，在一个进程多个连接时必须想办法把数据和每个连接绑定
unsigned int curA, curB;
Num numer;
char *ip_addr = "0.0.0.0";
int listenAdata, listenAgraph, listenAtick, listenBdata, listenBgraph, listenBother;
#define exit ::exit
#define free ::free
#define memset ::memset
void clean_sock(void)
{
    perror("clean check error:");
    close(listenAdata);
    close(listenAgraph);
    close(listenAtick);
    close(listenBdata);
    close(listenBgraph);
    delete nodesATick;
    string tmp = user_dir + "/graph";
    DEBUG("in clean");
    for (int i = 0; i < 4; i++)
    {
        close(gepfd[i]);
    }
    DEBUG("begore rm graph dir");
    //execlp("rm", "rm", "-rf", tmp.c_str(), NULL);
    DEBUG("clean failed");
}
void *Adata(void *arg)
{
    printf("Adata:%d\n", syscall(__NR_gettid));
    struct epoll_event ev;
    string type;
    int epfd = (long)arg;
    int nfds;
    int n;
    int socket;
    int i;
    ANodeInfo *a_info;
    char message_box[MESSAE_LENGTH];
    DEBUG("Adata start");
    while (1)
    {
        DEBUG("Adata working");
        nfds = epoll_wait(epfd, a_data_event, ANUM, -1);
        DEBUG("Adata epoll shit");
        if (nfds < 0)
        {
            if (errno == 4)
            {
                continue;
            }
            perror("epoll wait failed in Aread");
            exit_database();
            exit(1);
        }
        DEBUG("Adata epoll wait success");
        for (i = 0; i < nfds; i++)
        {
            if (a_data_event[i].events & EPOLLIN)
            {
                a_info = (ANodeInfo *)(a_data_event[i].data.ptr);
                if (a_info == NULL)
                {
                    printf("error in A read:NULL pointer\n");
                    exit(1);
                }
                unsigned int len;
                DEBUG("before recv len");
                n = recv(a_info->fd_data, &len, sizeof(int), MSG_WAITALL);
                if (n == 0 | n < 0)
                {
                    if (n < 0 && errno != ECONNRESET)
                    {
                        perror("recv failed in A");
                        close(a_info->fd_data);
                        close(a_info->fd_graph);
                        close(a_info->fd_tick);
                        delete a_info;
                        exit_database();
                        exit(1);
                    }
                    else if (n == 0 | errno == ECONNRESET)
                    {
                        char *p = (char *)malloc(20);
                        printf("%d:board[%s:%d] has disconnected:%s\n", __LINE__, inet_ntop(AF_INET, &a_info->client_data.sin_addr, p, 20), ntohs(a_info->client_data.sin_port), strerror(errno));
                        free(p);
                        if (epoll_ctl(epfd, EPOLL_CTL_DEL, a_info->fd_data, NULL) == -1)
                        {
                            printf("A read epoll del failed %d:%s", __LINE__, strerror(errno));
                            exit_database();
                            exit(1);
                        }
                        numer.decreaseA();
                        cout << a_info->name << endl;
                        nodesA.lock();
                        auto c = nodesA.find(a_info->name);
                        if (c != nodesA.end())
                        {
                            for (auto b = c->second->connection.begin(); b != c->second->connection.end(); c++)
                            {
                                json j;
                                j["type"] = "cmd";
                                j["content"] = "breset";
                                string a = j.dump();
                                int m = send((*b)->fd_data, a.c_str(), a.size(), 0);
                                if (m <= 0 && errno != EPIPE)
                                {
                                    printf("send breset failed in %d:%s\n", __LINE__, strerror(errno));
                                    exit_database();
                                    exit(1);
                                }
                            }
                            DEBUG("");
                        }
                        nodesA.erase(a_info->name);
                        DEBUG("after erase");
                        nodesA.unlock();
                        close(a_info->fd_data);
                        //close(a_info->fd_graph);
                        //close(a_info->fd_tick);
                        freeV.lock();
                        freeV.push(a_info->vcode);
                        freeV.unlock();
                        delete a_info;
                        numer.decreaseA();
                        DEBUG("");
                        continue;
                    }
                }
                len = ntohl(len);
                n = recv(a_info->fd_data, message_box, len, MSG_WAITALL);

                if (n == 0 | n < 0)
                {
                    if (n < 0 && errno != ECONNRESET)
                    {
                        perror("recv failed in A");
                        close(a_info->fd_data);
                        close(a_info->fd_graph);
                        close(a_info->fd_tick);
                        delete a_info;
                        exit_database();
                        exit(1);
                    }
                    else if (n == 0 | errno == ECONNRESET)
                    {
                        char *p = (char *)malloc(20);
                        printf("%d:board %s:[%s:%d] has disconnected:%s\n", __LINE__, a_info->name.c_str(), inet_ntop(AF_INET, &a_info->client_data.sin_addr, p, 20), ntohs(a_info->client_data.sin_port), strerror(errno));
                        free(p);
                        if (epoll_ctl(epfd, EPOLL_CTL_DEL, a_info->fd_data, NULL) == -1)
                        {
                            printf("A read epoll del failed %d:%s", __LINE__, strerror(errno));
                            exit_database();
                            exit(1);
                        }
                        numer.decreaseA();
                        ::puts("normal disconnect!");
                        nodesA.lock();
                        auto c = nodesA.find(a_info->name);
                        if (c != nodesA.end())
                        {
                            for (auto b = c->second->connection.begin(); b != c->second->connection.end(); c++)
                            {
                                json j;
                                j["type"] = "cmd";
                                j["content"] = "breset";
                                string a = j.dump();
                                int m = send((*b)->fd_data, a.c_str(), a.size(), 0);
                                if (m <= 0 && errno != EPIPE)
                                {
                                    printf("send breset failed in %d:%s\n", __LINE__, strerror(errno));
                                    exit_database();
                                    exit(1);
                                }
                            }

                            nodesA.erase(a_info->name);
                        }
                        nodesA.unlock();
                        DEBUG("after erase");
                        close(a_info->fd_data);
                        //close(a_info->fd_graph);
                        //close(a_info->fd_tick);
                        freeV.lock();
                        freeV.push(a_info->vcode);
                        freeV.unlock();
                        numer.decreaseA();
                        delete a_info;
                    }
                }
                else
                {
                    DEBUG("recv message");
                    message_box[n] = 0;
                    DEBUG(message_box);
                    a_info->message = message_box;
                    json data = json::parse(message_box);
                    type = data["type"];
                    if (type == "data")
                    {
                        unsigned int wood_time = a_info->wood_time;
                        if (wood_time == 0)
                        {
                            wood_time = a_info->wood_time = time(NULL);
                            printf("\n\n\njust for one time should it be\n\n\n");
                            save_board_data(message_box);
                        }
                        clock_after = time(NULL);
                        unsigned int sec = difftime(clock_after, wood_time);
                        if (sec >= 60 * 60 * STORE_HOUR)
                        {
                            printf("\n\n\nsecsecsec:%d\n\n\n", sec);
                            save_board_data(message_box);
                            a_info->wood_time = clock_after;
                        }
                        DEBUG("");
                        a_info->name = data["name"];
                        DEBUG("");
                        nodesA.lock();
                        DEBUG("");
                        auto p = nodesA.find(a_info->name);
                        // if (p != nodesA.end())
                        DEBUG("");
                        if (p != nodesA.end())
                        {
                            p->second->message = message_box;
                            p->second->position = data["position"];
                            p->second->temp = data["temp"];
                            p->second->humi = data["humi"];
                            json reply;
                            DEBUG("");
                            reply["type"] = "data";
                            DEBUG("");
                            reply["boardName"] = data["name"];
                            reply["temp"] = data["temp"];
                            DEBUG("");
                            reply["humi"] = data["humi"];
                            DEBUG("");
                            reply["position"] = data["position"];
                            string reply_string = reply.dump();
                            for (auto m = p->second->connection.begin(); m != p->second->connection.end(); m++)
                            {
                                int fd_tmp = (*m)->fd_data;
                                int len_tmp = reply_string.size();
                                len_tmp = htonl(len_tmp);
                                n = send(fd_tmp, &len_tmp, sizeof(len_tmp), 0);
                                if (n < 0 && errno == EPIPE)
                                {
                                    // close((*m)->fd_data);
                                    // p->second->connection.remove(*m);
                                }
                                else if (n <= 0)
                                {
                                    perror("send len to client failed");
                                    exit_database();
                                    exit(1);
                                }
                                n = send(fd_tmp, reply_string.c_str(), reply_string.size(), 0);
                                if (n < 0 && errno == EPIPE)
                                {
                                    // close((*m)->fd_data);
                                    // p->second->connection.remove(*m);
                                }
                                else if (n <= 0)
                                {
                                    perror("send data to client failed");
                                    exit_database();
                                    exit(1);
                                }
                            }
                        }
                        nodesA.unlock();
                    }

                    else if (type == "cmd")
                    {
                        string name, content;
                        name = data["name"];
                        content = data["content"];
                        if (content == "logout")
                        {
                            char *p = (char *)malloc(20);
                            printf("%s:%d logout:%s\n", inet_ntop(AF_INET, &a_info->client_data.sin_addr, p, 20), ntohs(a_info->client_data.sin_port), strerror(errno));
                            free(p);
                            epoll_ctl(epfd, EPOLL_CTL_DEL, a_info->fd_data, &ev);
                            nodesA.lock();
                            auto c = nodesA.find(name);
                            if (c != nodesA.end())
                            {
                                for (auto b = c->second->connection.begin(); b != c->second->connection.end(); c++)
                                {
                                    json j;
                                    j["type"] = "cmd";
                                    j["content"] = "breset";
                                    string a = j.dump();
                                    int m = send((*b)->fd_data, a.c_str(), a.size(), 0);
                                    if (m <= 0 && errno != EPIPE)
                                    {
                                        printf("send breset failed in %d:%s\n", __LINE__, strerror(errno));
                                        exit_database();
                                        exit(1);
                                    }
                                }
                                nodesA.erase(name);
                                int vcode = a_info->vcode;
                                freeV.lock();
                                freeV.push(vcode);
                                freeV.unlock();
                            }
                            nodesA.unlock();
                            close(a_info->fd_data);
                            //close(a_info->fd_graph);
                            //close(a_info->fd_tick);
                            delete a_info;
                            numer.decreaseA();
                        }
                    }
                }
            }
            else if (a_data_event[i].events & EPOLLERR)
            {
                a_info = (ANodeInfo *)a_data_event[i].data.ptr;
                printf("in %d:fd %d", __LINE__, a_info->fd_data);
                perror("epoll wait error");
                exit(1);
            }
            else if (a_data_event[i].events & EPOLLHUP)
            {
                DEBUG("");
                a_info = (ANodeInfo *)a_data_event[i].data.ptr;
                char *p = (char *)malloc(20);
                cout << "debug:epoll hup" << endl;
                printf("board %s:[%s:%d] has disconnected:%s(net broke)\n", a_info->name.c_str(), inet_ntop(AF_INET, &a_info->client_data.sin_addr, p, 20), ntohs(a_info->client_data.sin_port), strerror(errno));
                free(p);
                close(a_info->fd_data);
                delete a_info;
            }
        }
    }
}
void *Agraph(void *arg)
{
    printf("Agraph:%d\n", syscall(__NR_gettid));
    struct epoll_event ev;
    int epfd = (long)arg;
    char *graph_buffer;
    int nfds;
    int len;
    int connfd;

    ANodeInfo *info;
    string tmp_file;
    int gfd;
    int n;
    while (1)
    {
        DEBUG("Agraph working");
        nfds = epoll_wait(epfd, a_graph_event, ANUM, -1);
        DEBUG("Agraph epoll shit");
        switch (nfds)
        {
        case -1:
            if (errno == 4)
            {
                continue;
            }
            perror("epoll wait failed in Agraph");
            exit_database();
            exit(1);
            break;
        default:
            for (int i = 0; i < nfds; i++)
            {
                if (a_graph_event[i].events & EPOLLIN)
                {
                    info = (ANodeInfo *)a_graph_event[i].data.ptr;
                    cout << "debug:" << info->work_dir << endl;
                    printf("debug:in Agraph:%s:%s\n", info->name.c_str(), info->work_dir.c_str());
                    connfd = info->fd_graph;
                    DEBUG("in Agraph");
                    n = recv(connfd, &len, sizeof(len), MSG_WAITALL);
                    printf("debug in Agraph:%d:n=%d\n", __LINE__, n);
                    ERROR_ACTION(n)
                    if (n == 0)
                    {
                        DEBUG("N==0");
                        if (epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL) == -1)
                        {
                            printf("A read epoll del failed %d:%s", __LINE__, strerror(errno));
                            exit_database();
                            exit(1);
                        }
                        DEBUG("before close fd_graph");
                        close(connfd);
                        DEBUG("before delete");
                        delete info;
                        DEBUG("after delete");
                        continue;
                    }
                    DEBUG("in Agraph");
                    len = ntohl(len);
                    printf("in line%d:len=%d\n", __LINE__, len);
                    printf("debug%d:%s\n", __LINE__, info->work_dir.c_str());
                    cout << info->work_dir << endl;
                    tmp_file = info->work_dir + "/" + randomName();
                    gfd = open(tmp_file.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0777);
                    if (gfd < 0)
                    {
                        printf("%s\n", tmp_file.c_str());
                        perror("");
                        exit(1);
                    }
                    lseek(gfd, len - 1, SEEK_SET);
                    n = write(gfd, "\0", 1);
                    lseek(gfd, 0, SEEK_SET);
                    ERROR_ACTION(n);
                    printf("debug:line%d:%d\n", __LINE__, len);
                    if ((graph_buffer = (char *)mmap(NULL, len, PROT_WRITE | PROT_READ, MAP_SHARED, gfd, 0)) == MAP_FAILED)
                    {
                        //FAIL:INVALID ARGUMENT
                        perror("mhash_map failed in sendfile");
                        exit(1);
                    }
                    n = recv(connfd, graph_buffer, len, MSG_WAITALL);
                    ERROR_ACTION(n)
                    if (n == 0)
                    {
                        if (epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL) == -1)
                        {
                            printf("A read epoll del failed %d:%s", __LINE__, strerror(errno));
                            exit_database();
                            exit(1);
                        }
                        close(connfd);
                        delete info;
                        ERROR_ACTION(munmap(graph_buffer, len));
                        continue;
                    }
                    else
                    {
                        nodesA.lock();
                        for (auto m = info->connection.begin(); m != info->connection.end(); m++)
                        {
                            int rlen = htonl(len);
                            n = send((*m)->fd_graph, &rlen, sizeof(rlen), 0);
                            if (n < 0 && errno == EPIPE)
                            {
                                // close((*m)->fd_graph);
                                // info->connection.remove(*m);
                            }
                            else if (n <= 0)
                            {
                                perror("send glen to client failed");
                                exit_database();
                                exit(1);
                            }
                            n = send((*m)->fd_graph, graph_buffer, len, 0);
                            if (n < 0 && errno == EPIPE)
                            {
                                // close((*m)->fd_graph);
                                // info->connection.remove(*m);
                            }
                            else if (n <= 0)
                            {
                                perror("send gdata to client failed");
                                exit_database();
                                exit(1);
                            }
                        }
                        nodesA.unlock();
                    }
                    ERROR_ACTION(munmap(graph_buffer, len));
                    close(gfd);
                }
                else if (a_graph_event[i].events & EPOLLERR)
                {
                    info = (ANodeInfo *)a_graph_event[i].data.ptr;
                    printf("in %d:fd %d", __LINE__, info->fd_data);
                    perror("epoll wait error");
                }
                else if (a_graph_event[i].events & EPOLLHUP)
                {
                    info = (ANodeInfo *)a_graph_event[i].data.ptr;
                    char *p = (char *)malloc(20);
                    cout << "debug:epoll hup(g)" << endl;
                    printf("board %s:[%s:%d] has disconnected:%s(net broke)\n", info->name.c_str(), inet_ntop(AF_INET, &info->client_data.sin_addr, p, 20), ntohs(info->client_data.sin_port), strerror(errno));
                    free(p);
                    close(info->fd_graph);
                    delete info;
                }
            }
            break;
        }
        DEBUG("");
    }
    DEBUG("");
}
struct TickData
{
    int epfdData;
    int epfdGraph;
};
sigjmp_buf env;
void timeOut(int signo)
{
    cout << "fuck!!!" << endl;
    siglongjmp(env, 1);
}
void *TickTock(void *arg)
{
    printf("ticktock:%d\n", syscall(__NR_gettid));
    TickData *tickData = (TickData *)arg;
    int epfdData = tickData->epfdData;
    int epfdGraph = tickData->epfdGraph;
    int n;
    char buffer[100];
    delete tickData;
    stack<ANodeInfo> waitForFuck;
    DEBUG("");
    while (1)
    {
        DEBUG("");
        cout << boolalpha;
        nodesATick->lock();
        fd_set fds;
        int maxfd;
        for (auto a = nodesATick->begin(); a != nodesATick->end();)
        {

            printf("tick debug:");
            DEBUG(a->name.c_str());
            printf("debug:fd_tick:%d\n", a->fd_tick);
            /*
            if (sigsetjmp(env, 1) != 0)
            {
                alarm(0);

                DEBUG("");
                shutdown((*a).fd_data, SHUT_RDWR);
                DEBUG("");
                shutdown(a->fd_graph, SHUT_RDWR);
                DEBUG("");
                printf("debug:delete name:%s\n", a->name.c_str());
                //close(a->second->fd_tick);
                cout<<"debug:"<<__LINE__<<":"<<&a<<endl;
                a = nodesATick->erase(a);
                //waitForFuck.push(*a);
                // nodesATick->erase(a->second->name);
                // delete a->second;
                DEBUG("after tick erase");
                continue;
            }
            */
            DEBUG("before tick recv");
            FD_ZERO(&fds);
            FD_SET(a->fd_tick, &fds);
            maxfd = a->fd_tick + 1;
            timeval out = {5, 0};
            n = select(maxfd, &fds, NULL, NULL, &out);
            ERROR_ACTION(n);
            if (n == 0)
            {
                DEBUG("");
                shutdown((*a).fd_data, SHUT_RDWR);
                DEBUG("");
                shutdown(a->fd_graph, SHUT_RDWR);
                DEBUG("");
                printf("debug:delete name:%s\n", a->name.c_str());
                close(a->fd_tick);
                a = nodesATick->erase(a);
                DEBUG("after tick erase");
                continue;
            }
            else
            {
                n = recv(a->fd_tick, buffer, 100, 0);
                if (n == 0 | errno == ECONNRESET)
                {
                    DEBUG("");
                    shutdown(a->fd_data, SHUT_RDWR);
                    DEBUG("");
                    shutdown(a->fd_graph, SHUT_RDWR);
                    DEBUG("");
                    a = nodesATick->erase(a);
                }
                else if (n < 0 && errno != ECONNRESET)
                {
                    perror("recv failed in tick thread");
                    exit(1);
                }
                else
                {
                    DEBUG(buffer);
                    a++;
                }
            }
            DEBUG("after tick recv");
        }
        DEBUG("Tick thread still in working");
        nodesATick->unlock();
        sleep(10);
        //for(int i=90000000;i>=0;i--);
    }
}
void *Bconnect(void *arg)
{
    printf("Bconnect:%d\n", syscall(__NR_gettid));
    struct epoll_event ev;
    int epfd = (long)arg;
    int nfds;
    int n;
    char request[REQUEST_LENGTH];
    BNodeInfo *info;
    int fd;
    int i;
    int len;
    while (1)
    {
        DEBUG("Bconn working");
        nfds = epoll_wait(epfd, b_connect_event, BNUM, -1);
        DEBUG("Bconn epoll shit");
        switch (nfds)
        {
        case -1:
            if (errno == 4)
            {
                continue;
            }
            perror("epoll wait failed in Bread");
            printf("%d\n", errno);
            exit_database();
            exit(1);
        case 0:
            continue;
        default:
            for (i = 0; i < nfds; i++)
            {
                if (b_connect_event[i].events & EPOLLIN)
                {
                    info = (BNodeInfo *)b_connect_event[i].data.ptr;
                    fd = info->fd_other;
                    n = recv(fd, &len, 4, MSG_WAITALL);
                    if (n == 0 | errno == ECONNRESET)
                    {
                        char *p = (char *)malloc(20);
                        time_t now;
                        now = time(NULL);
                        printf("%s:socket error:connection with board%s [%s:%d]:%s\n", asctime(localtime(&now)), info->client_name.c_str(), inet_ntop(AF_INET, &info->clientData.sin_addr, p, 20), ntohs(info->clientData.sin_port), strerror(errno));
                        free(p);
                        ERROR_ACTION(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev));
                        close(fd);
                        close(info->fd_graph);
                        close(info->fd_data);

                        if (info->board_name != BBBEFORE)
                        {
                            nodesA.lock();
                            auto c = nodesA.find(info->board_name);
                            if (c != nodesA.end())
                            {
                                c->second->connection.remove(info);
                                c->second->gdata_node->connection.remove(info);
                            }
                            nodesA.unlock();
                        }
                        delete info;
                        numer.decreaseB();
                        continue;
                    }
                    else if (n < 0 && errno != ECONNRESET)
                    {
                        perror("recv failed in Bread");
                        exit_database();
                        exit(1);
                    }

                    else
                    {
                        len = ntohl(len);
                        n = recv(fd, request, len, MSG_WAITALL);
                        if (n == 0 | errno == ECONNRESET)
                        {
                            char *p = (char *)malloc(20);
                            time_t now;
                            now = time(NULL);
                            printf("%s:socket error:connection with board%s [%s:%d]:%s\n", asctime(localtime(&now)), info->client_name.c_str(), inet_ntop(AF_INET, &info->clientData.sin_addr, p, 20), ntohs(info->clientData.sin_port), strerror(errno));
                            free(p);
                            ERROR_ACTION(epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev));
                            close(fd);
                            close(info->fd_graph);
                            close(info->fd_data);

                            if (info->board_name != BBBEFORE)
                            {
                                nodesA.lock();
                                auto c = nodesA.find(info->board_name);
                                if (c != nodesA.end())
                                {
                                    c->second->connection.remove(info);
                                    c->second->gdata_node->connection.remove(info);
                                }
                                nodesA.unlock();
                            }
                            delete info;
                            numer.decreaseB();
                        }
                        else if (n < 0 && errno != ECONNRESET)
                        {
                            perror("recv failed in Bread");
                            exit_database();
                            exit(1);
                        }
                        else
                        {
                            json j = json::parse(request);
                            string type = j["type"];
                            if (type == "connect")
                            {
                                string boardName = j["board name"];
                                string clientName = j["cleint name"];
                                info->board_name = boardName;
                                info->client_name = clientName;
                                nodesA.lock();
                                auto c = nodesA.find(boardName);

                                c->second->connection.push_back(info);
                                c->second->gdata_node->connection.push_back(info);

                                nodesA.unlock();
                                int vcode = c->second->vcode;
                                vcode = htonl(vcode);
                                n = send(fd, &vcode, sizeof(vcode), 0);
                                if (n < 0 && errno == EPIPE)
                                {
                                }
                                else if (n <= 0)
                                {
                                    printf("send failed in %d:%s\n", __LINE__, strerror(errno));
                                    exit_database();
                                    exit(1);
                                }
                            }
                        }
                    }
                }
                else if (b_connect_event[i].events & EPOLLHUP)
                {
                    perror("epoll err in B connection");
                    exit_database();
                    exit(1);
                }
            }
        }
    }
}
void *
AThread(void *arg)
{
    printf("AThread:%d\n", syscall(__NR_gettid));
    struct sockaddr_in server_data, server_graph, client_data, client_graph;
    struct sockaddr_in server_tick, client_tick;
    struct epoll_event ev1, ev2;
    pthread_t adata, agraph, atick;
    int connfdData, connfdGraph, connfdTick;
    int epfdData, epfdGraph;
    epfdData = epoll_create(ANUM);
    epfdGraph = epoll_create(ANUM);
    TickData *tickData = new (TickData);
    tickData->epfdData = epfdData;
    tickData->epfdGraph = epfdGraph;
    if (epfdData == -1 | epfdGraph == -1)
    {
        perror("epfd create failed");
        exit(1);
    }
    gepfd[0] = epfdData;
    gepfd[1] = epfdGraph;

    ANodeInfo *a_info_1, *a_info_2;
    listenAgraph = socket(AF_INET, SOCK_STREAM, 0);
    listenAdata = socket(AF_INET, SOCK_STREAM, 0);
    listenAtick = socket(AF_INET, SOCK_STREAM, 0);
    if (listenAgraph == -1 | listenAdata == -1 | listenAtick == -1)
    {
        perror("error create TCP socket");
        exit(1);
    }
    memset(&server_data, 0, sizeof(server_data));
    memset(&server_graph, 0, sizeof(server_graph));
    memset(&server_tick, 0, sizeof(server_tick));
    server_data.sin_family = AF_INET;
    server_data.sin_port = htons(portAdata);
    server_graph.sin_family = AF_INET;
    server_graph.sin_port = htons(portAgraph);
    server_tick.sin_family = AF_INET;
    server_tick.sin_port = htons(portTick);
    socklen_t client_data_addr_len = sizeof(client_data);
    socklen_t client_graph_addr_len = sizeof(client_graph);
    socklen_t client_tick_addr_len = sizeof(client_tick);
    if (inet_aton(ip_addr, &server_data.sin_addr) == 0)
    {
        perror("address transferring error");
        exit(1);
    }
    if (inet_aton(ip_addr, &server_graph.sin_addr) == 0)
    {
        perror("address transferring error");
        exit(1);
    }
    if (inet_aton(ip_addr, &server_tick.sin_addr) == 0)
    {
        perror("address transferring error");
        exit(1);
    }
    if (bind(listenAdata, (struct sockaddr *)&server_data, sizeof(server_data)) == -1)
    {
        perror("error while trying to bind on portAdata");
        exit(1);
    }
    if (bind(listenAgraph, (struct sockaddr *)&server_graph, sizeof(server_graph)) == -1)
    {
        perror("error while trying to bind on portAgraph");
        exit(1);
    }
    if (bind(listenAtick, (struct sockaddr *)&server_tick, sizeof(server_tick)) == -1)
    {
        perror("error while trying to bind on portAtick");
        exit(1);
    }
    if (listen(listenAdata, ANUM * 30) == -1)
    {
        printf("%d\n", listenAdata);
        perror("error while trying to listen to Adata");
        exit(1);
    }
    if (listen(listenAgraph, ANUM * 30) == -1)
    {
        printf("%d\n", listenAgraph);
        perror("error while trying to listen to Agraph");
        exit(1);
    }
    if (listen(listenAtick, ANUM * 30) == -1)
    {
        printf("%d\n", listenAgraph);
        perror("error while trying to listen to Agraph");
        exit(1);
    }
    ERROR_ACTION(pthread_create(&adata, NULL, Adata, (void *)epfdData))
    ERROR_ACTION(pthread_create(&agraph, NULL, Agraph, (void *)epfdGraph))
    ERROR_ACTION(pthread_create(&atick, NULL, TickTock, tickData))
    int len_tmp;
    char message_buffer[200];
    char reply[200];
    int n;
    while (1)
    {
        DEBUG("ATH working");
        connfdData = accept(listenAdata, (struct sockaddr *)&client_data, &client_data_addr_len);
        if (connfdData < 0)
        {
            perror("error accepting from board(data)");
            exit(1);
        }
        DEBUG("");
        n = recv(connfdData, &len_tmp, sizeof(len_tmp), MSG_WAITALL);
        DEBUG("");
        ERROR_ACTION(n)
        if (n == 0)
        {
            close(connfdData);
            continue;
        }
        len_tmp = ntohl(len_tmp);
        n = recv(connfdData, message_buffer, len_tmp, MSG_WAITALL);
        if (n < 0 && errno != ECONNRESET)
        {
            exit(1);
        }
        if (n == 0 | errno == ECONNRESET)
        {
            close(connfdData);
            continue;
        }
        message_buffer[n] = 0;
        nodesA.lock();
        n = nodesA.count(message_buffer);
        nodesA.unlock();
        int rcode, code;
        if (n != 0)
        {
            code = -1;
            strcpy(reply, "already has a board name");
            strcat(reply, message_buffer);
        }

        else
        {
            DEBUG(message_buffer);
            freeV.lock();
            if (freeV.empty())
            {
                DEBUG("");
                code = -1;
                strcpy(reply, "Video reaches maximum number of connections");
            }
            else
            {
                DEBUG("");
                code = freeV.top();
                freeV.pop();

                strcpy(reply, "conratulations,connection has been set");
            }
            freeV.unlock();
        }
        DEBUG("");
        rcode = htonl(code);
        n = send(connfdData, &rcode, sizeof(rcode), 0);
        DEBUG("");
        if (n <= 0)
        {
            if (errno == EPIPE)
            {
                close(connfdData);
                continue;
            }
            else
            {
                exit_database();
                exit(1);
            }
        }
        len_tmp = strlen(reply);
        len_tmp = htonl(len_tmp);
        n = send(connfdData, &len_tmp, sizeof(len_tmp), 0);
        if (n <= 0)
        {
            if (errno == EPIPE)
            {
                close(connfdData);
                continue;
            }
            else
            {
                exit_database();
                exit(1);
            }
        }
        n = send(connfdData, reply, strlen(reply), 0);
        if (n <= 0)
        {
            if (errno == EPIPE)
            {
                close(connfdData);
                continue;
            }
            else
            {
                exit_database();
                exit(1);
            }
        }
        if (code == -1)
        {
            recv(connfdData, message_buffer, 100, 0);
            close(connfdData);
            continue;
        }
        //TO DO 这样的逻辑可能导致卡死，需要设置超时
        fd_set accept_tout;
        int maxfd, n;
        timeval tout = {1, 0};
        FD_ZERO(&accept_tout);
        FD_SET(listenAgraph, &accept_tout);
        maxfd = listenAgraph + 1;
        n = select(maxfd, &accept_tout, NULL, NULL, &tout);
        if (n < 0)
        {
            printf("select failed in %d:%s\n", __LINE__, strerror(errno));
            exit(1);
        }
        else if (n == 0)
        {
            close(connfdData);
            continue;
        }
        connfdGraph = accept(listenAgraph, (struct sockaddr *)&client_graph, &client_graph_addr_len);
        if (connfdData < 0)
        {
            perror("error accepting from board(graph)");
            exit(1);
        }
        FD_ZERO(&accept_tout);
        FD_SET(listenAtick, &accept_tout);
        maxfd = listenAtick + 1;
        n = select(maxfd, &accept_tout, NULL, NULL, &tout);
        if (n < 0)
        {
            printf("select failed in %d:%s\n", __LINE__, strerror(errno));
            exit(1);
        }
        else if (n == 0)
        {
            close(connfdData);
            close(connfdGraph);
            continue;
        }
        connfdTick = accept(listenAtick, (struct sockaddr *)&client_tick, &client_tick_addr_len);
        if (connfdData < 0)
        {
            perror("error accepting from board(tick)");
            exit(1);
        }
        DEBUG(message_buffer);
        a_info_1 = new ANodeInfo;
        DEBUG("LET ME KNOW");
        a_info_1->vcode = code;
        DEBUG("");
        string s = message_buffer;
        cout << s << endl;
        //a_info_1->name = s;
        DEBUG("");
        a_info_1->client_data = client_data;
        DEBUG("");
        a_info_1->client_graph = client_graph;
        DEBUG("");
        a_info_1->client_tick = client_tick;
        DEBUG("");
        a_info_1->fd_data = connfdData;
        DEBUG("");
        a_info_1->fd_graph = connfdGraph;
        DEBUG("");
        a_info_1->fd_tick = connfdTick;
        DEBUG("");
        a_info_1->wood_time = 0;
        DEBUG("");
        //   a_info_1->name = "test";
        //   cout<<a_info_1->name<<endl;
        a_info_1->name = s;
        //a_info_1->data_node = a_info_1;
        DEBUG("LET ME KNOW2");
        a_info_2 = new ANodeInfo;
        DEBUG("LET ME KNOW3");
        a_info_1->gdata_node = a_info_2;
        a_info_2->vcode = code;
        a_info_2->name = message_buffer;
        a_info_2->client_data = client_data;
        a_info_2->client_graph = client_graph;
        a_info_2->client_tick = client_tick;
        a_info_2->fd_data = connfdData;
        a_info_2->fd_graph = connfdGraph;
        a_info_2->fd_tick = connfdTick;
        a_info_2->wood_time = 0;
        ev1.data.ptr = a_info_1;
        ev1.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP;
        ev2.data.ptr = a_info_2;
        ev2.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP;
        //有风险：最好做2份回调数据，不清楚epoll原理
        a_info_1->work_dir = user_dir + "/graph" + "/" + a_info_1->name;
        a_info_2->work_dir = user_dir + "/graph" + "/" + a_info_2->name;
        cout << "debug:" << a_info_2->work_dir << endl;
        if ((access(a_info_1->name.c_str(), F_OK)))
        {
            mkdir(a_info_1->work_dir.c_str(), 0777);
        }
        ERROR_ACTION(epoll_ctl(epfdData, EPOLL_CTL_ADD, connfdData, &ev1))
        ERROR_ACTION(epoll_ctl(epfdGraph, EPOLL_CTL_ADD, connfdGraph, &ev2))
        DEBUG("before add element");
        nodesA.lock();
        nodesA.emplace(message_buffer, a_info_1);
        nodesA.unlock();
        ANodeInfo info_spare;
        info_spare.fd_data = connfdData;
        info_spare.fd_graph = connfdGraph;
        info_spare.fd_tick = connfdTick;
        info_spare.name = message_buffer;
        nodesATick->lock();
        nodesATick->push_back(info_spare);
        nodesATick->unlock();
        DEBUG("after add element");
        DEBUG("before create dir");

        DEBUG("after create workdir");
        numer.increaseA();
    }
    DEBUG("ATH quiting");
}

void *BThread(void *arg)
{
    printf("BThread:%d", syscall(__NR_gettid));
    BNodeInfo *info;
    pthread_t bthread;
    int epfd;
    int fdpro;
    epfd = epoll_create(BNUM);
    ERROR_ACTION(epfd)
    gepfd[2] = epfd;
    struct sockaddr_in serverData, serverGraph, serverOther, clientData, clientGraph, clientOther;
    struct epoll_event ev;
    int connfdData, connfdGraph, connfdOther;
    ERROR_ACTION(listenBdata = socket(AF_INET, SOCK_STREAM, 0))
    ERROR_ACTION(listenBgraph = socket(AF_INET, SOCK_STREAM, 0))
    listenBdata = socket(AF_INET, SOCK_STREAM, 0);
    listenBgraph = socket(AF_INET, SOCK_STREAM, 0);
    listenBother = socket(AF_INET, SOCK_STREAM, 0);
    pthread_create(&bthread, NULL, Bconnect, (void *)epfd);
    memset(&serverData, 0, sizeof(serverData));
    serverData.sin_family = AF_INET;
    serverData.sin_port = htons(portBdata);
    memset(&serverGraph, 0, sizeof(serverGraph));
    serverGraph.sin_family = AF_INET;
    serverGraph.sin_port = htons(portBgraph);
    memset(&serverOther, 0, sizeof(serverOther));
    serverOther.sin_family = AF_INET;
    serverOther.sin_port = htons(portBother);
    socklen_t client_addr_data_len = sizeof(clientData);
    socklen_t client_addr_graph_len = sizeof(clientGraph);
    socklen_t client_addr_other_len = sizeof(clientOther);
    if (inet_aton(ip_addr, &serverData.sin_addr) == 0)
    {
        perror("address transferring error");
        exit(1);
    }
    if (bind(listenBdata, (struct sockaddr *)&serverData, sizeof(serverData)) == -1)
    {
        perror("error while trying to bind on portBdata\n");
        exit(1);
    }
    if (listen(listenBdata, BNUM) == -1)
    {
        printf("%d\n", listenBdata);
        perror("error while trying to listen to Bdata\n");
        exit(1);
    }
    if (inet_aton(ip_addr, &serverGraph.sin_addr) == 0)
    {
        perror("address transferring error");
        exit(1);
    }
    if (bind(listenBgraph, (struct sockaddr *)&serverGraph, sizeof(serverGraph)) == -1)
    {
        perror("error while trying to bind on portA\n");
        exit(1);
    }
    if (listen(listenBgraph, BNUM) == -1)
    {
        printf("%d\n", listenBgraph);
        perror("error while trying to listen to B\n");
        exit(1);
    }
    if (inet_aton(ip_addr, &serverOther.sin_addr) == 0)
    {
        perror("address transferring error");
        exit(1);
    }
    if (bind(listenBother, (struct sockaddr *)&serverOther, sizeof(serverOther)) == -1)
    {
        perror("error while trying to bind on portA\n");
        exit(1);
    }
    if (listen(listenBother, BNUM) == -1)
    {
        printf("%d\n", listenBgraph);
        perror("error while trying to listen to B\n");
        exit(1);
    }
    while (1)
    {
        DEBUG("BTH working");
        connfdData = accept(listenBdata, (struct sockaddr *)&clientData, &client_addr_data_len);
        //printf("%s","beta of sigint!\n");
        if (connfdData < 0)
        {
            perror("error accepting from android:");
            exit(1);
        }
        connfdGraph = accept(listenBgraph, (struct sockaddr *)&clientGraph, &client_addr_graph_len);
        if (connfdGraph < 0)
        {
            perror("error accepting from android:");
            exit(1);
        }
        connfdOther = accept(listenBother, (sockaddr *)&clientOther, &client_addr_other_len);
        if (connfdOther < 0)
        {
            perror("error accepting from android:");
            exit(1);
        }
        info = new BNodeInfo;
        ev.events = EPOLLIN | EPOLLET | EPOLLERR;
        ev.data.ptr = info;
        info->clientData = clientData;
        info->fd_data = connfdData;
        info->fd_other = connfdOther;
        info->fd_graph = connfdGraph;
        info->client_name = BCBEFORE;
        info->board_name = BBBEFORE;
        nodesA.lock();
        json Bdata;
        int num = nodesA.size();
        Bdata["num"] = num;
        json j, m;
        int i = 0;
        for (auto a = nodesA.begin(); a != nodesA.end(); a++)
        {
            m["name"] = a->second->name;
            m["position"] = a->second->position;
            j[i] = m;
            i++;
        }
        nodesA.unlock();
        Bdata["nodes"] = j;
        string data_string = j.dump();
        int len = data_string.length();
        len = htonl(len);
        int n = send(connfdOther, &len, sizeof(len), 0);
        if (n < 0 && errno == SIGPIPE)
        {
            close(connfdData);
            close(connfdGraph);
            close(connfdOther);
            delete info;
            continue;
        }
        else if (n <= 0)
        {
            exit_database();
            exit(1);
        }
        n = send(connfdOther, data_string.c_str(), data_string.size(), 0);
        if (n < 0 && errno == SIGPIPE)
        {
            close(connfdData);
            close(connfdGraph);
            close(connfdOther);
            delete info;
            continue;
        }
        else if (n <= 0)
        {
            exit_database();
            exit(1);
        }
        epoll_ctl(epfd, EPOLL_CTL_ADD, connfdOther, &ev);
        numer.increaseB();
    }
}
pthread_mutex_t freelock;
int ack;
void sigPipeHandler(int signo)
{
    signum = signo;
    printf("[recv SIGPIPE!]\n");
}
int main(void)
{
    nodesATick = new WrapList<ANodeInfo>;
    printf("main:%d\n", syscall(__NR_gettid));
    for (int i = 0; i < ANUM; i++)
    {
        freeV.push(i);
    }
    struct passwd *cur_user = getpwuid(getuid());
    user_dir = cur_user->pw_dir;
    graph_dir = user_dir + "/graph";
    if ((access(graph_dir.c_str(), F_OK)))
    {
        mkdir(graph_dir.c_str(), 0777);
    }
    struct sigaction sigpipe, sigalarm;
    sigemptyset(&sigalarm.sa_mask);
    sigalarm.sa_flags = 0;
    sigalarm.sa_flags |= SA_RESTART;
    sigalarm.sa_handler = timeOut;
    // if (sigaction(SIGALRM, &sigalarm, NULL) == -1)
    // {
    //     perror("sigaction error:");
    //     exit(1);
    // }
    sigemptyset(&sigpipe.sa_mask);
    sigpipe.sa_flags = 0;
    sigpipe.sa_flags |= SA_RESTART;
    sigpipe.sa_handler = sigPipeHandler;
    if (sigaction(SIGPIPE, &sigpipe, NULL) == -1)
    {
        perror("sigaction error:");
        exit(1);
    }
    database_init();
    atexit(clean_sock);
    pthread_t pA, pB;
    pthread_t pC;
    pthread_create(&pA, NULL, AThread, NULL);
    pthread_create(&pB, NULL, BThread, NULL);

    pthread_join(pA, NULL);
    pthread_join(pB, NULL);
    while (1)
        ;
}
