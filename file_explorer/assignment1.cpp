#include <bits/stdc++.h>

// Terminal utility
#include <termios.h>
#include <sys/ioctl.h>

// opendir, readdir
#include <sys/dir.h>

// Create file
#include <fcntl.h>

// To get file/folder information
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

// Formatting
#include <iomanip>
#include <time.h>

using namespace std;

// ------------- Data Structures---------------
struct editorConfig
{
    int screenrows;
    int screencols;
    struct termios orig_termios;
    int cx, cy;
};
struct editorConfig E;

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
string cwd;

void clear_screen()
{
    cout << "\033[2J\033[H";
}

int getWindowSize(int *rows, int *cols)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
        return -1;
    }
    else
    {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

void move_cursor(int row, int col)
{
    cout << "\033[" << row << ";" << col << "H";
}

void change_statusbar(string s)
{
    move_cursor(E.screenrows - 3, 1);
    cout << "\033[K";
    cout << s;
}

string split(string str, char del)
{
    string temp = "";
    vector<string> pth;

    for (int i = 0; i < str.size(); i++)
    {
        if (str[i] != del)
            temp += str[i];
        else
        {
            pth.push_back(temp);
            temp = "";
        }
    }

    pth.push_back(temp);

    string finalpth = "";
    for (int i = 0; i < pth.size() - 2; i++)
    {
        finalpth = finalpth + pth[i] + "/";
    }

    return finalpth;
}

//-------------- Directory Utility Functions ------------------------
void getcurrdir()
{
    char buf[100];
    cwd = getcwd(buf, 100);
    cwd += "/";
}

bool files_sort(filestr const &lhs, filestr const &rhs) { return lhs.name < rhs.name; }

void printfiles()
{
    int pw = 13;
    int ugw = 18;
    int lmw = 20;
    int sw = 8;
    char f = ' ';

    int sz = filesarr.size();
    int end = min(sz, E.screenrows);

    for (int i = 0; i < end; i++)
    {
        cout << left << setw(pw) << setfill(f) << filesarr[i].permission;
        cout << left << setw(ugw) << setfill(f) << filesarr[i].user;
        cout << left << setw(ugw) << setfill(f) << filesarr[i].group;
        cout << left << setw(sw) << setfill(f) << filesarr[i].size;
        cout << left << setw(lmw) << setfill(f) << filesarr[i].lastmodified;
        cout << left << setw(pw) << setfill(f) << filesarr[i].name;
        cout << "\r\n";
    }

    E.cx = 1;
    E.cy = 0;
    change_statusbar("Normal Mode");
    move_cursor(E.cx, 1);
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

void getAllFiles(string path)
{
    getWindowSize(&E.screenrows, &E.screencols);
    E.cx = 1;
    E.cy = 1;
    clear_screen();
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
            else
            {

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
        }

        sort(filesarr.begin(), filesarr.end(), &files_sort);
        printfiles();
    }
    closedir(curr_dir);
}

void change_dir(const string path)
{
    if (chdir(path.c_str()))
    {
        cout << "Unable to change the Directory" << endl;
        return;
    }
    getcurrdir();
    getAllFiles(cwd);
}

void make_dir(string &path)
{
    string foldername;
    cin >> foldername;

    foldername = path + foldername;

    if (!mkdir(foldername.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
    {
        cout << "Folder created successfully" << endl;
        getAllFiles(path);
    }
    else
        cout << "Failed to create dir" << endl;
}

void remove_dir(string &path)
{
    string foldername;
    cin >> foldername;

    foldername = path + foldername;

    cout << "Deleting: " << foldername << endl;
    int n;
    cin >> n;

    if (!rmdir(foldername.c_str()))
    {
        cout << "Folder deleted successfully" << endl;
        getAllFiles(path);
    }
    else
        cout << "Failed to delete dir" << endl;
}

//------------------ File Utilities ------------------------
void create_file(string &path)
{
    string filename;
    cin >> filename;

    filename = path + filename;

    if (creat(filename.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH) == -1)
        cout << "Failed to create the file" << endl;
    else
    {
        cout << "File created" << endl;
        getAllFiles(path);
    }
}

void delete_file(string &path)
{
    cout << "Deleting: " << path << endl;
    int n;
    cin >> n;

    if (!remove(path.c_str()))
    {
        cout << "File deleted successfully" << endl;
        getAllFiles(path);
    }
    else
        cout << "Failed to delete the file" << endl;
}

void copy_file(const string source, const string destination)
{
    int s = open(source.c_str(), O_RDONLY);
    int d;
    char buf;
    if (s == -1)
    {
        cout << "Source file cannot be opened" << endl;
        close(s);
        return;
    }
    d = open(destination.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (d == -1)
    {
        cout << "Destination file cannot be opened" << endl;
        close(d);
        close(s);
        return;
    }

    while (read(s, &buf, 1))
    {
        write(d, &buf, 1);
    }

    cout << "File copied successfully" << endl;
    close(d);
    close(s);
}

void rename_file(const string path, const string newpath)
{
    if (rename(path.c_str(), newpath.c_str()) == 0)
    {
        cout << "File renamed successfully" << endl;
    }
    else
    {
        cout << "Rename File: Operation failed" << endl;
    }
}

// -------------------Normal Mode----------------
void enableNormalMode()
{

    tcgetattr(STDIN_FILENO, &E.orig_termios);
    // On pressing :
    // atexit(disableRawMode);

    struct termios raw = E.orig_termios;

    // IXON for ctrl S or ctrl q
    // ICRNL for ctrl m
    raw.c_iflag = raw.c_iflag & ~(IXON | ICRNL);

    // To disable enter as terminal default
    raw.c_oflag &= ~(OPOST);

    // ISIG for ctrl c or ctrl z
    // IEXTEN for ctrl v
    raw.c_lflag = raw.c_lflag & ~(ECHO | ICANON | ISIG | IEXTEN);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void upkey()
{
    if (E.cx - 1 > 0)
        move_cursor(--E.cx, 1);
}

void downkey()
{
    int sz = filesarr.size();
    if (E.cx + 1 <= min(E.screenrows, sz))
        move_cursor(++E.cx, 1);
}

void enter()
{
    struct filestr f = filesarr[E.cx - 1];

    if (f.permission[0] == 'd')
    {
        if (f.path.size() == 1 and f.path[0] == '.')
        {
            return;
        }
        else if (f.path.size() == 2 and f.path == "..")
        {
            change_dir(split(f.path, '/'));
            getAllFiles(cwd);
        }
        else
        {
            change_dir(f.path + "/");
            getAllFiles(cwd);
        }
    }
    else
    {
        cout << "opening";
    }
}

void exitfunc()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios);
    clear_screen();
}

int main()
{
    getcurrdir();
    getAllFiles(cwd);

    enableNormalMode();
    char ch;

    while (true)
    {
        ch = cin.get();
        if (ch == 'q')
            break;
        int t = ch;

        switch (t)
        {
        case 65:
            upkey();
            break;
        case 66:
            downkey();
            break;
        case 13:
            enter();
            break;
        default:
            // cout << ch << "  " << t;
            break;
        }
    }

    // change_dir("../../");

    // string src = "./hello.txt";
    // string dest = "../hello1/hellocop.txt";
    // string p = "../hello1/";
    // delete_file(p);
    // copy_file(src, dest);

    // change_dir(p);

    // string newpath;
    // cin >> newpath;
    // change_dir(newpath);

    // make_dir(path);
    // remove_dir(path);
    // delete_file(path);

    atexit(clear_screen);

    return 0;
}