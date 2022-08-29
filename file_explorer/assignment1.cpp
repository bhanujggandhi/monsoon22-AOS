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
#include <signal.h>

using namespace std;

// ------------- Data Structures---------------
struct editorConfig
{
    int screenrows;
    int screencols;
    struct termios orig_termios;
    int cx, cy;
    int rowoffset;
    int globalind;
    int start;
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
string HOME;

stack<string> backstk;
stack<string> forwardstk;

void clear_screen()
{
    cout << "\033[H\033[2J\033[3J";
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
        *rows = ws.ws_row - 2;
        return 0;
    }
}

void move_cursor(int row, int col)
{
    cout << "\033[" << row << ";" << col << "H";
}

void change_statusbar(string s, int bottom)
{
    move_cursor(E.screenrows - bottom, 1);
    cout << "\033[K";
    cout << "\033[1;7m" << s << "\033[0m";
}

string splittoprev(string str, char del)
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

void init()
{
    E.cx = 1;
    E.cy = 1;
    E.globalind = 0;
    E.start = 0;
}

//-------------- Directory Utility Functions ------------------------
void getcurrdir()
{
    char buf[100];
    cwd = getcwd(buf, 100);
    cwd += "/";
}

void getHomeDir()
{
    const char *h;
    if ((h = getenv("HOME")) == NULL)
    {
        h = getpwuid(getuid())->pw_dir;
    }

    HOME = h;
    cout << HOME;
}

bool files_sort(filestr const &lhs, filestr const &rhs) { return lhs.name < rhs.name; }

void printfiles()
{
    int pw = 13;
    int ugw = 18;
    int lmw = 20;
    int sw = 8;
    int nw = 13;
    char f = ' ';
    string directorytext = "\033[1;34m";
    string exectext = "\033[1;32m";
    string normaltext = "\033[0m";

    clear_screen();

    int sz = filesarr.size();
    int end = min(sz, E.screenrows);

    for (int i = E.start; i < end + E.start; i++)
    {
        cout << left << setw(pw) << setfill(f) << filesarr[i].permission;
        cout << left << setw(ugw) << setfill(f) << filesarr[i].user;
        cout << left << setw(ugw) << setfill(f) << filesarr[i].group;
        cout << left << setw(sw) << setfill(f) << filesarr[i].size;
        cout << left << setw(lmw) << setfill(f) << filesarr[i].lastmodified;
        if (filesarr[i].permission[0] == 'd')
            cout << left << setfill(f) << directorytext << filesarr[i].name << normaltext;
        else if (filesarr[i].permission[3] == 'x')
            cout << left << setfill(f) << exectext << filesarr[i].name << normaltext;
        else
            cout << left << setfill(f) << filesarr[i].name;
        cout << "\r\n";
    }

    // E.cx = 1;
    // E.cy = 0;
    change_statusbar("--Normal Mode--  " + cwd, -1);
    // change_statusbar(cwd, 1);
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
    // clear_screen();
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
    init();
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
    // raw.c_oflag &= ~(OPOST);

    // ISIG for ctrl c or ctrl z
    // IEXTEN for ctrl v
    raw.c_lflag = raw.c_lflag & ~(ECHO | ICANON | ISIG | IEXTEN);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void upkey()
{
    if (E.cx - 1 > 0)
    {
        --E.globalind;
        move_cursor(--E.cx, 1);
    }
    else
    {
        if (E.start > 0)
        {
            E.start--;
            E.globalind--;
            printfiles();
        }
    }
}

void downkey()
{
    int sz = filesarr.size() - E.start;
    if (E.cx + 1 <= min(E.screenrows, sz))
    {
        ++E.globalind;
        move_cursor(++E.cx, 1);
    }
    else
    {
        if (E.screenrows < sz)
        {
            if (E.globalind + 1 < filesarr.size())
            {
                ++E.start;
                ++E.globalind;
                printfiles();
            }
            else
            {
                return;
            }
        }
    }
}

void goto_parent_dir()
{
    backstk.push(cwd);
    change_dir(splittoprev(cwd, '/'));
    getAllFiles(cwd);
}

void enter()
{
    struct filestr f = filesarr[E.globalind];

    if (f.permission[0] == 'd')
    {
        if (f.path == ".")
        {
            return;
        }
        else if (f.path == "..")
        {
            backstk.push(cwd);
            goto_parent_dir();
        }
        else
        {
            backstk.push(cwd);
            change_dir(f.path);
            getAllFiles(cwd);
        }
    }
    else
    {
        int pid = fork();
        if (pid == 0)
        {
            execl("/usr/bin/xdg-open", "xdg-open", f.path.c_str(), (char *)0);
            exit(1);
        }
        getAllFiles(cwd);
    }
}

void goback()
{
    if (backstk.empty())
        return;
    else
    {
        forwardstk.push(cwd);
        change_dir(backstk.top());
        backstk.pop();
        getAllFiles(cwd);
    }
}

void goforward()
{
    if (forwardstk.empty())
        return;
    else
    {
        backstk.push(cwd);
        change_dir(forwardstk.top());
        forwardstk.pop();
        getAllFiles(cwd);
    }
}

void goHome()
{
    backstk.push(cwd);
    change_dir(HOME);
    getAllFiles(cwd);
}

void resizehandler(int t)
{
    getAllFiles(cwd);
}

void exitfunc()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios);
    clear_screen();
}

int main()
{
    init();
    getHomeDir();
    getcurrdir();
    getAllFiles(cwd);
    signal(SIGWINCH, resizehandler);

    enableNormalMode();
    char ch;

    while (true)
    {
        ch = cin.get();
        if (ch == 'q' or ch == 'Q')
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
        case 127:
            goto_parent_dir();
            break;
        case 67:
            goforward();
            break;
        case 68:
            goback();
            break;
        case 104 | 72:
            goHome();
            break;
        default:
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

    atexit(exitfunc);

    return 0;
}