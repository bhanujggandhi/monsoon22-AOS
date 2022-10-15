#include <bits/stdc++.h>

using namespace std;

int main() {

    char resolvedpath[_POSIX_PATH_MAX];
    if (realpath("../peer-to-peer/tracker_info.txt", resolvedpath) == NULL) {
        printf("ERROR: bad path %s\n", resolvedpath);
        return 1;
    }

    std::ifstream file(resolvedpath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            printf("%s\n", line.c_str());
        }
        file.close();
    }

    return 0;
}