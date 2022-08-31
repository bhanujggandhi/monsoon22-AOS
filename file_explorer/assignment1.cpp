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
struct terminalConfig
{
    int maxRows;
    int maxCols;
    struct termios orig_termios;
    int cur_x, cur_y;
    int file_idx;
    int file_start;
};
struct terminalConfig E;

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

string keys = "";
vector<string> cmdkeys;

void clear_screen()
{
    // cout << "\033[H\033[2J\033[3J";
    cout << "\033[H\033[2J";
}

void clear_currline()
{
    cout << "\033[2K";
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
        *rows = ws.ws_row - 4;
        return 0;
    }
}

void move_cursor(int row, int col)
{
    cout << "\033[" << row << ";" << col << "H";
}

void change_statusbar(string s, int bottom)
{
    move_cursor(E.maxRows - bottom, 1);
    clear_currline();
    cout << "\033[1;7m" << s << "\033[0m";
}

void printoutput(const string msg, bool status)
{
    move_cursor(E.maxRows + 4, 1);
    clear_currline();
    if (status)
        cout << "\033[1;32m" << msg << "\033[0m";
    else
        cout << "\033[1;31m" << msg << "\033[0m";
}

void splitutility(string str, char del, vector<string> &pth)
{
    string temp = "";

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
}

string getfilenamesplit(string str, char del)
{
    vector<string> pth;
    splitutility(str, del, pth);

    return pth[pth.size() - 1];
}

string splittoprev(string str, char del)
{
    vector<string> pth;
    splitutility(str, del, pth);

    string finalpath = "";

    for (int i = 0; i < pth.size() - 1; i++)
    {
        finalpath = finalpath + "/" + pth[i];
    }

    return finalpath.substr(1);
}

bool checkDir(string path)
{
    struct stat sb;
    if (stat(path.c_str(), &sb) == -1)
    {
        printoutput("Directory could not opened", false);
        return false;
    }
    else
    {
        if ((S_ISDIR(sb.st_mode)))
            return true;
        else
            return false;
    }
}

void splitcommads(string str, char del)
{
    string temp = "";

    for (int i = 0; i < str.size(); i++)
    {
        if (str[i] != del)
            temp += str[i];
        else
        {
            cmdkeys.push_back(temp);
            temp = "";
        }
    }

    cmdkeys.push_back(temp);
}

string pathresolver(string path)
{
    if (path[0] == '~')
    {
        path = HOME + path.substr(1);
    }
    string resolvedpath = "";
    char buffer[PATH_MAX];
    char *res = realpath(path.c_str(), buffer);
    if (res)
    {
        string temp(buffer);
        resolvedpath = temp;
    }
    else
    {
        resolvedpath = "ERR";
    }

    return resolvedpath;
}

void init()
{
    E.cur_x = 1;
    E.cur_y = 1;
    E.file_idx = 0;
    E.file_start = 0;
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
    int end = min(sz, E.maxRows);

    for (int i = E.file_start; i < end + E.file_start; i++)
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
    change_statusbar("--Normal Mode--  " + cwd, -2);
    // change_statusbar(cwd, 1);
    move_cursor(E.cur_x, 1);
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
    getWindowSize(&E.maxRows, &E.maxCols);
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

bool change_dir(const string path)
{
    string p = pathresolver(path);
    if (chdir(p.c_str()))
    {
        return false;
    }
    init();
    getcurrdir();
    getAllFiles(cwd);
    return true;
}

void make_dir(const string path, string foldername)
{

    foldername = path + "/" + foldername;

    if (!mkdir(foldername.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
    {
        getAllFiles(cwd);
        printoutput("Folder created successfully", true);
    }
    else
        printoutput("Failed to create dir", false);
}

//------------------ File Utilities ------------------------
void create_file(string path, string filename)
{

    filename = path + "/" + filename;

    if (creat(filename.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH) == -1)
        printoutput("Failed to create the file", false);
    else
    {
        getAllFiles(cwd);
        printoutput("File created successfully", true);
    }
}

void delete_file(string path)
{
    path = pathresolver(path);
    cout << path;
    if (path == "ERR")
    {
        printoutput("Invalid Path", false);
    }
    else
    {
        if (!remove(path.c_str()))
        {
            getAllFiles(cwd);
            printoutput("File deleted successfully", true);
        }
        else
            printoutput("Failed to delete the file", false);
    }
}

void remove_dir(string path)
{
    DIR *dir;
    struct dirent *newDir;

    path = pathresolver(path);
    dir = opendir(path.c_str());

    if (dir == NULL)
    {
        printoutput("Error in opening source directory", false);
    }
    else
    {
        for (newDir = readdir(dir); newDir != NULL; newDir = readdir(dir))
        {
            string filename(newDir->d_name);
            if (filename != "." and filename != "..")
            {
                string destfolderpath = path + "/" + filename;

                if (checkDir(destfolderpath))
                {
                    remove_dir(destfolderpath);
                    rmdir(destfolderpath.c_str());
                }
                else
                {
                    delete_file(destfolderpath);
                }
            }
        }
        rmdir(path.c_str());
    }
    closedir(dir);
}

void copy_file(const string source, const string destination)
{
    int s = open(source.c_str(), O_RDONLY);
    int d;
    char buf[BUFSIZ];
    if (s == -1)
    {
        printoutput("Source file cannot be opened", false);
        close(s);
        return;
    }
    d = open(destination.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (d == -1)
    {
        printoutput("Destination file cannot be opened", false);
        close(d);
        close(s);
        return;
    }

    size_t size;
    while ((size = read(s, buf, BUFSIZ)) > 0)
    {
        write(d, buf, size);
    }
    close(d);
    close(s);
}

void copy_dir(const string source, const string destination)
{
    if (!mkdir(destination.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
    {
        DIR *dir;
        struct dirent *newDir;

        dir = opendir(source.c_str());
        if (dir == NULL)
        {
            printoutput("Error in opening source directory", false);
        }
        else
        {
            for (newDir = readdir(dir); newDir != NULL; newDir = readdir(dir))
            {
                string filename(newDir->d_name);
                if (filename != "." and filename != "..")
                {
                    string srcfilepath = source + "/" + filename;
                    string destfilepath = destination + "/" + filename;

                    if (checkDir(srcfilepath))
                    {
                        copy_dir(srcfilepath, destfilepath);
                    }
                    else
                    {
                        copy_file(srcfilepath, destfilepath);
                    }
                }
            }
        }
        closedir(dir);
    }
    else
    {
        printoutput("Error in creating new directory", false);
    }
}

bool rename_file(const string path, const string newpath)
{
    return !(rename(path.c_str(), newpath.c_str()));
}

void move_dir(const string source, const string destination)
{
    if (!mkdir(destination.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
    {
        DIR *dir;
        struct dirent *newDir;

        dir = opendir(source.c_str());
        if (dir == NULL)
        {
            printoutput("Error in opening source directory", false);
        }
        else
        {
            for (newDir = readdir(dir); newDir != NULL; newDir = readdir(dir))
            {
                string filename(newDir->d_name);
                if (filename != "." and filename != "..")
                {
                    string srcfilepath = source + "/" + filename;
                    string destfilepath = destination + "/" + filename;

                    if (checkDir(srcfilepath))
                    {
                        move_dir(srcfilepath, destfilepath);
                        rmdir(srcfilepath.c_str());
                    }
                    else
                    {
                        rename_file(srcfilepath, destfilepath);
                    }
                }
            }
        }
        rmdir(source.c_str());
        closedir(dir);
    }
    else
    {
        printoutput("Error in creating new directory", false);
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
    if (E.cur_x - 1 > 0)
    {
        --E.file_idx;
        move_cursor(--E.cur_x, 1);
    }
    else
    {
        if (E.file_start > 0)
        {
            E.file_start--;
            E.file_idx--;
            printfiles();
        }
    }
}

void downkey()
{
    int sz = filesarr.size() - E.file_start;
    if (E.cur_x + 1 <= min(E.maxRows, sz))
    {
        ++E.file_idx;
        move_cursor(++E.cur_x, 1);
    }
    else
    {
        if (E.maxRows < sz)
        {
            if (E.file_idx + 1 < filesarr.size())
            {
                ++E.file_start;
                ++E.file_idx;
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
    change_dir("..");
    getAllFiles(cwd);
}

void enter()
{
    struct filestr f = filesarr[E.file_idx];

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
    init();
    getAllFiles(cwd);
}

void exitfunc()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios);
    clear_screen();
}

// ----------------- Command Mode -----------------------

void clearcommandline()
{
    move_cursor(E.maxRows + 3, 1);
    clear_currline();
    move_cursor(E.maxRows + 4, 1);
    clear_currline();
}

void exitcommandmode()
{
    clearcommandline();
    change_statusbar("--Normal Mode--  " + cwd, -2);
    init();
    move_cursor(E.cur_x, 1);
}

void copyexec()
{
    if (cmdkeys.size() < 3)
    {
        printoutput("Insufficient number of arguments", false);
        return;
    }

    for (int i = 1; i < cmdkeys.size() - 1; i++)
    {
        string sourcepath = cmdkeys[i];
        string filename = getfilenamesplit(sourcepath, '/');
        // cout << filename << endl;

        string destination = cmdkeys[cmdkeys.size() - 1];
        destination = pathresolver(destination);
        destination = destination + "/" + filename;

        sourcepath = pathresolver(sourcepath);

        if (sourcepath == "ERR")
        {
            printoutput("Invalid source path", false);
            return;
        }

        if (checkDir(sourcepath))
        {
            copy_dir(sourcepath, destination);
            getAllFiles(cwd);
            printoutput("Directory copied successfully", true);
        }
        else
        {
            copy_file(sourcepath, destination);
            getAllFiles(cwd);
            printoutput("File copied successfully", true);
        }
    }
}

void moveexec()
{
    if (cmdkeys.size() < 3)
    {
        printoutput("Insufficient number of arguments", false);
        return;
    }

    for (int i = 1; i < cmdkeys.size() - 1; i++)
    {
        string sourcepath = cmdkeys[i];
        string filename = getfilenamesplit(sourcepath, '/');
        // cout << filename << endl;

        string destination = cmdkeys[cmdkeys.size() - 1];
        destination = pathresolver(destination);
        destination = destination + "/" + filename;

        sourcepath = pathresolver(sourcepath);

        if (sourcepath == "ERR")
        {
            printoutput("Invalid source path", false);
            return;
        }

        if (checkDir(sourcepath))
        {
            move_dir(sourcepath, destination);
            getAllFiles(cwd);
            printoutput("Directory moved successfully", true);
        }
        else
        {
            if (rename_file(sourcepath, destination))
            {
                getAllFiles(cwd);
                printoutput("File moved successfully", true);
            }
            else
                printoutput("File couldn't moved", false);
        }
    }
}

void renameexec()
{
    if (cmdkeys.size() != 3)
    {
        printoutput("Inapproriate use of rename command", false);
    }
    string filename = cmdkeys[1];
    string newname = cmdkeys[2];
    string pth = pathresolver(filename);
    string origpath = splittoprev(pth, '/');
    filename = getfilenamesplit(filename, '/');
    newname = getfilenamesplit(newname, '/');
    newname = origpath + "/" + newname;

    if (rename_file(pth, newname))
    {
        getAllFiles(cwd);
        printoutput("Rename operation sucessful", true);
    }
    else
        printoutput("Rename operation failed", false);
}

void create_fileexec()
{
    if (cmdkeys.size() != 3)
    {
        printoutput("Invalid usage of create_file command", false);
        return;
    }

    string filename = cmdkeys[1];
    string destination = cmdkeys[2];

    destination = pathresolver(destination);

    if (destination == "ERR")
    {
        printoutput("Invalid destination path", false);
    }
    else
    {
        create_file(destination, filename);
        // cout << destination << "   " << filename << endl;
    }
}

void create_direxec()
{
    if (cmdkeys.size() != 3)
    {
        printoutput("Invalid usage of create_file command", false);
        return;
    }

    string foldername = cmdkeys[1];
    string destination = cmdkeys[2];

    destination = pathresolver(destination);

    if (destination == "ERR")
    {
        printoutput("Invalid destination path", false);
    }
    else
    {
        make_dir(destination, foldername);
        // cout << destination << "   " << filename << endl;
    }
}

void delete_fileexec()
{
    if (cmdkeys.size() != 2)
    {
        printoutput("Invalid usage of delete_file command", false);
    }

    delete_file(cmdkeys[1]);
}

void delete_direxec()
{
    if (cmdkeys.size() != 2)
    {
        printoutput("Insufficient number of arguments", false);
        return;
    }

    string path = cmdkeys[1];
    path = pathresolver(path);
    if (path == "ERR")
    {
        printoutput("Invalid path", false);
    }
    else
    {
        remove_dir(path);
        getAllFiles(cwd);
        printoutput("Directory deleted successfully", true);
    }
}

int searchexec(string source)
{
    if (cmdkeys.size() != 2)
    {
        printoutput("Inapproriate use of search command", false);
        return -1;
    }

    string searchkey = cmdkeys[1];
    // cout << searchkey << endl;

    DIR *dir;
    struct dirent *newDir;
    // cout << source << endl;
    dir = opendir(source.c_str());
    if (dir == NULL)
    {
        printoutput("Error in opening directory", false);
        return -1;
    }
    else
    {
        for (newDir = readdir(dir); newDir != NULL; newDir = readdir(dir))
        {
            string filename(newDir->d_name);
            if (filename != "." and filename != "..")
            {
                if (filename == searchkey)
                {
                    return 1;
                    closedir(dir);
                }
                else
                {
                    string srcfilepath = source + "/" + filename;

                    if (checkDir(srcfilepath))
                    {
                        if (searchexec(srcfilepath) == 1)
                        {
                            return 1;
                            closedir(dir);
                        }
                    }
                }
            }
        }
        closedir(dir);
        return 0;
    }
}

void commandexec()
{
    cmdkeys.clear();
    splitcommads(keys, ' ');
    keys = "";

    if (cmdkeys.empty())
        return;

    string task = cmdkeys[0];

    if (task == "copy")
    {
        copyexec();
    }
    else if (task == "move")
    {
        moveexec();
    }
    else if (task == "rename")
    {
        renameexec();
    }
    else if (task == "create_file")
    {
        create_fileexec();
    }
    else if (task == "create_dir")
    {
        create_direxec();
    }
    else if (task == "delete_file")
    {
        delete_fileexec();
    }
    else if (task == "delete_dir")
    {
        // delete_direxec();
    }
    else if (task == "goto")
    {
        backstk.push(cwd);
        if (change_dir(cmdkeys[1]))
        {
            printoutput("Directory changed successfully", true);
        }
        else
        {
            backstk.pop();
            printoutput("Invalid directory path", false);
        }
    }
    else if (task == "search")
    {
        int output = searchexec(cwd);
        if (output == 1)
            printoutput("True", true);
        else if (output == 0)
            printoutput("False", false);
        else
        {
        }
    }
    else if (task == "quit")
    {
        exitfunc();
        exit(0);
        return;
    }
    else
    {
        printoutput("Invalid Command!", false);
    }

    move_cursor(E.maxRows + 3, 1);
    keys = "";
}

void commandmode()
{
    change_statusbar("--Command Mode--  " + cwd, -2);
    move_cursor(E.maxRows + 3, 1);

    int col = 1;
    char ch;
    while (true)
    {
        ch = cin.get();
        int t = ch;
        if (t == 27)
        {
            exitcommandmode();
            break;
        }

        switch (ch)
        {
        case 13:
            clearcommandline();
            commandexec();
            col = 1;
            break;
        case 127:
            break;
        default:
            keys.push_back(ch);
            break;
        }

        if (ch != 13)
        {
            if (ch != 127)
            {
                cout << ch;
                move_cursor(E.maxRows + 3, ++col);
            }
            else
            {
                clear_currline();
                if (keys.length() <= 1)
                    keys = "";
                else
                    keys.pop_back();
                move_cursor(E.maxRows + 3, 1);
                cout << keys;
                col = keys.size();
                move_cursor(E.maxRows + 3, ++col);
            }
        }
    }
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
        case 58:
            commandmode();
            break;
        default:
            // cout << t << "   " << ch << "   ";
            break;
        }
    }

    atexit(&exitfunc);

    return 0;
}