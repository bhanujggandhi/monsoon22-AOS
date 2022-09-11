// To inclde stdlibs and STL
#include <bits/stdc++.h>

// Terminal utility
#include <sys/ioctl.h>
#include <termios.h>

// opendir, readdir
#include <sys/dir.h>

// Create file
#include <fcntl.h>

// To get file/folder information
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>

// Formatting
#include <signal.h>
#include <time.h>
#include <iomanip>

using namespace std;

// ------------- Data Structures---------------
struct terminalConfig {
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
struct filestr {
    string permission;
    string user;
    string group;
    string size;
    string name;
    string path;
    string lastmodified;
};

vector<filestr> filesarr;
string CWD;
string HOME;
stack<string> backstk;
stack<string> forwardstk;

string keys = "";
vector<string> cmdkeys;
bool mode = false;

// ----------------- Path Utilities -----------------
void splitutility(string str, char del, vector<string> &pth) {
    string temp = "";

    for (int i = 0; i < str.size(); i++) {
        if (str[i] != del)
            temp += str[i];
        else {
            pth.push_back(temp);
            temp = "";
        }
    }

    pth.push_back(temp);
}

string getfilenamesplit(string str, char del) {
    vector<string> pth;
    splitutility(str, del, pth);

    return pth[pth.size() - 1];
}

string splittoprev(string str, char del) {
    vector<string> pth;
    splitutility(str, del, pth);

    string finalpath = "";

    for (int i = 0; i < pth.size() - 1; i++) {
        finalpath = finalpath + "/" + pth[i];
    }

    return finalpath.substr(1);
}

void splitcommads(string str, char del) {
    string temp = "";

    for (int i = 0; i < str.size(); i++) {
        if (str[i] != del) {
            if (str[i] == '\\') {
                temp += str[i + 1];
                i += 2;
            }
            temp += str[i];
        } else {
            cmdkeys.push_back(temp);
            temp = "";
        }
    }

    cmdkeys.push_back(temp);
}

string getCurrDirFromPath(string str, char del) {
    vector<string> pth;
    splitutility(str, del, pth);

    return pth[pth.size() - 2];
}

string pathresolver(string path) {
    if (path[0] == '~') {
        path = HOME + path.substr(1);
    }
    string resolvedpath = "";
    char buffer[PATH_MAX];
    char *res = realpath(path.c_str(), buffer);
    if (res) {
        string temp(buffer);
        resolvedpath = temp;
    } else {
        resolvedpath = "ERR";
    }

    return resolvedpath;
}

// -------- Terminal Utilities --------------

void init() {
    E.cur_x = 1;
    E.cur_y = 1;
    E.file_idx = 0;
    E.file_start = 0;
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return -1;
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row - 4;
        return 0;
    }
}

void clear_screen() {
    cout << "\033[H\033[2J\033[3J";
    // cout << "\033[H\033[2J";
}

void clear_currline() { cout << "\033[2K"; }

void move_cursor(int row, int col) {
    cout << "\033[" << row << ";" << col << "H";
}

void change_statusbar(string mode, string s, int bottom) {
    move_cursor(E.maxRows - bottom, 1);
    clear_currline();
    if ((s.size() + mode.size()) > E.maxCols) {
        cout << "\033[1;7m" << mode + getCurrDirFromPath(s, '/') << "\033[0m";
    } else {
        cout << "\033[1;7m" << mode + s << "\033[0m";
    }
}

void printoutput(const string msg, bool status) {
    move_cursor(E.maxRows + 4, 1);
    clear_currline();
    if (status)
        cout << "\033[1;92m" << msg << "\033[0m";
    else
        cout << "\033[1;91m" << msg << "\033[0m";
}

// ----------------- Files Utility -------------------

bool files_sort(filestr const &lhs, filestr const &rhs) {
    return lhs.name < rhs.name;
}

string getPermissions(struct stat &_sb) {
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

void printfiles() {
    int cfac = (E.maxCols - 6) / 6;
    int sw = 6;
    int nw = (E.maxCols - 6) / 3;

    char f = ' ';
    string directorytext = "\033[1;96m";
    string exectext = "\033[1;92m";
    string normaltext = "\033[0m";

    clear_screen();

    int sz = filesarr.size();
    int end = min(sz, E.maxRows);

    for (int i = E.file_start; i < end + E.file_start; i++) {
        if (filesarr[i].permission.size() > cfac) {
            cout << left << setw(cfac) << setfill(f)
                 << filesarr[i].permission.substr(0, cfac - 4) + "..";
        } else {
            cout << left << setw(cfac) << setfill(f) << filesarr[i].permission;
        }
        if (filesarr[i].user.size() > cfac) {
            cout << left << setw(cfac) << setfill(f)
                 << filesarr[i].user.substr(0, cfac - 4) + "..";
        } else {
            cout << left << setw(cfac) << setfill(f) << filesarr[i].user;
        }
        if (filesarr[i].group.size() > E.maxCols / 6) {
            cout << left << setw(cfac) << setfill(f)
                 << filesarr[i].group.substr(0, cfac - 4) + "..";
        } else {
            cout << left << setw(cfac) << setfill(f) << filesarr[i].group;
        }
        cout << left << setw(sw) << setfill(f) << filesarr[i].size;
        if (filesarr[i].lastmodified.size() > cfac) {
            cout << left << setw(cfac) << setfill(f)
                 << filesarr[i].lastmodified.substr(0, cfac - 4) + "..";
        } else {
            cout << left << setw(cfac) << setfill(f)
                 << filesarr[i].lastmodified;
        }
        if (filesarr[i].name.size() > nw) {
            if (filesarr[i].permission[0] == 'd')
                cout << left << setw(nw) << setfill(f)
                     << directorytext + filesarr[i].name.substr(0, nw - 4) +
                            ".." + normaltext;
            else if (filesarr[i].permission[3] == 'x')
                cout << left << setw(nw) << setfill(f)
                     << exectext + filesarr[i].name.substr(0, nw - 4) + ".." +
                            normaltext;
            else
                cout << left << setw(nw) << setfill(f)
                     << filesarr[i].name.substr(0, nw - 4) + "..";
        } else {
            if (filesarr[i].permission[0] == 'd')
                cout << left << setw(nw) << setfill(f)
                     << directorytext + filesarr[i].name + normaltext;
            else if (filesarr[i].permission[3] == 'x')
                cout << left << setw(nw) << setfill(f)
                     << exectext + filesarr[i].name + normaltext;
            else
                cout << left << setw(nw) << setfill(f) << filesarr[i].name;
        }
        cout << "\r\n";
    }

    // E.cx = 1;
    // E.cy = 0;
    change_statusbar("--Normal Mode--  ", CWD, -2);
    // change_statusbar(cwd, 1);
    move_cursor(E.cur_x, 1);
}

string formatSize(long long size) {
    string output = "";
    if (1024 * 1024 * 1024 < size) {
        output += to_string((size / (1024 * 1024 * 1024)));
        output += "G";
    } else if (1024 * 1024 < size) {
        output += to_string((size / (1024 * 1024)));
        output += "M";
    } else if (1024 < size) {
        output += to_string((size / 1024));
        output += "K";
    } else if (1024 > size) {
        output += to_string(size);
        output += "B";
    }
    return output;
}

void getAllFiles(string path) {
    getWindowSize(&E.maxRows, &E.maxCols);
    // clear_screen();
    DIR *curr_dir;
    filesarr.clear();
    curr_dir = opendir(path.c_str());

    if (curr_dir == NULL) {
        string err(strerror(errno));
        printoutput(err, false);
        return;
    } else {
        for (dir = readdir(curr_dir); dir != NULL; dir = readdir(curr_dir)) {
            struct filestr currfile;
            string curr_path = dir->d_name;
            curr_path = path + curr_path;

            if (stat(curr_path.c_str(), &sb)) {
                string err(strerror(errno));
                printoutput(err, false);
                return;
            } else {

                currfile.permission = getPermissions(sb);
                passwd *u = getpwuid(sb.st_uid);
                if (u)
                    currfile.user = u->pw_name;
                else
                    currfile.user = "----";
                group *g = getgrgid(sb.st_gid);
                if (g)
                    currfile.group = g->gr_name;
                else
                    currfile.group = "----";

                currfile.size = formatSize(sb.st_size);
                currfile.name = dir->d_name;
                currfile.path = curr_path;
                char t[50] = "";
                strftime(t, 50, "%b %d %H:%M %Y", localtime(&sb.st_mtime));
                string ti(t);
                currfile.lastmodified = ti;

                filesarr.push_back(currfile);
            }
        }

        if (filesarr.size() > 0) {
            sort(filesarr.begin(), filesarr.end(), &files_sort);
            printfiles();
        }
    }
    closedir(curr_dir);
}

void create_file(string path, string filename) {

    struct stat buf;
    filename = path + "/" + filename;

    if (stat(filename.c_str(), &buf)) {
        if (creat(filename.c_str(),
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH) == -1)
            printoutput("Failed to create the file", false);
        else {
            getAllFiles(CWD);
            change_statusbar("--Command Mode--  ", CWD, -2);
            printoutput("File created successfully", true);
        }
    } else {
        printoutput("File already exists", true);
    }
}

void delete_file(string path) {
    path = pathresolver(path);
    cout << path;
    if (path == "ERR") {
        printoutput("Invalid Path", false);
    } else {
        if (!remove(path.c_str())) {
            getAllFiles(CWD);
            change_statusbar("--Command Mode--  ", CWD, -2);
            printoutput("File deleted successfully", true);
        } else
            printoutput("Failed to delete the file", false);
    }
}

void copy_file(const string source, const string destination) {
    int s = open(source.c_str(), O_RDONLY);
    int d;
    char buf[BUFSIZ];
    if (s == -1) {
        printoutput("Source file cannot be opened", false);
        close(s);
        return;
    }
    d = open(destination.c_str(), O_WRONLY | O_CREAT,
             S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if (d == -1) {
        printoutput("Destination file cannot be opened", false);
        close(d);
        close(s);
        return;
    }
    size_t size;
    while ((size = read(s, buf, BUFSIZ)) > 0) {
        write(d, buf, size);
    }

    struct stat st;
    stat(source.c_str(), &st);
    chmod(destination.c_str(), st.st_mode);
    chown(destination.c_str(), st.st_uid, st.st_gid);

    close(d);
    close(s);
}

bool rename_file(const string path, const string newpath) {
    return !(rename(path.c_str(), newpath.c_str()));
}

//-------------- Directory Utility ------------------------

bool checkDir(string path) {
    struct stat sb;
    if (stat(path.c_str(), &sb) == -1) {
        // string err(strerror(errno));
        // printoutput(err, false);
        return false;
    } else {
        if ((S_ISDIR(sb.st_mode)))
            return true;
        else
            return false;
    }
}

void getcurrdir() {
    char buf[100];
    CWD = getcwd(buf, 100);
    if (CWD != "/") CWD += "/";
}

void getHomeDir() {
    const char *h;
    if ((h = getenv("HOME")) == NULL) {
        h = getpwuid(getuid())->pw_dir;
    }

    HOME = h;
}

bool change_dir(const string path) {
    string p = pathresolver(path);
    if (chdir(p.c_str())) {
        return false;
    }
    init();
    getcurrdir();
    getAllFiles(CWD);
    return true;
}

void make_dir(const string path, string foldername) {
    struct stat buf;
    foldername = path + "/" + foldername;

    if (stat(foldername.c_str(), &buf)) {
        if (!mkdir(foldername.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
            getAllFiles(CWD);
            change_statusbar("--Command Mode--  ", CWD, -2);
            printoutput("Directory created successfully", true);
        } else
            printoutput("Failed to create dir", false);
    } else {
        printoutput("Directory already exists", true);
    }
}

void remove_dir(string path) {
    DIR *dir;
    struct dirent *newDir;

    path = pathresolver(path);
    dir = opendir(path.c_str());

    if (dir == NULL) {
        string err(strerror(errno));
        printoutput(err, false);
    } else {
        for (newDir = readdir(dir); newDir != NULL; newDir = readdir(dir)) {
            string filename(newDir->d_name);
            if (filename != "." and filename != "..") {
                string destfolderpath = path + "/" + filename;

                if (checkDir(destfolderpath)) {
                    remove_dir(destfolderpath);
                    rmdir(destfolderpath.c_str());
                } else {
                    delete_file(destfolderpath);
                }
            }
        }
        rmdir(path.c_str());
    }
    closedir(dir);
}

void copy_dir(const string source, const string destination) {
    if (!mkdir(destination.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
        DIR *dir;
        struct dirent *newDir;

        dir = opendir(source.c_str());
        if (dir == NULL) {
            string err(strerror(errno));
            printoutput(err, false);
        } else {
            for (newDir = readdir(dir); newDir != NULL; newDir = readdir(dir)) {
                string filename(newDir->d_name);
                if (filename != "." and filename != "..") {
                    string srcfilepath = source + "/" + filename;
                    string destfilepath = destination + "/" + filename;

                    if (checkDir(srcfilepath)) {
                        copy_dir(srcfilepath, destfilepath);
                    } else {
                        copy_file(srcfilepath, destfilepath);
                    }
                }
            }
        }
        struct stat st;
        stat(source.c_str(), &st);
        chmod(destination.c_str(), st.st_mode);
        chown(destination.c_str(), st.st_uid, st.st_gid);
        closedir(dir);
    } else {
        printoutput("Error in creating new directory", false);
    }
}

void move_dir(const string source, const string destination) {
    if (!mkdir(destination.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
        DIR *dir;
        struct dirent *newDir;

        dir = opendir(source.c_str());
        if (dir == NULL) {
            string err(strerror(errno));
            printoutput(err, false);
        } else {
            for (newDir = readdir(dir); newDir != NULL; newDir = readdir(dir)) {
                string filename(newDir->d_name);
                if (filename != "." and filename != "..") {
                    string srcfilepath = source + "/" + filename;
                    string destfilepath = destination + "/" + filename;

                    if (checkDir(srcfilepath)) {
                        move_dir(srcfilepath, destfilepath);
                        rmdir(srcfilepath.c_str());
                    } else {
                        rename_file(srcfilepath, destfilepath);
                    }
                }
            }
        }
        struct stat st;
        stat(source.c_str(), &st);
        chmod(destination.c_str(), st.st_mode);
        chown(destination.c_str(), st.st_uid, st.st_gid);
        rmdir(source.c_str());
        closedir(dir);
    } else {
        printoutput("Error in creating new directory", false);
    }
}

// ------------------ Clean up -------------

void resizehandler(int t) {
    init();
    getAllFiles(CWD);
    if (mode) {
        change_statusbar("--Command Mode--  ", CWD, -2);
        move_cursor(E.maxRows + 3, 1);
    } else {
        move_cursor(E.cur_x, E.cur_y);
    }
}

void exitfunc() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios);
    clear_screen();
}

// -------------------Normal Mode----------------
void enableNormalMode() {

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

void upkey() {
    if (E.cur_x - 1 > 0) {
        --E.file_idx;
        move_cursor(--E.cur_x, 1);
    } else {
        if (E.file_start > 0) {
            E.file_start--;
            E.file_idx--;
            printfiles();
        }
    }
}

void downkey() {
    int sz = filesarr.size() - E.file_start;
    if (E.cur_x + 1 <= min(E.maxRows, sz)) {
        ++E.file_idx;
        move_cursor(++E.cur_x, 1);
    } else {
        if (E.maxRows < sz) {
            if (E.file_idx + 1 < filesarr.size()) {
                ++E.file_start;
                ++E.file_idx;
                printfiles();
            } else {
                return;
            }
        }
    }
}

void goto_parent_dir() {
    if (backstk.empty() or
        (!backstk.empty() and (backstk.top() != CWD or CWD != "/")))
        backstk.push(CWD);

    change_dir("..");
    getAllFiles(CWD);
}

void enter() {
    struct filestr f = filesarr[E.file_idx];

    if (f.permission[0] == 'd') {
        if (f.path == ".") {
            return;
        } else if (f.path == "..") {
            backstk.push(CWD);
            goto_parent_dir();
        } else {
            backstk.push(CWD);
            change_dir(f.path);
            getAllFiles(CWD);
        }
    } else {
        int pid = fork();
        if (pid == 0) {
            if (execl("/usr/bin/xdg-open", "xdg-open 2>/dev/null",
                      f.path.c_str(), (char *)0) == -1) {
                return;
            }
            exit(1);
        }
    }
    getAllFiles(CWD);
}

void goback() {
    if (backstk.empty())
        return;
    else {
        forwardstk.push(CWD);
        change_dir(backstk.top());
        backstk.pop();
        getAllFiles(CWD);
    }
}

void goforward() {
    if (forwardstk.empty())
        return;
    else {
        if (backstk.empty() or
            (!backstk.empty() and (backstk.top() != CWD or CWD != "/")))
            backstk.push(CWD);
        change_dir(forwardstk.top());
        forwardstk.pop();
        getAllFiles(CWD);
    }
}

void goHome() {
    backstk.push(CWD);
    change_dir(HOME);
    getAllFiles(CWD);
}

// ----------------- Command Mode -----------------------

void clearcommandline() {
    move_cursor(E.maxRows + 3, 1);
    clear_currline();
    move_cursor(E.maxRows + 4, 1);
    clear_currline();
}

void exitcommandmode() {
    clearcommandline();
    mode = false;
    change_statusbar("--Normal Mode--  ", CWD, -2);
    init();
    move_cursor(E.cur_x, 1);
}

void copyexec() {
    if (cmdkeys.size() < 3) {
        printoutput("Insufficient number of arguments", false);
        return;
    }

    for (int i = 1; i < cmdkeys.size() - 1; i++) {
        string sourcepath = cmdkeys[i];
        string filename = getfilenamesplit(sourcepath, '/');
        // cout << filename << endl;

        string destination = cmdkeys[cmdkeys.size() - 1];
        destination = pathresolver(destination);
        destination = destination + "/" + filename;

        sourcepath = pathresolver(sourcepath);

        if (sourcepath == "ERR") {
            printoutput("Invalid source path", false);
            return;
        }

        if (checkDir(sourcepath)) {
            copy_dir(sourcepath, destination);
            getAllFiles(CWD);
            change_statusbar("--Command Mode--  ", CWD, -2);
            printoutput("Directory copied successfully", true);
        } else {
            copy_file(sourcepath, destination);
            getAllFiles(CWD);
            change_statusbar("--Command Mode--  ", CWD, -2);
            printoutput("File copied successfully", true);
        }
    }
}

void moveexec() {
    if (cmdkeys.size() < 3) {
        printoutput("Insufficient number of arguments", false);
        return;
    }

    for (int i = 1; i < cmdkeys.size() - 1; i++) {
        string sourcepath = cmdkeys[i];
        string filename = getfilenamesplit(sourcepath, '/');
        // cout << filename << endl;

        string destination = cmdkeys[cmdkeys.size() - 1];
        destination = pathresolver(destination);
        destination = destination + "/" + filename;

        sourcepath = pathresolver(sourcepath);

        if (sourcepath == "ERR") {
            printoutput("Invalid source path", false);
            return;
        }

        if (checkDir(sourcepath)) {
            move_dir(sourcepath, destination);
            getAllFiles(CWD);
            change_statusbar("--Command Mode--  ", CWD, -2);
            printoutput("Directory moved successfully", true);
        } else {
            if (rename_file(sourcepath, destination)) {
                getAllFiles(CWD);
                change_statusbar("--Command Mode--  ", CWD, -2);
                printoutput("File moved successfully", true);
            } else
                printoutput("File couldn't moved", false);
        }
    }
}

void renameexec() {
    if (cmdkeys.size() != 3) {
        printoutput("Inapproriate use of rename command", false);
    }
    string filename = cmdkeys[1];
    string newname = cmdkeys[2];
    string pth = pathresolver(filename);
    string origpath = splittoprev(pth, '/');
    filename = getfilenamesplit(filename, '/');
    newname = getfilenamesplit(newname, '/');
    newname = origpath + "/" + newname;

    if (rename_file(pth, newname)) {
        getAllFiles(CWD);
        change_statusbar("--Command Mode--  ", CWD, -2);
        printoutput("Rename operation sucessful", true);
    } else
        printoutput("Rename operation failed", false);
}

void create_fileexec() {
    if (cmdkeys.size() != 3) {
        printoutput("Invalid usage of create_file command", false);
        return;
    }

    string filename = cmdkeys[1];
    string destination = cmdkeys[2];

    destination = pathresolver(destination);

    if (destination == "ERR") {
        printoutput("Invalid destination path", false);
    } else {
        create_file(destination, filename);
        // cout << destination << "   " << filename << endl;
    }
}

void create_direxec() {
    if (cmdkeys.size() != 3) {
        printoutput("Invalid usage of create_file command", false);
        return;
    }

    string foldername = cmdkeys[1];
    string destination = cmdkeys[2];

    destination = pathresolver(destination);

    if (destination == "ERR") {
        printoutput("Invalid destination path", false);
    } else {
        make_dir(destination, foldername);
        // cout << destination << "   " << filename << endl;
    }
}

void delete_fileexec() {
    if (cmdkeys.size() != 2) {
        printoutput("Invalid usage of delete_file command", false);
    }

    delete_file(cmdkeys[1]);
}

void delete_direxec() {
    if (cmdkeys.size() != 2) {
        printoutput("Insufficient number of arguments", false);
        return;
    }

    string path = cmdkeys[1];
    path = pathresolver(path);
    if (path == "ERR") {
        printoutput("Invalid path", false);
    } else {
        remove_dir(path);
        getAllFiles(CWD);
        change_statusbar("--Command Mode--  ", CWD, -2);
        printoutput("Directory deleted successfully", true);
    }
}

int searchexec(string source) {
    if (cmdkeys.size() != 2) {
        printoutput("Inapproriate use of search command", false);
        return -1;
    }

    string searchkey = cmdkeys[1];
    // cout << searchkey << endl;

    DIR *dir;
    struct dirent *newDir;
    // cout << source << endl;
    dir = opendir(source.c_str());
    if (dir == NULL) {
        // printoutput("Error in opening directory", false);
        return -1;
    } else {
        for (newDir = readdir(dir); newDir != NULL; newDir = readdir(dir)) {
            string filename(newDir->d_name);
            if (filename != "." and filename != "..") {
                if (filename == searchkey) {
                    closedir(dir);
                    return 1;
                } else {
                    string srcfilepath = source + "/" + filename;

                    if (checkDir(srcfilepath)) {
                        if (searchexec(srcfilepath) == 1) {
                            closedir(dir);
                            return 1;
                        }
                    }
                }
            }
        }
        closedir(dir);
        return 0;
    }
}

void commandexec() {
    cmdkeys.clear();
    splitcommads(keys, ' ');
    keys = "";

    if (cmdkeys.empty()) return;

    string task = cmdkeys[0];

    if (task == "copy") {
        copyexec();
    } else if (task == "move") {
        moveexec();
    } else if (task == "rename") {
        renameexec();
    } else if (task == "create_file") {
        create_fileexec();
    } else if (task == "create_dir") {
        create_direxec();
    } else if (task == "delete_file") {
        delete_fileexec();
    } else if (task == "delete_dir") {
        delete_direxec();
    } else if (task == "goto") {
        if (backstk.empty() or
            (!backstk.empty() and (backstk.top() != CWD or CWD != "/")))
            backstk.push(CWD);
        if (change_dir(cmdkeys[1])) {
            printoutput("Directory changed successfully", true);
            change_statusbar("--Command Mode--  ", CWD, -2);
        } else {
            backstk.pop();
            printoutput("Invalid directory path", false);
        }
    } else if (task == "search") {
        int output = searchexec(CWD);
        if (output == 1)
            printoutput("True", true);
        else if (output == 0)
            printoutput("False", false);
        else {
        }
    } else if (task == "quit") {
        exitfunc();
        exit(0);
        return;
    } else {
        printoutput("Invalid Command!", false);
    }

    move_cursor(E.maxRows + 3, 1);
    keys = "";
}

void commandmode() {
    change_statusbar("--Command Mode--  ", CWD, -2);
    move_cursor(E.maxRows + 3, 1);
    mode = true;

    int col = 1;
    char ch;
    while (true) {
        ch = cin.get();
        int t = ch;
        if (t == 27) {
            exitcommandmode();
            break;
        }

        switch (ch) {
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

        if (ch != 13) {
            if (ch != 127) {
                cout << ch;
                move_cursor(E.maxRows + 3, ++col);
            } else {
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

// --------------- Driver Code -------------

int main() {
    init();
    getHomeDir();
    getcurrdir();
    getAllFiles(CWD);
    signal(SIGWINCH, resizehandler);

    enableNormalMode();
    char ch;

    while (true) {
        ch = cin.get();
        if (ch == 'q' or ch == 'Q') break;
        int t = ch;

        switch (t) {
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
                mode = true;
                commandmode();
                break;
            default:
                break;
        }
    }

    atexit(&exitfunc);

    return 0;
}