#include<iostream>
#include<functional>

using namespace std;

int main() {
	cout <<std::hash<uint64_t>()(12345) << endl;
	return 0;
}