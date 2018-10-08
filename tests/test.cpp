#include <iostream>
#include "../booty/utils/Strings.hpp"

using namespace std;

int main() {
    string s = "sdf23\nad12\nad2\n\n\nasd\n";
    cout << "start" << endl;
    auto res = booty::Split(s);
    for (auto each : res) {
        cout << each << ' ';
    }
    cout << "\nfinish" << endl;
    return 0;
}