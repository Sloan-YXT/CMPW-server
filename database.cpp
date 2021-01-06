#include <mysql/mysql.h>
#include <nlohmann/json.hpp>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "cutil.h"
#include "data.h"
#include <string>
#include <iostream>
#define ATIMES 1
using namespace std;
using namespace nlohmann;
static MYSQL mydata_A;
static const char *database = "watcher";
static const char *create_board_tale =
    "create table if not exists board_data"
    "(id int(11)  primary key auto_increment,"
    "year char(4) not null,"
    "month char(3) not null,"
    "date char(3) not null,"
    "wday varchar(10) not null,"
    "ttime char(10) not null,"
    "board_name varchar(50) not null,"
    "board_location varchar(50) not null,"
    "temp varchar(10) not null,"
    "humi varchar(10) not null"
    ");";
char deleteA[100] =
    "delete from board_data where month !=";
void routine_delete()
{
    struct tm *now;
    time_t time_now = time(NULL);
    now = localtime(&time_now);
    static char year[10], month[10];
    sprintf(year, "%d", now->tm_yday + 1900);
    sprintf(month, "%d", now->tm_mon + 1);
    strcat(deleteA, month);
    strcat(deleteA, " and id > 0");
    puts(deleteA);
    if (mysql_query(&mydata_A, "SET SQL_SAFE_UPDATES = 0"))
    {
        printf("SET SQL_SAFE_UPDATES failed!\n");
        printf("MySQL query error : %s/n", mysql_error(&mydata_A));
        exit(1);
    }
    if (mysql_query(&mydata_A, deleteA))
    {
        printf("delete data failed!\n");
        printf("MySQL query error : %s/n", mysql_error(&mydata_A));
        exit(1);
    }
}
void database_init()
{
    if (mysql_library_init(0, NULL, NULL))
    {
        perror("mysql lib init failed");
        exit(1);
    }
    if (mysql_init(&mydata_A) == NULL)
    {
        printf("mysql data init failed:%d", mysql_error(&mydata_A));
        exit(1);
    }
    if (mysql_options(&mydata_A, MYSQL_SET_CHARSET_NAME, "utf8"))
    {
        printf("mysql connection init failed:%d", mysql_error(&mydata_A));
        exit(1);
    }
    if (NULL == mysql_real_connect(&mydata_A,"localhost", "root", "Yy649535675!", database, 3306, NULL, CLIENT_MULTI_STATEMENTS))
    {
        printf("connect failed!\n");
        perror("");
        exit(1);
    }
    MYSQL_RES *pRes;
    if (mysql_query(&mydata_A, create_board_tale))
    {
        printf("create board_table failed!\n");
        printf("MySQL query error : %s/n", mysql_error(&mydata_A));
        exit(1);
    };
    routine_delete();
    // do
    // {
    //     pRes = mysql_use_result(&mydata_A);
    //     mysql_free_result(pRes);
    // } while (!mysql_next_result(&mydata_A));
    // mysql_free_result(pRes);
}
void save_board_data(string json_data)
{
    string insert = "insert into board_data values(";
    json j = json::parse(json_data);
    string name = j["name"];
    string location = j["position"];
    string temp = j["temp"];
    string humi = j["humi"];
    char year[10], month[10], date[10], ttime[10], wday[10];
    time_t time_now = time(NULL);
    struct tm *now = localtime(&time_now);
    sprintf(year, "%d", now->tm_year + 1900);
    sprintf(month, "%d", now->tm_mon + 1);
    sprintf(date, "%d", now->tm_mday);
    sprintf(ttime, "%d:%d:%d", now->tm_hour, now->tm_min, now->tm_sec);
    sprintf(wday, "%d", now->tm_wday);
    insert = insert + "\'0\',\'" + year + "\',\'" + month + "\',\'" + date + "\',\'" + wday + "\',\'" +
             ttime + "\',\"" + name + "\",\"" + location + "\",\'" + temp + "\',\'" + humi + "\');";
    //cout<<insert<<endl;
    if (mysql_query(&mydata_A, insert.c_str()))
    {
        printf("insert board data failed!\n");
        printf("MySQL query error : %s/n", mysql_error(&mydata_A));
        exit(1);
    };
}
void exit_database()
{
    mysql_close(&mydata_A);
    mysql_library_end();
}