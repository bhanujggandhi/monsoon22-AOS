#include <bits/stdc++.h>

using namespace std;

bool cmp(pair<int, vector<string>> &p1, pair<int, vector<string>> &p2) {
    return p1.second.size() < p2.second.size();
}

int main() {

    vector<pair<int, vector<string>>> pieceselec;

    pieceselec.push_back({1, {"1", "2", "3"}});
    pieceselec.push_back({2, {"1"}});
    pieceselec.push_back({3, {"1", "2"}});
    pieceselec.push_back({4, {"3"}});
    pieceselec.push_back({5, {"2", "3"}});
    pieceselec.push_back({6, {"1", "3"}});
    pieceselec.push_back({7,
                          {
                              "1",
                          }});
    pieceselec.push_back({8,
                          {
                              "1",
                          }});
    pieceselec.push_back({9, {"1", "2", "3"}});

    sort(pieceselec.begin(), pieceselec.end(), cmp);

    for (auto x : pieceselec) {
        cout << x.first << "      " << x.second.size() << endl;
    }

    return 0;
}