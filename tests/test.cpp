#include<iostream>
#include<type_traits>

using namespace std;

int main() {
	std::aligned_storage<sizeof(int), alignof(int)>::type item;
	new (&item) int(1);
	int* get = static_cast<int*>(static_cast<void*>(&item));
	cout << *get << endl;
	return 0;
}