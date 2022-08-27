#include <bits/stdc++.h>

// opendir, readdir
#include <dirent.h>

// To get file/folder information
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

// Create file
#include <fcntl.h>

// Formatting
#include <iomanip>
#include <time.h>

using namespace std;

struct dirent *dir;
struct stat sb;
struct filestr
{
    string permission;
    string user;
    string group;
    string size;
    string name;
    string path;
    string lastmodified;
};

vector<filestr> filesarr;

// Print all the files
bool files_sort(filestr const &lhs, filestr const &rhs) { return lhs.name < rhs.name; }

void printfiles()
{
    int pw = 13;
    int ugw = 18;
    int lmw = 20;
    int sw = 8;
    char f = ' ';
    for (int i = 0; i < filesarr.size(); i++)
    {
        cout << left << setw(pw) << setfill(f) << filesarr[i].permission;
        cout << left << setw(ugw) << setfill(f) << filesarr[i].user;
        cout << left << setw(ugw) << setfill(f) << filesarr[i].group;
        cout << left << setw(sw) << setfill(f) << filesarr[i].size;
        cout << left << setw(lmw) << setfill(f) << filesarr[i].lastmodified;
        cout << left << setw(pw) << setfill(f) << filesarr[i].name;
        cout << endl;
    }
}

string getPermissions(struct stat &_sb)
{
    string permission = "";
    mode_t perm = _sb.st_mode;

    permission += (S_ISDIR(perm)) ? 'd' : '-';
    permission += (perm & S_IRUSR) ? 'r' : '-';
    permission += (perm & S_IWUSR) ? 'w' : '-';
    permission += (perm & S_IXUSR) ? 'x' : '-';
    permission += (perm & S_IRGRP) ? 'r' : '-';
    permission += (perm & S_IWGRP) ? 'w' : '-';
    permission += (perm & S_IXGRP) ? 'x' : '-';
    permission += (perm & S_IROTH) ? 'r' : '-';
    permission += (perm & S_IWOTH) ? 'w' : '-';
    permission += (perm & S_IXOTH) ? 'x' : '-';

    return permission;
}

void getAllFiles(string &path)
{
    DIR *curr_dir;
    filesarr.clear();
    curr_dir = opendir(path.c_str());

    if (curr_dir == NULL)
    {
        cout << "Directory could not be opened" << endl;
    }
    else
    {
        for (dir = readdir(curr_dir); dir != NULL; dir = readdir(curr_dir))
        {
            struct filestr currfile;
            string curr_path = dir->d_name;
            curr_path = path + curr_path;

            if (stat(curr_path.c_str(), &sb))
                cout << "Permission Denied" << endl;

            currfile.permission = getPermissions(sb);
            currfile.user = getpwuid(sb.st_uid)->pw_name;
            currfile.group = getgrgid(sb.st_gid)->gr_name;
            currfile.size = (sb.st_size >= 1024 ? to_string(sb.st_size / 1024) + "K" : to_string(sb.st_size) + "B");
            currfile.name = dir->d_name;
            currfile.path = curr_path;
            char t[50] = "";
            strftime(t, 50, "%b %d %H:%M %Y", localtime(&sb.st_mtime));
            string ti(t);
            currfile.lastmodified = ti;

            filesarr.push_back(currfile);
        }

        sort(filesarr.begin(), filesarr.end(), &files_sort);
        printfiles();
    }
}

// Functional Utilities
void create_file(string &path)
{
    string filename;

    cin >> filename;

    filename = path + filename;

    if (creat(filename.c_str(), S_IRWXU | S_IRGRP | S_IROTH) == -1)
        cout << "Failed to create the file" << endl;
    else
        cout << "File created" << endl;
}

int main()
{
    string path = "/mnt/LINUXDATA/bhanujggandhi/Learning/bashlearn/";
    getAllFiles(path);

    return 0;
}