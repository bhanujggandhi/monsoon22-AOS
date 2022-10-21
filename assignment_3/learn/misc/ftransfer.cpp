#include <bits/stdc++.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define CHUNKSIZE 524288

using namespace std;

long getfilesize(string filename) {
    struct stat sbuf;
    int f = stat(filename.c_str(), &sbuf);
    return f == 0 ? sbuf.st_size : -1;
}

int main() {

    char resolvedpath[_POSIX_PATH_MAX];
    if (realpath("../../temp.pdf", resolvedpath) == NULL) {
        printf("ERROR: bad path %s\n", resolvedpath);
        return 1;
    }

    int sfd = open(resolvedpath, O_RDONLY);
    long fsize = getfilesize(resolvedpath);
    int chunks = ceil((double)fsize / CHUNKSIZE);

    int dfd = open(
        "/mnt/LINUXDATA/bhanujggandhi/Learning/iiit/sem1/aos/assignment_3/"
        "learn/misc/copied.pdf",
        O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    int fz = fsize;
    for (int i = 0; i < chunks; i++) {
        off64_t offset = i * CHUNKSIZE;
        size_t n = 0;
        char buff[BUFSIZ];
        memset(buff, 0, BUFSIZ);
        int file_size = CHUNKSIZE;

        while ((n = pread64(sfd, buff, BUFSIZ, offset)) > 0 and file_size > 0) {
            pwrite64(dfd, buff, n, offset);
            offset += n;
            memset(buff, 0, BUFSIZ);
            file_size -= n;
            fz -= n;
        }
        cout << "Chunk " << i << endl;
    }
    // size_t n;
    // char buf[BUFSIZ];
    // while ((n = read(sfd, buf, BUFSIZ)) > 0) {
    //     write(dfd, buf, n);
    // }
    close(sfd);
    close(dfd);
    return 0;
}