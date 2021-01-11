#include "database.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unistd.h>
using namespace std;
int main(void)
{
    database_init();
    for (int i = 0; i < 10; i++)
    {
        string res = get_month_data(1, "yaoxuetao's raspi");
        cout << res << endl;
        delete_data(1, "yaoxuetao's raspi");
        sleep(5);
    }
    exit_database();
}