#include <iostream>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <iomanip>
#include <stack>
#include <pthread.h>
using namespace std;
auto find(int a);
auto find(unordered_map<int, string> &obj, int key)
{
	return obj.find(key);
}
auto push(int a, stack<int> &b)
{
	return b.push(a);
}
// void *del(void *args)
// {
// 	auto c =
// }
int main(void)
{
	//char graph_buffer[1024 * 1024 * 400];
	unordered_map<int, string> c;
	auto iter = c.equal_range(1);
	cout << boolalpha;
	cout
		<< (iter.first == iter.second);
	string a = "1234";
	string b = a;
	a.erase(0, 1);
	cout << a << endl;
	cout << b << endl;
	cout << (b == string("1234")) << endl;
	pair<int, string> tmp1(1, "123");
	pair<int, string> tmp2(2, "456");
	c.insert(tmp1);
	c.emplace(3, "333");
	c.insert(tmp2);
	//auto p = c.find(1);
	auto p = find(c, 1);
	if (p != c.end())
	{
		cout << c.erase(p->first) << endl;
		c.insert(tmp2);
	}
	cout << c.erase(456) << endl;
	while (1)
	{
		for (auto a = c.begin(); a != c.end(); a++)
		{
			cout << a->first << ":" << a->second << ";";
			c.erase(a->first);
		}
	}
	cout << c.count(1) << endl;
	stack<int> d;
	push(1, d);
	cout << endl;
	cout << d.top() << endl;
	cout << c.size() << endl;
}