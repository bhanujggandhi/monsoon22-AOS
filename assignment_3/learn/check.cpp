#include <bits/stdc++.h>

using namespace std;

int main() {
    unordered_set<string> st;

    st.insert("bgroup");
    st.insert("igroup");
    st.insert("agroup");
    st.insert("vgroup");
    st.insert("fgroup");
    st.insert("cgroup");

    string greqParse = "";
    int i = 1;
    for (auto uid : st) {
        greqParse += to_string(i) + ". " + uid + "\n";
        i++;
    }

    cout << greqParse;
    return 0;
}