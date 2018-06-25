#include<chrono>
#include<iostream>

using namespace std;
using namespace std::chrono;

void TestSystemClock() {
	auto now = system_clock::now();
	cout << "Time since epoch:" << now.time_since_epoch().count() << endl;
}

int main() {
	TestSystemClock();
	return 0;
}