#include<iostream>
#include"../../booty/detail/SignalTrival.hpp"

using namespace std;
using namespace booty;

void TestSignalTrival() {
	::detail::SignalTrival<void()> s1;
	s1.connect([] {cout << "Hello Booty!\n"; });
	s1.connect([] {cout << "Hello Booty Again!\n"; });
	s1.call();
	cout << "==============\n";
	::detail::SignalTrival<void(int)> s2;
	s2.connect([](int x) {cout << "Hello arg x: " << x << endl; }, 1);
	s2.connect([](int x) {cout << "Another arg x: " << x << endl; }, 99);
	s2.call();
	cout << "==============\n";
	::detail::SignalTrival<void(int, double, int)> s3;
	s3.connect([](int x, double y, int z) {cout << "Args are: " << x << ' ' << y << ' ' << z << endl; }
	, 999, 1.123456, 321);
	s3.call();
}


int main() {
	cout << "Test SignalTrival()\n";
	TestSignalTrival();
	return 0;
}