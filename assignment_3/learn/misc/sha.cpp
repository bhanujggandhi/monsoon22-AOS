// C++ Standard Library
#include <bits/stdc++.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

// OpenSSL Library
#include <openssl/sha.h>

using namespace std;

string generateSHA(string filepath, long offset) {
    char resolvedpath[_POSIX_PATH_MAX];
    if (realpath(filepath.c_str(), resolvedpath) == NULL) {
        printf("ERROR: bad path %s\n", resolvedpath);
        return NULL;
    }

    FILE *fd = fopen(resolvedpath, "rb");
    char shabuf[SHA_DIGEST_LENGTH];
    bzero(shabuf, sizeof(shabuf));
    fseek(fd, offset, SEEK_SET);
    fread(shabuf, 1, SHA_DIGEST_LENGTH, fd);

    unsigned char SHA_Buffer[SHA_DIGEST_LENGTH];
    char buffer[SHA_DIGEST_LENGTH * 2];
    int i;
    bzero(buffer, sizeof(buffer));
    bzero(SHA_Buffer, sizeof(SHA_Buffer));
    SHA1((unsigned char *)shabuf, 20, SHA_Buffer);

    for (i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf((char *)&(buffer[i * 2]), "%02x", SHA_Buffer[i]);
    }

    fclose(fd);
    string shastr(buffer);
    return shastr;
}

int main(int argc, char *argv[]) {
    cout << generateSHA("./readbyline.cpp", 0) << endl;
    return 0;
}