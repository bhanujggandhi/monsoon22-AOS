#include <bits/stdc++.h>

using namespace std;

int main() {
    unordered_set<string> st;

    st.insert("bgroup");
    st.insert("igroup");

    // for (auto x : st) {
    //     cout << x << endl;
    // }

    st.erase("bgroup");
    for (auto x : st) {
        cout << x << endl;
    }
    return 0;
}