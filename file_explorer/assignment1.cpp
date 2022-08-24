#include <bits/stdc++.h>
#include <fcntl.h>

using namespace std;

void create_file()
{
    int filediscriptor;
    string filename;

    cin >> filename;

    filediscriptor = creat(filename.c_str(), S_IRWXU | S_IRGRP | S_IROTH);

    if (filediscriptor == -1)
        cout << "Failed to create the file" << endl;
    else
        cout << "File created" << endl;
}

int main()
{

    return 0;
}