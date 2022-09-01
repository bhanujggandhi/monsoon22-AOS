

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <string.h>
#include <iostream>
#include <cstdlib>
#include <time.h>
#include <stdarg.h>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <bits/stdc++.h>
#include <unistd.h>

/*** Data ***/
using namespace std;
#define TAB_SPACE 8
#define T_B 1099511627776
#define G_B 1073741824
#define M_B 1048576
#define K_B 1024
string command;
int curr_pos = 0;
string home_dir;
char cwd[PATH_MAX];
bool absolute(string);
bool mode = false;      // start with normal mode
bool last_move = false; // false no move, non < and non > move previously true means previous move was < or >
int current_pos_in;
typedef struct erow
{
    int size;
    int rsize;
    char *chars;
    char *render;
} erow;
struct abuf
{
    // append buffer
    char *b;
    int len;
};
vector<string> history;
struct dirent *dir_entry;
struct curr_dir_info
{
    string filename;
    string filePermission;
    string fileSize;
    string filesUserID;
    string filesGroupId;
    string filelastmodified;
};
vector<curr_dir_info *> fils_info;
struct config
{
    int cx;
    void s_cx(int x) { cx = x; }
    int g_cx() { return cx; }
    erow *row;
    int cy;
    void s_cy(int x) { cy = x; }
    int g_cy() { return cy; }
    char *filename;
    int coloff;
    void s_coloff(int x) { coloff = x; }
    int g_coloff() { return coloff; }
    char statusmsg[80];
    int rowoff;
    void s_rowoff(int x) { rowoff = x; }
    int g_rowoff() { return rowoff; }
    int screenrows;
    void s_screenrows(int x) { screenrows = x; }
    int g_screenrows() { return screenrows; }
    struct termios orig_termios;
    int rx;
    void s_rx(int x) { rx = x; }
    int g_rx() { return rx; }
    int screencols;
    void s_screencols(int x) { screencols = x; }
    int g_screencols() { return screencols; }
    int numrows;
    void s_numrows(int x) { numrows = x; }
    int g_numrows() { return numrows; }
};
struct config E;
/*** All function Declaration ***/
void reset_cx();
void reset_cy();
void reset_row_count();
void reset_row();
void reset_rowoff();
void reset_coloff();
void reset_rx();
void reset_fils_info();
void reset_filename();
void reset_statusmsg();
void adjust_screenrows();
void refreshScreen();
void appendRow(char *, size_t);
void updateRow(erow *);
void draw_rows(struct abuf *);
void setStatusMessage(const char *, ...);
void drawMessageBar(struct abuf *);
void abFree(struct abuf *);
void abAppend(struct abuf *, const char *, int);
void processKeyPress();
void move_cursor(int);
void scroller();
void directoryOpen(string);
void commandMode(int);
void process_string(string);
void die(const char *);
void backspaceDir(int);
void enterDir(int);
void move_home(int);
void move_right(int);
void move_left(int);
void goto_dir(string);
void move_file(string, string);
void delete_file_dir_move(string);
void copy_helper_move(string, string);
void delete_file_dir(string);
void copy_helper(string, string);
void enableRawMode();
void disableRawMode();
void window_resize(int);
void init();
int is_dir(string);
int readKey();
int rowCxToRx(erow *, int);
int getWindowSize(int *, int *);
int main_search(string);
int main_rename(string, string);
int main_move_file(string, string);
int main_move_dir(string, string);
int move_dir(string, string);
int copy_files_move(string, string, string);
int create_dir_move(string, struct stat);
int main_delete_files_folder(string);
int main_delelte_file(string);
int main_create_file(string, string);
int create_dir_create_file(string);
int main_create_dir(string, string);
int main_copy_directory(string, string);
int copy_files(string, string, string);
int create_dir(string, struct stat);
int main_copy_files(string, string);
int create_dir_copy_files(string, struct stat);
string sanitise_input(string);
string get_details(char *);
string fix_Destination(string, string);
bool absolute(string);
bool search(string, string);
/*** init ***/
void reset_cx() { E.cx = 0; }
void reset_cy() { E.cy = 0; }
void reset_row_count() { E.numrows = 0; }
void reset_row()
{
    E.row = NULL;
    ;
}
void reset_rowoff() { E.rowoff = 0; }
void reset_coloff() { E.coloff = 0; }
void reset_rx() { E.rx = 0; }
void reset_fils_info() { fils_info.clear(); }
void reset_filename() { E.filename = NULL; }
void reset_statusmsg() { E.statusmsg[0] = '\0'; }
void adjust_screenrows() { E.screenrows -= 1; }
void init()
{
    reset_cx();
    reset_cy();
    reset_row_count();
    reset_row();
    reset_rowoff();
    reset_coloff();
    reset_rx();
    reset_fils_info();
    reset_filename();
    reset_statusmsg();
    if (getWindowSize(&E.screenrows, &E.screencols) == -1)
        die("getWindowSize");
    adjust_screenrows();
}
/*** Window Resize ***/
void window_resize(int signal)
{
    init();
    const char *ab = ".";
    directoryOpen(ab);
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        // resize terminal should not alter history & home_dir
    }
    else
    {
        die("Unable to get cwd");
    }
    refreshScreen();
}
/*** Disable canonical mode ***/
void disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
        die("tcsetattr");
}
/*** Enable canonical mode ***/
void enableRawMode()
{
    signal(SIGWINCH, window_resize);
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
        die("tcgetattr");
    struct termios raw = E.orig_termios;
    raw.c_iflag = ((raw.c_iflag) & (~(BRKINT | ICRNL | INPCK | ISTRIP | IXON)));
    raw.c_oflag = ((raw.c_oflag) & (~(OPOST)));
    raw.c_cflag = ((raw.c_cflag) | (CS8));
    raw.c_lflag = ((raw.c_lflag) & ~(ECHO | ICANON | IEXTEN | ISIG));
    atexit(disableRawMode);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");
}
/*** Copy Files ***/
int create_dir_copy_files(string input, struct stat sb)
{
    struct stat info;
    int i = 0;
    if (input[input.length() - 1] != '/')
        input += '/';
    string temp = "";
    while (i < input.length())
    {
        for (; input[i] != '/' && i < input.length(); i++)
        {
            temp += input[i];
        }
        if (mkdir(temp.c_str(), 0777) != 0)
        {
            ;
        }
        temp += '/';
        i++;
    }
    return 1;
}
int main_copy_files(string source_file, string dest_directory)
{
    char buf;
    struct stat sb;
    struct passwd *pwd;
    struct group *grp;
    string first_name = source_file;
    int i = source_file.length() - 1; // last character
    string lastword;                  // last word is the file name
    while (i != 0 && source_file[i] != '/')
    {
        --i;
    }
    if (i == 0)
        lastword = source_file; // entire name is file name only, i.e. the path does not has directory name
    else
    {
        lastword = first_name.substr(i + 1); // +1 to skip leading space or '/' i guess
    }
    string dest_file;
    if (dest_directory[dest_directory.length() - 1] != '/')
        dest_file = dest_directory + '/' + lastword;
    else if (dest_directory[dest_directory.length() - 1] == '/')
        dest_file = dest_directory + lastword;
    if (stat(first_name.c_str(), &sb) == -1)
    {
        exit(EXIT_FAILURE);
    }
    if (dest_directory != "")
    {
        create_dir_copy_files(dest_directory, sb);
    }
    int fd_one, fd_two;
    fd_one = open(first_name.c_str(), O_RDONLY);
    if (fd_one == -1)
    {
        printf("Error opening first_file\n");
        close(fd_one);
        return 0;
    }
    fd_two = open(dest_file.c_str(), O_WRONLY | O_CREAT, sb.st_mode);
    while (read(fd_one, &buf, 1))
    {
        write(fd_two, &buf, 1);
    }
    if (fchown(fd_two, sb.st_uid, sb.st_gid) == -1)
    {
        cout << "chown fail\n";
    }
    close(fd_one);
    close(fd_two);
    return 0;
}
/*** Copy Directories ***/
int create_dir(string input, struct stat sb)
{
    struct stat info;
    int i = 0;
    if (input[input.length() - 1] != '/')
        input += '/';
    string temp = "";
    while (i < input.length())
    {
        for (; input[i] != '/' && i < input.length(); i++)
        {
            temp += input[i];
        }
        if (mkdir(temp.c_str(), 0777) != 0)
        {
            ;
        }
        temp += '/';
        i++;
    }
    return 1;
}
int copy_files(string source, string destination, string file_name)
{
    char cwd[PATH_MAX];
    char buf;
    struct stat sb;
    struct passwd *pwd;
    struct group *grp;
    string source_file = source + '/' + file_name;
    string destination_file = destination + '/' + file_name;
    if (stat(source_file.c_str(), &sb) == -1)
    {
        exit(EXIT_FAILURE);
    }
    if (destination != "")
    {
        create_dir(destination, sb);
    }
    int fd_one, fd_two;
    fd_one = open(source_file.c_str(), O_RDONLY);
    if (fd_one == -1)
    {
        close(fd_one);
        return 0;
    }
    fd_two = open(destination_file.c_str(), O_WRONLY | O_CREAT, sb.st_mode);
    while (read(fd_one, &buf, 1))
    {
        write(fd_two, &buf, 1);
    }
    if (fchown(fd_two, sb.st_uid, sb.st_gid) == -1)
    {
        ;
    }
    close(fd_one);
    close(fd_two);
    return 0;
}
void copy_helper(string first_name, string second_name)
{
    char cwd[PATH_MAX];
    char cwd2[PATH_MAX];
    struct stat sb;
    struct stat inner;
    struct dirent *de;
    DIR *dr;
    if (stat(first_name.c_str(), &sb) < 0)
    { // unable to get stats of source directory or file
        return;
    }
    if ((S_ISDIR(sb.st_mode)))
    {
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            ;
        }
        dr = opendir(first_name.c_str()); // opened source directory
        if (dr == NULL)
        {
            return;
        }
        if (chdir(first_name.c_str()) < 0)
        {
            cout << "chdir issue";
            return;
        } // moved to source directory
        while ((de = readdir(dr)) != NULL)
        {
            if (strcmp(".", (de->d_name)) == 0 || strcmp("..", (de->d_name)) == 0)
            {
                create_dir(second_name, sb);
                continue;
            }
            if (stat(de->d_name, &inner) == 0)
            { // reading details about one of file/folder in present sirectory successful
                if (S_ISDIR(inner.st_mode))
                { // so we have picked a folder from current directory
                    string copy_this = de->d_name;
                    string new_source = first_name + '/' + copy_this;
                    string new_destination = second_name + '/' + copy_this;
                    copy_helper(new_source, new_destination);
                }
                else
                {
                    string file_name = de->d_name;
                    copy_files(first_name, second_name, file_name);
                }
            }
            else
            {
                ;
            }
        }
        chdir("..");
        closedir(dr);
    }
    else
    {
        copy_files(first_name, second_name, "noname"); // we got a file as input this should not have happened
        return;
    }
}
int main_copy_directory(string source, string destination)
{
    char cwd[PATH_MAX]; // give path starting with '/' but does not ends with '/'
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
    }
    string CWD = cwd;
    string real_source;
    string real_destination;
    if (absolute(source) == false && source[0] != '/')
    {
        real_source = CWD + '/' + source;
    }
    else if (absolute(source) == false && source[0] == '/')
    {
        real_source = CWD + source;
    }
    else if (absolute(source) == true)
    {
        real_source = source;
    }
    if (absolute(destination) == false && destination[0] != '/')
    {
        real_destination = CWD + '/' + destination;
    }
    else if (absolute(destination) == false && destination[0] == '/')
    {
        real_destination = CWD + destination;
    }
    else if (absolute(destination) == true)
    {
        real_destination = destination;
    }
    real_destination = fix_Destination(source, real_destination);
    copy_helper(real_source, real_destination);
    if (chdir(CWD.c_str()) < 0)
    {
        cout << "chdir issue";
    }
    return 0;
}
/*** Create Directory ***/
int main_create_dir(string dir_name, string dir_loc)
{
    struct stat info;
    string create;
    if (dir_loc[dir_loc.length() - 1] != '/')
    {
        create = dir_loc + '/' + dir_name + '/';
    }
    else if (dir_loc[dir_loc.length() - 1] == '/')
    {
        create = dir_loc + dir_name + '/';
    }
    int i = 0;
    string temp;
    while (i < create.length())
    {
        for (; create[i] != '/' && i < create.length(); i++)
        {
            temp += create[i];
        }
        if (temp != "" && mkdir(temp.c_str(), 0777) != 0)
        {
            // Not created as it already exists or some error
        }
        temp += '/';
        i++;
    }
    return 0;
}
/*** Create File ***/
int create_dir_create_file(string name)
{
    struct stat info;
    string input = name;
    int i = 0;
    string temp;
    while (i < input.length())
    {
        for (; input[i] != '/' && i < input.length(); i++)
        {
            temp += input[i];
        }
        if (temp != "" && mkdir(temp.c_str(), 0777) != 0)
        {
            // Not created as it already exists or some error
        }
        temp += '/';
        i++;
    }
    return 0;
}
int main_create_file(string file_name, string dir_name)
{
    struct stat info;
    string to_create;
    string create;
    if (dir_name[dir_name.length() - 1] != '/')
    {
        to_create = dir_name + '/';
        create = dir_name + '/' + file_name;
    }
    else if (dir_name[dir_name.length() - 1] == '/')
    {
        to_create = dir_name;
        create = dir_name + file_name;
    }
    create_dir_create_file(to_create);
    fstream file;
    file.open(create.c_str(), ios::out);
    if (!file)
    {
        cout << "Error in creating file!!!";
    }
    else
        cout << "File created successfully.";
    file.close();
    return 0;
}
/*** Delete Files ***/
int main_delelte_file(string first_name)
{
    int del = remove(first_name.c_str());
    if (!del)
    {
    }
    else
    {
        printf("\nthe file is not Deleted:");
        return 1;
    }
    return 0;
}

/*** Delete Directories ***/
void delete_file_dir(string first_name)
{
    char cwd[PATH_MAX];
    struct stat sb;
    struct stat inner;
    struct dirent *de;
    DIR *dr;
    if (stat(first_name.c_str(), &sb) < 0)
    { // unable to fetch fstat
        return;
    }
    if ((S_ISDIR(sb.st_mode)))
    {
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            ;
        }
        dr = opendir(first_name.c_str());
        if (dr == NULL)
        {
            return;
        }
        if (chdir(first_name.c_str()) < 0)
            return;
        while ((de = readdir(dr)) != NULL)
        {
            if (strcmp(".", (de->d_name)) == 0 || strcmp("..", (de->d_name)) == 0)
            {
                continue;
            }
            if (stat(de->d_name, &inner) == 0)
            {
                if (S_ISDIR(inner.st_mode))
                {
                    string delete_this = de->d_name;
                    delete_file_dir(delete_this);
                    remove(delete_this.c_str());
                }
                else
                {
                    int del = remove(de->d_name);
                    if (!del)
                    { // The file is Deleted successfully
                    }
                    else
                    { // the file is not Deleted
                        return;
                    }
                }
            }
            else
            {
                return;
            }
        }
        remove(first_name.c_str());
        chdir("..");
        closedir(dr);
    }
    else
    {
        remove(first_name.c_str());
        return;
    }
}
int main_delete_files_folder(string first_name)
{
    char swd[PATH_MAX];
    if (getcwd(swd, sizeof(swd)) != NULL)
    {
        // starting point
    }
    string CWD = swd;
    delete_file_dir(first_name);
    string temp;
    int i;
    for (i = CWD.length() - 1; i >= 0; i--)
    {
        if (CWD[i] == '/')
        {
            temp = CWD.substr(i + 1);
        }
    }
    if (chdir(CWD.c_str()) < 0)
    {
        cout << "chdir issue";
    }
    if (first_name != "")
        remove(first_name.c_str());
    return 0;
}

/*** Move Dir ***/
int create_dir_move(string input, struct stat sb)
{
    struct stat info;
    int i = 0;
    if (input[input.length() - 1] != '/')
        input += '/';
    string temp = "";
    while (i < input.length())
    {
        for (; input[i] != '/' && i < input.length(); i++)
        {
            temp += input[i];
        }
        if (mkdir(temp.c_str(), 0777) != 0)
        {
            ;
        }
        temp += '/';
        i++;
    }
    return 1;
}
int copy_files_move(string source, string destination, string file_name)
{
    char cwd[PATH_MAX];
    char buf;
    struct stat sb;
    struct passwd *pwd;
    struct group *grp;
    string source_file = source + '/' + file_name;
    string destination_file = destination + '/' + file_name;
    if (stat(source_file.c_str(), &sb) == -1)
    {
        exit(EXIT_FAILURE);
    }
    if (destination != "")
    {
        create_dir_move(destination, sb);
    }
    int fd_one, fd_two;
    fd_one = open(source_file.c_str(), O_RDONLY);
    if (fd_one == -1)
    {
        close(fd_one);
        return 0;
    }
    fd_two = open(destination_file.c_str(), O_WRONLY | O_CREAT, sb.st_mode);
    while (read(fd_one, &buf, 1))
    {
        write(fd_two, &buf, 1);
    }
    if (fchown(fd_two, sb.st_uid, sb.st_gid) == -1)
    {
        ;
    }
    close(fd_one);
    close(fd_two);
    return 0;
}

void copy_helper_move(string first_name, string second_name)
{
    char cwd[PATH_MAX];
    char cwd2[PATH_MAX];
    struct stat sb;
    struct stat inner;
    struct dirent *de;
    DIR *dr;
    if (stat(first_name.c_str(), &sb) < 0)
    {
        return;
    }
    if ((S_ISDIR(sb.st_mode)))
    {
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            ;
        }
        dr = opendir(first_name.c_str()); // opened source directory
        if (dr == NULL)
        {
            // unable to open
            return;
        }
        if (chdir(first_name.c_str()) < 0)
        {
            cout << "chdir issue";
            return;
        } // moved to source directory
        while ((de = readdir(dr)) != NULL)
        {
            if (strcmp(".", (de->d_name)) == 0 || strcmp("..", (de->d_name)) == 0)
            {
                create_dir_move(second_name, sb);
                continue;
            }
            if (stat(de->d_name, &inner) == 0)
            { // reading details about one of file/folder in present sirectory successful
                if (S_ISDIR(inner.st_mode))
                { // so we have picked a folder from current directory
                    string copy_this = de->d_name;
                    string new_source = first_name + '/' + copy_this;
                    string new_destination = second_name + '/' + copy_this;
                    copy_helper_move(new_source, new_destination);
                }
                else
                {
                    string file_name = de->d_name;
                    copy_files_move(first_name, second_name, file_name); ////////////////////////////
                }
            }
            else
            { // unable to get file/folder stats
                ;
            }
        }
        chdir("..");
        closedir(dr);
    }
    else
    {
        copy_files_move(first_name, second_name, "noname"); // we got a file as input this should not have happened
        return;
    }
}
void delete_file_dir_move(string first_name)
{
    char cwd[PATH_MAX];
    struct stat sb;
    struct stat inner;
    struct dirent *de;
    DIR *dr;
    if (stat(first_name.c_str(), &sb) < 0)
    {
        return;
    }
    if ((S_ISDIR(sb.st_mode)))
    {
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
        }
        dr = opendir(first_name.c_str());
        if (dr == NULL)
        {
            return;
        }
        if (chdir(first_name.c_str()) < 0)
            return;
        while ((de = readdir(dr)) != NULL)
        {
            if (strcmp(".", (de->d_name)) == 0 || strcmp("..", (de->d_name)) == 0)
            {
                continue;
            }
            if (stat(de->d_name, &inner) == 0)
            {
                if (S_ISDIR(inner.st_mode))
                {
                    string delete_this = de->d_name;
                    delete_file_dir_move(delete_this);
                    remove(delete_this.c_str());
                }
                else
                {
                    int del = remove(de->d_name);
                    if (!del)
                    {
                    }
                    else
                    {
                        return;
                    }
                }
            }
            else
            {
                return;
            }
        }
        remove(first_name.c_str());
        chdir("..");
        closedir(dr);
    }
    else
    {
        remove(first_name.c_str());
        return;
    }
}

int move_dir(string source, string destination)
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
    }
    string CWD = cwd;
    copy_helper_move(source, destination);
    if (chdir(CWD.c_str()) < 0)
        return 0;
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
    }
    delete_file_dir_move(source);
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
    }
    string temp;
    for (int i = source.length() - 1; i >= 0; i--)
    {
        if (source[i] == '/')
        {
            temp = source.substr(i + 1);
        }
    }
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
    }
    if (temp != "")
        remove(temp.c_str());
    return 0;
}
int main_move_dir(string source, string destination) // asumption this is called to copy directory only
{
    char cwd[PATH_MAX]; // give path starting with '/' but does not ends with '/'
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
    }
    string CWD = cwd;
    string real_source;
    string real_destination;
    if (absolute(source) == false && source[0] != '/')
    {
        real_source = CWD + '/' + source;
    }
    else if (absolute(source) == false && source[0] == '/')
    {
        real_source = CWD + source;
    }
    else if (absolute(source) == true)
    {
        real_source = source;
    }
    if (absolute(destination) == false && destination[0] != '/')
    {
        real_destination = CWD + '/' + destination;
    }
    else if (absolute(destination) == false && destination[0] == '/')
    {
        real_destination = CWD + destination;
    }
    else if (absolute(destination) == true)
    {
        real_destination = destination;
    }
    real_destination = fix_Destination(source, real_destination);
    copy_helper(real_source, real_destination);
    move_dir(real_source, real_destination);
    if (chdir(CWD.c_str()) < 0)
    {
        die(CWD.c_str());
    }
    return 0;
}

/*** Move Files ***/
void move_file(string source, string destination)
{ // works even if destination exists or not
    char cwd[PATH_MAX];
    char buf;
    struct stat sb;
    int i = source.length() - 1; // last character
    if (stat(source.c_str(), &sb) == -1)
    {
        ;
    }
    string file_name;
    while (i != 0 && source[i] != '/')
    {
        --i;
    }
    file_name = source.substr(i + 1);
    if (destination != "")
    {
        if (destination[destination.length() - 1] != '/')
        {
            destination += '/';
        }
        create_dir_move(destination, sb);
    }
    int fd_one, fd_two;
    fd_one = open(source.c_str(), O_RDONLY);
    if (fd_one == -1)
    {
        printf("Error opening first_file\n");
        close(fd_one);
        return;
    }
    string dest_file;
    dest_file = destination + file_name;
    fd_two = open(dest_file.c_str(), O_WRONLY | O_CREAT, sb.st_mode);
    while (read(fd_one, &buf, 1))
    {
        write(fd_two, &buf, 1);
    }
    if (fchown(fd_two, sb.st_uid, sb.st_gid) == -1)
    {
        cout << "chown fail\n";
    }
    close(fd_one);
    close(fd_two);
    remove(source.c_str());
}
int main_move_file(string source, string destination) // asumption this is called to copy directory only
{
    char cwd[PATH_MAX]; // give path starting with '/' but does not ends with '/'
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
    }
    string CWD = cwd;
    string real_source;
    string real_destination;
    if (absolute(source) == false && source[0] != '/')
    {
        real_source = CWD + '/' + source;
    }
    else if (absolute(source) == false && source[0] == '/')
    {
        real_source = CWD + source;
    }
    else if (absolute(source) == true)
    {
        real_source = source;
    }
    if (absolute(destination) == false && destination[0] != '/')
    {
        real_destination = CWD + '/' + destination;
    }
    else if (absolute(destination) == false && destination[0] == '/')
    {
        real_destination = CWD + destination;
    }
    else if (absolute(destination) == true)
    {
        real_destination = destination;
    }
    move_file(real_source, real_destination);
    return 0;
}
/*** Rename File ***/
int main_rename(string old_name, string new_name)
{
    int value = rename(old_name.c_str(), new_name.c_str());
    if (!value)
    {
        printf("%s", "File name changed successfully");
    }
    else
    {
        perror("Error");
    }
    return 0;
}

/*** Search File/Folder ***/
bool search(string first_name, string to_search)
{ // searches directory_name or file_name not directory_location or file_location
    bool flag = false;
    if (strcmp(to_search.c_str(), (first_name.c_str())) == 0)
    {
        flag = true;
        return flag;
    }
    struct stat sb;
    struct stat inner;
    struct dirent *de;
    DIR *dr;
    if (stat(first_name.c_str(), &sb) < 0)
    {
        return flag;
    }
    if ((S_ISDIR(sb.st_mode)))
    {
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
        }
        dr = opendir(first_name.c_str());
        if (dr == NULL)
        {
            return flag;
        }
        if (chdir(first_name.c_str()) < 0)
            return flag;
        while ((de = readdir(dr)) != NULL)
        {
            if (strcmp(".", (de->d_name)) == 0 || strcmp("..", (de->d_name)) == 0)
            {
                continue;
            }
            if (stat(de->d_name, &inner) == 0)
            {
                if (S_ISDIR(inner.st_mode))
                {
                    if (strcmp(to_search.c_str(), (de->d_name)) == 0)
                    {
                        flag = true;
                        return flag;
                    }
                    else
                    {
                        string find_this = de->d_name;
                        flag = search(find_this, to_search);
                    }
                }
                else
                {
                    if (strcmp(to_search.c_str(), (de->d_name)) == 0)
                    {
                        flag = true;
                        return flag;
                    }
                }
            }
            else
            {
                if (strcmp(to_search.c_str(), first_name.c_str()) == 0)
                {
                    flag = true;
                    return flag;
                }
            }
        }
        chdir("..");
        closedir(dr);
    }
    else
    {
        if (strcmp(to_search.c_str(), (first_name.c_str())) == 0)
        {
            flag = true;
            return flag;
        }
    }
    return flag;
}
int main_search(string first_name)
{
    char cwd[PATH_MAX];
    bool found = search(".", first_name);
    string result;
    if (found)
        result = "True";
    else
        result = "False";
    setStatusMessage(result.c_str());
    refreshScreen();
    unsigned int microseconds = 3000000;
    usleep(microseconds);
    return 0;
}
/*** Goto Dir ***/
void goto_dir(string dest)
{
    dest = sanitise_input(dest);
    const char *dests = dest.c_str();
    int ch = chdir(dests);
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        if ((!last_move))
        { // this goto is enter hence it is not < or >
            history.push_back(cwd);
            last_move = false;
            curr_pos = history.size();
        }
        else if ((last_move))
        {                                                             // this goto is enter hence it is not < or >
            history.erase(history.begin() + curr_pos, history.end()); // check +1 or not
            history.push_back(cwd);
            last_move = false;
            curr_pos = history.size() - 1;
        }
    }
    else
    {
        die("getcwd() error");
    }
    /*if the change of directory was successful it will print successful otherwise it will print not successful*/
    if (ch < 0)
        die("chdir change of directory unsuccessful");
    else
    {
        directoryOpen(".");
    }
}

/*** Check Absolute path or not ***/
bool absolute(string str)
{
    int j;
    if (str[0] == '~' && str[1] == '/')
        j = 2;
    else if (str[0] == '~' && str[1] != '/')
        j = 1;
    else if (str[0] == '/')
        j = 1;
    else
        j = 0;
    int i;
    for (i = j; i < str.length(); i++)
    {
        if (str[i] == '/')
        {
            i--;
            break;
        }
    }
    string keyword = str.substr(j, i);
    if (strcmp("bin", (keyword.c_str())) == 0 || strcmp("libx32", (keyword.c_str())) == 0 || strcmp("snap", (keyword.c_str())) == 0 || strcmp("boot", (keyword.c_str())) == 0 || strcmp("media", (keyword.c_str())) == 0 || strcmp("srv", (keyword.c_str())) == 0 || strcmp("dev", (keyword.c_str())) == 0 || strcmp("mnt", (keyword.c_str())) == 0 || strcmp("sys", (keyword.c_str())) == 0 || strcmp("etc", (keyword.c_str())) == 0 || strcmp("opt", (keyword.c_str())) == 0 || strcmp("tmp", (keyword.c_str())) == 0 || strcmp("home", (keyword.c_str())) == 0 || strcmp("proc", (keyword.c_str())) == 0 || strcmp("usr", (keyword.c_str())) == 0 || strcmp("lib", (keyword.c_str())) == 0 || strcmp("root", (keyword.c_str())) == 0 || strcmp("var", (keyword.c_str())) == 0 || strcmp("lib32", (keyword.c_str())) == 0 || strcmp("run", (keyword.c_str())) == 0 || strcmp("lib64", (keyword.c_str())) == 0 || strcmp("sbin", (keyword.c_str())) == 0)
        return true;
    return false;
}

/*** Preprocess Command Input ***/
string sanitise_input(string input)
{
    string loc = "";
    if (input[0] == '~')
    {
        loc = home_dir + input.substr(1);
        return loc;
    }
    else if (absolute(input))
        return input;
    else if (absolute(input) == false && input[0] == '/')
    {
        loc = input.substr(1);
        return loc;
    }
    return input;
}

/*** Is Dir ***/
int is_dir(string address)
{
    struct stat sb;
    if (stat(address.c_str(), &sb) < 0)
    {
        return -1; // error occured
    }
    else
    {
        if ((S_ISDIR(sb.st_mode)))
        {
            return 1;
        }
        else
            return 0;
    }
    return 0;
} // 1: Directory, 0:file, -1:error
/*** Correct Destination Location Copy/Move ***/
string fix_Destination(string source, string destination)
{
    if (strcmp(source.c_str(), ".") == 0)
    {
        return source;
    }
    if (strcmp(source.c_str(), "./") == 0)
    {
        return source;
    }
    int i = source.length() - 1; // last character
    if (source[i] == '/')
        i--;
    string lastword; // last word is the file name
    while (i != 0 && source[i] != '/')
    {
        --i;
    }
    if (i == 0)
        lastword = source; // entire name is file name only, i.e. the path does not has directory name
    else
    {
        lastword = source.substr(i + 1); // +1 to skip leading space or '/' i guess
    }
    if (destination[destination.length() - 1] == '/')
        destination += lastword;
    else if (destination[destination.length() - 1] != '/')
        destination = destination + '/' + lastword;
    return destination;
}
/*** Move left on <- key Press ***/
void move_left(int c)
{
    if (curr_pos <= 0)
    {
        curr_pos = 0;
    }
    else
    {
        if (mode == false)
        {
            string dest = history[--curr_pos];
            dest += "\0";
            const char *dests = dest.c_str();
            int ch = chdir(dests);
            if (getcwd(cwd, sizeof(cwd)) != NULL)
            {
                // this move is < or >
                last_move = true;
            }
            else
            {
                die("getcwd() error");
            }
            /*if the change of directory was successful it will print successful otherwise it will print not successful*/
            if (ch < 0)
                die("chdir change of directory unsuccessful");
            else
            {
                directoryOpen(".");
            }
        }
    }
    directoryOpen(".");
}
/*** Move right on -> key Press ***/
void move_right(int c)
{
    if (curr_pos >= history.size() - 1)
    {
        curr_pos = history.size() - 1;
    }
    else
    {
        if (mode == false)
        {
            string dest = history[++curr_pos];
            dest += "\0";
            const char *dests = dest.c_str();
            int ch = chdir(dests);
            if (getcwd(cwd, sizeof(cwd)) != NULL)
            {
                // this move is < or >
                last_move = true;
            }
            else
            {
                die("getcwd() error");
            }
            /*if the change of directory was successful it will print successful otherwise it will print not successful*/
            if (ch < 0)
                die("chdir change of directory unsuccessful");
            else
            {
                directoryOpen(".");
            }
        }
    }
    directoryOpen(".");
}
/*** Move home on h key Press ***/
void move_home(int c)
{
    if (mode == false)
    {
        string dest = home_dir;
        dest += "\0";
        const char *dests = dest.c_str();
        int ch = chdir(dests);
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            if ((!last_move))
            { // this move is home hence it is not < or >
                history.push_back(cwd);
                last_move = false;
                curr_pos = history.size();
            }
            else if ((last_move))
            {                                                             // this move is home hence it is not < or >
                history.erase(history.begin() + curr_pos, history.end()); // check +1 or not
                history.push_back(cwd);
                last_move = false;
                curr_pos = history.size() - 1;
            }
        }
        else
        {
            die("getcwd() error");
        }
        /*if the change of directory was successful it will print successful otherwise it will print not successful*/
        if (ch < 0)
            die("chdir change of directory unsuccessful");
        else
        {
            directoryOpen(".");
        }
    }
}
/*** enter on "Enter" key Press ***/
void enterDir(int c)
{
    if (mode == false)
    {
        string dest;
        if (E.cy == fils_info.size())
        { // Tried to Open Wrong File/Folder
            return;
        }
        dest = fils_info[E.cy]->filename; // now space seperated folder are also opening
        char file = (fils_info[E.cy]->filePermission)[0];
        if (file == 'd')
        {
            dest += "\0";
            const char *dests = dest.c_str();
            int ch = chdir(dests);
            if (getcwd(cwd, sizeof(cwd)) != NULL)
            {
                if ((!last_move))
                {
                    // this move is enter hence it is not < or >
                    history.push_back(cwd);
                    last_move = false;
                    curr_pos = history.size();
                }
                else if ((last_move))
                {
                    // this move is enter hence it is not < or >
                    history.erase(history.begin() + curr_pos, history.end()); // check +1 or not
                    history.push_back(cwd);
                    last_move = false;
                    curr_pos = history.size() - 1;
                }
            }
            else
            {
                die("getcwd() error");
            }
            /*if the change of directory was successful it will print successful otherwise it will print not successful*/
            if (ch < 0)
                die("chdir change of directory unsuccessful");
            else
            {
                directoryOpen(".");
            }
        }
        if (file == '-')
        {
            int pid = fork();
            if (pid == 0) // if child process
            {
                execl("/usr/bin/xdg-open", "xdg-open", dest.c_str(), (char *)0);
                exit(0);
            }
        }
    }
}

/*** Move back on "Backspace" key Press ***/
void backspaceDir(int c)
{
    if (mode == false)
    {
        string dest = "..";
        dest += "\0";
        const char *dests = dest.c_str();
        int ch = chdir(dests);
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            if ((!last_move))
            {
                // this move is backspace hence it is not < or >
                history.push_back(cwd);
                last_move = false;
                curr_pos = history.size();
            }
            else if ((last_move))
            {
                // this move is backspace hence it is not < or >
                history.erase(history.begin() + curr_pos, history.end()); // check +1 or not
                history.push_back(cwd);
                last_move = false;
                curr_pos = history.size() - 1;
            }
        }
        else
        {
            die("getcwd() error");
        }
        /*if the change of directory was successful it will print successful otherwise it will print not successful*/
        if (ch < 0)
            die("chdir change of directory unsuccessful");
        else
        {
            directoryOpen(".");
        }
    }
}
/*** Error handling ***/
void die(const char *s)
{
    cout << "\x1b[2J"
         << "\x1b[H" << s;
    exit(1);
}
/*** Get File Details ***/
string get_details(char *file_name)
{
    struct stat sb;
    struct passwd *pwd;
    struct group *grp;
    string ans = file_name;
    string file_Name = file_name;
    curr_dir_info *new_file = new curr_dir_info();
    new_file->filename = file_Name;
    if (stat(file_name, &sb) == -1)
    {
        die("stat");
    }
    ans += "\t";
    string file_size;
    if (T_B < ((long long)sb.st_size))
    {
        ans += to_string((((long long)sb.st_size) / T_B));
        ans += "TB";
        file_size += to_string((((long long)sb.st_size) / T_B));
        file_size += "TB";
    }
    else if (G_B < ((long long)sb.st_size))
    {
        ans += to_string((((long long)sb.st_size) / G_B));
        ans += "GB";
        file_size += to_string((((long long)sb.st_size) / G_B));
        file_size += "GB";
    }
    else if (M_B < ((long long)sb.st_size))
    {
        ans += to_string((((long long)sb.st_size) / M_B));
        ans += "MB";
        file_size += to_string((((long long)sb.st_size) / M_B));
        file_size += "MB";
    }
    else if (K_B < ((long long)sb.st_size))
    {
        ans += to_string((((long long)sb.st_size) / K_B));
        ans += "KB";
        file_size += to_string((((long long)sb.st_size) / K_B));
        file_size += "KB";
    }
    else if (K_B > ((long long)sb.st_size))
    {
        ans += to_string((((long long)sb.st_size)));
        ans += "B";
        file_size += to_string((((long long)sb.st_size)));
        file_size += "B";
    }
    new_file->fileSize = file_size;
    ans += "\t";
    string user_id;
    if ((pwd = getpwuid(sb.st_uid)) != NULL)
    {
        ans += (pwd->pw_name);
        user_id += (pwd->pw_name);
    }
    else
    {
        ans += (sb.st_uid);
        user_id += (sb.st_uid);
    }
    new_file->filesUserID = user_id;
    ans += "\t";
    string group_id;
    if ((grp = getgrgid(sb.st_gid)) != NULL)
    {
        ans += (grp->gr_name);
        group_id += (grp->gr_name);
    }
    else
    {
        ans += (sb.st_gid);
        group_id += (sb.st_gid);
    }
    new_file->filesGroupId = group_id;
    ans += "\t";
    string permission;
    if ((S_ISDIR(sb.st_mode)))
    {
        ans += "d";
        permission += "d";
    }
    else
    {
        ans += "-";
        permission += "-";
    }
    if ((sb.st_mode & S_IRUSR))
    {
        ans += "r";
        permission += "r";
    }
    else
    {
        ans += "-";
        permission += "-";
    }
    if ((sb.st_mode & S_IWUSR))
    {
        ans += "w";
        permission += "w";
    }
    else
    {
        ans += "-";
        permission += "-";
    }
    if ((sb.st_mode & S_IXUSR))
    {
        ans += "x";
        permission += "x";
    }
    else
    {
        ans += "-";
        permission += "-";
    }
    if ((sb.st_mode & S_IRGRP))
    {
        ans += "r";
        permission += "r";
    }
    else
    {
        ans += "-";
        permission += "-";
    }
    if ((sb.st_mode & S_IWGRP))
    {
        ans += "w";
        permission += "w";
    }
    else
    {
        ans += "-";
        permission += "-";
    }
    if ((sb.st_mode & S_IXGRP))
    {
        ans += "x";
        permission += "x";
    }
    else
    {
        ans += "-";
        permission += "-";
    }
    if ((sb.st_mode & S_IROTH))
    {
        ans += "r";
        permission += "r";
    }
    else
    {
        ans += "-";
        permission += "-";
    }
    if ((sb.st_mode & S_IWOTH))
    {
        ans += "w";
        permission += "w";
    }
    else
    {
        ans += "-";
        permission += "-";
    }
    if ((sb.st_mode & S_IXOTH))
    {
        ans += "x";
        permission += "x";
    }
    else
    {
        ans += "-";
        permission += "-";
    }
    ans += "\t";
    new_file->filePermission = permission;
    string modified_time;
    modified_time += ctime(&sb.st_mtime);
    new_file->filelastmodified = modified_time;
    ans += ctime(&sb.st_mtime);
    fils_info.push_back(new_file);
    return ans;
}

/*** Process Command Mode Input ***/
void process_string(string query)
{
    string command;
    vector<string> source;
    string destination;
    string temp;
    bool command_fetched = false;
    for (int i = 13; i < query.length(); i++)
    {
        if ((query[i] != ' ') && command_fetched == false)
        {
            command += query[i];
        }
        else if (query[i] == ' ' && command_fetched == false)
        {
            command_fetched = true;
            continue;
        }
        else if ((query[i] != ' ') && command_fetched == true)
        {
            temp += query[i];
        }
        else if ((query[i] == ' ') && (i > 0) && (query[i - 1] == '\\') && command_fetched == true && (temp != ""))
        {
            temp.pop_back();
            temp += query[i];
        }
        else if ((query[i] == ' ') && command_fetched == true && (temp != ""))
        {
            source.push_back(temp);
            temp = "";
        }
    }
    source.push_back(temp);
    if (command == "search" && source.size() == 1)
    { // working
        main_search(source[0]);
        return;
    }
    else if (command == "search" && source.size() != 1)
    {
        die("Mismatched Number of parameter(s)");
    } // will work for . & ./ also

    if (command == "rename" && source.size() == 2)
    { // working
        // command:rename foo.txt bar.txt
        destination = source[source.size() - 1];
        source.pop_back();
        string src1 = sanitise_input(source[0]);
        string src2 = sanitise_input(destination);
        main_rename(src1, src2);
        return;
    }
    else if (command == "rename" && source.size() != 2)
    {
        die("Mismatched Number of parameter(s)");
    } // will work for . & ./ also

    if (command == "create_file" && source.size() == 2)
    { // working
        // command: create_file <file_name> <destination_path>
        destination = source[source.size() - 1];
        source.pop_back();
        string dir_loc = sanitise_input(destination);
        main_create_file(source[0], dir_loc);
        return;
    }
    else if (command == "create_file" && source.size() != 2)
    {
        die("Mismatched Number of parameter(s)");
    } // will work for . & ./ also

    if (command == "create_dir" && source.size() == 2)
    { // working
        // command: create_dir <dir_name> <destination_path>
        destination = source[source.size() - 1];
        source.pop_back();
        string dir_loc = sanitise_input(destination);
        main_create_dir(source[0], dir_loc); // all working
        return;
    }
    else if (command == "create_dir" && source.size() != 2)
    {
        die("Mismatched Number of parameter(s)");
    } // will work for . & ./ also

    if (command == "copy" && source.size() >= 2)
    { // 1: Directory, 0:file, -1:error
        destination = source[source.size() - 1];
        source.pop_back();
        for (int i = 0; i < source.size(); i++)
        {
            string src_loc = sanitise_input(source[i]);
            int temp = is_dir(src_loc);
            if (temp == 0)
            { // working for files
                // copy file command: copy foo.txt bar.txt baz.mp4 ~/foobar
                string dest_loc = sanitise_input(destination); // dest may not exist
                main_copy_files(src_loc, dest_loc);
            } // works for . also
            else if (temp == 1)
            {
                string dest_loc = sanitise_input(destination); // dest may not exist
                main_copy_directory(src_loc, dest_loc);
            }
            else if (temp == -1)
            {
                die("copy command is_dir check failed");
            }
        }
        return;
    }
    else if (command == "copy" && source.size() < 2)
    {
        die("Mismatched number of parameter");
    }
    if (command == "move" && source.size() >= 2)
    {
        destination = source[source.size() - 1];
        source.pop_back();
        string locs;
        for (int i = 0; i < source.size(); i++)
        {
            string src_loc = sanitise_input(source[i]);
            int temp = is_dir(src_loc);
            if (temp == 0)
            { // working for files
                // move file command: copy foo.txt bar.txt baz.mp4 ~/foobar
                string dest_loc = sanitise_input(destination); // dest may not exist
                main_move_file(src_loc, dest_loc);
            } // works for . also
            else if (temp == 1)
            {
                // command: move footxt bartxt bazmp4 ~/foobar
                string dest_loc = sanitise_input(destination); // dest may not exist
                main_move_dir(src_loc, dest_loc);
            }
            else if (temp == -1)
            {
                die("move command is_dir check failed");
            }
        }
        return;
    }
    else if (command == "move" && source.size() < 2)
    {
        die("Mismatched number of parameter");
    }
    if (command == "delete_file" && source.size() == 1)
    {
        // command: delete_file <file_path>’
        string src_loc = sanitise_input(source[0]);
        main_delelte_file(src_loc);
        return;
    }
    else if (command == "delete_file" && source.size() != 1)
    {
        die("Mismatched number of parameter");
    }

    if (command == "delete_dir" && source.size() == 1)
    {
        // Delete Directory command: ‘delete_dir <dir_path>’
        string src_loc = sanitise_input(source[0]);
        main_delete_files_folder(src_loc);
        return;
    }
    else if (command == "delete_dir" && source.size() != 1)
    {
        die("Mismatched number of parameter");
    }
    if (command == "goto" && source.size() == 1)
    {
        goto_dir(source[0].c_str());
        return;
    }
    else
    {
        die("Invalid Command");
    }
}

/*** Command Mode ***/
void commandMode(int c)
{
    if (E.cy == E.numrows)
    {
        char *a = (char *)malloc(2); // this can fail anytime
        appendRow(a, 0);
    }
    string typed = "Command Mode:";
    setStatusMessage(typed.c_str());
    refreshScreen();
    int nread;
    char cb;
    while (true)
    {
        int status = read(STDIN_FILENO, &cb, 1);
        if (status == -1 && errno != EAGAIN)
            die("read");
        if (cb == 27)
        {
            // Escape pressed
            typed = "Normal Mode";
            setStatusMessage(typed.c_str());
            refreshScreen();
            return;
        }
        else if (cb == 13)
        {
            // Enter Pressed
            process_string(typed);
            typed = "Command Mode:";
            setStatusMessage(typed.c_str());
            refreshScreen();
        }
        else if (cb == 127 && typed.length() > 13)
        {
            // Backspace Pressed
            typed.pop_back();
            setStatusMessage(typed.c_str());
            refreshScreen();
        }
        else
        {
            if (cb == 'q' && typed.length() == 13)
            {
                //'q' pressed, but exit only if noting has been typed so far
                // as 'q' can appear in file and folder name also
                die("Program Exited");
            }
            typed += cb;
            setStatusMessage(typed.c_str());
            refreshScreen();
        }
    }
    E.cx++; // needed anymore?
}

/*** Get Current Directory Detals ***/
void directoryOpen(string str)
{
    init();
    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    struct dirent *de;
    E.filename = strdup(str.c_str());
    vector<string> strs;
    DIR *dr = opendir(str.c_str());
    if (!dr)
    {
        string temp = "opendir (unable to open):" + str;
        die(temp.c_str());
    }
    while ((de = readdir(dr)) != NULL)
    {
        strs.push_back(get_details(de->d_name));
    }
    for (int i = 0; i < strs.size(); i++)
    {
        char *holder = strdup(strs[i].c_str());
        linelen = strs[i].length();
        appendRow(holder, linelen - 1);
    }
    closedir(dr);
}

int getWindowSize(int *rows, int *cols)
{
    struct winsize ws;
    int status;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
        {
            die("getWindowSize: fallBack & ioctl Failed");
            return -1;
        }
        else
        {
            unsigned int i = 0;
            char buf[32];
            if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
            {
                die("getWindowSize: Reading cursor position from Screen failed");
                return -1;
            }
            while (i < sizeof(buf) - 1)
            {
                if (read(STDIN_FILENO, &buf[i], 1) != 1)
                {
                    break;
                }
                if (buf[i] == 'R')
                {
                    break;
                }
                i++;
            }
            buf[i] = '\0';
            if (buf[0] != '\x1b' || buf[1] != '[')
            {
                die("getWindowSize: Wrong content in buffer");
                return -1;
            }
            if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
            {
                die("getWindowSize: Reading cursor position from Buffer failed");
                return -1;
            }
            return 0;
        }
    }
    else
    {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/*** input handle ***/
int readKey()
{
    char c;
    int prev = -1;
    int prev_to_prev = -1;
    int prev_to_prev_to_prev = -1;
    bool mode = false;
    while (true)
    {
        int status = read(STDIN_FILENO, &c, 1);
        if (status == -1 && errno != EAGAIN)
            die("read");
        if (c == 'q')
        {
            write(STDOUT_FILENO, "\x1b[2J", 4); // clear scrren before exit
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
        }
        if (iscntrl(c))
        {
            if (c == 27 && mode)
            {
                printf("Normal Mode\r\n");
                mode = false;
            }
            else if (c == 27 && mode == false)
            {
                // we are at Normal mode and input is escape which is meaningless
            }
            else
            {
                if (mode == false)
                {
                    if (c == 127)
                        return -17; //-17 Backspace

                    if (c == 13)
                        return -21; //-21 Enter
                }
            }
        }
        else
        {
            if ((c == 126) && (prev == 53) && (prev_to_prev == 91) && (prev_to_prev_to_prev == 27) && mode == false)
            {
                return -7; // printf("-7 PG UP\r\n");
            }
            else if ((c == 126) && (prev == 54) && (prev_to_prev == 91) && (prev_to_prev_to_prev == 27) && mode == false)
            {
                return -8; // printf("-8 PG Down\r\n");
            }
            else if ((c == 126) && (prev == 50) && (prev_to_prev == 91) && (prev_to_prev_to_prev == 27) && mode == false)
            {
                return -22; // printf("-22 Insert\r\n");
            }
            else if ((c == 126) && (prev == 51) && (prev_to_prev == 91) && (prev_to_prev_to_prev == 27) && mode == false)
            {
                return -23; // printf("-23 Delete\r\n");
            }
            else if ((c == 65) && (prev == 91) && (prev_to_prev == 27) && mode == false)
            {
                return -1; // printf("-1 Up\r\n");
            }
            else if ((c == 66) && (prev == 91) && (prev_to_prev == 27) && mode == false)
            {
                return -2; // printf("-2 Down\r\n");
            }
            else if ((c == 68) && (prev == 91) && (prev_to_prev == 27) && mode == false)
            {
                return -3; // printf("-3 Left\r\n");
            }
            else if ((c == 67) && (prev == 91) && (prev_to_prev == 27) && mode == false)
            {
                return -4; // printf("-4 Right\r\n");
            }
            else if ((c == 72) && (prev == 91) && (prev_to_prev == 27) && mode == false)
            {
                return -5; // printf("-5 Home\r\n");
            }
            else if ((c == 70) && (prev == 91) && (prev_to_prev == 27) && mode == false)
            {
                return -6; // printf("-6 End\r\n");
            }
            else if (c == 43 && mode == false)
                return -9; // printf("-9 +\r\n");
            else if (c == 45 && mode == false)
                return -10; // printf("-10 -\r\n");
            else if (c == 108 && mode == false)
                return -13; // printf("-13 l\r\n");
            else if (c == 107 && mode == false)
                return -14; // printf("-14 k\r\n");
            else if (c == 93 && mode == false)
                return -15; // printf("-15 ]\r\n");
            else if (c == 91 && prev != 27 && mode == false)
                return -16; // printf("-16 [\r\n");
            else if (c == 104 && mode == false)
                return -18; // printf("-18 h\r\n");
            else if (c == 113 && mode == false)
                return -19; // printf("-19 q\r\n");
            else if (c == 58 && mode == false)
            {
                mode = true;
                return -20; // printf("-20 :\r\n");
            }
        }
        prev_to_prev_to_prev = prev_to_prev;
        prev_to_prev = prev;
        prev = c;
    }
    return -50;
}

/*** Scroll ***/
void scroller()
{
    // To do transformation
    // when we move up, there may not be any character above line,just above it
    // when we move down, there may not be any character below it,just below it
    // when we move left from x coordinate=0, we have to go one line up
    // when we are at end of current line and we press right well will need to go down
    E.s_rx(0);
    if (E.g_cy() < E.g_numrows())
    {
        E.s_rx(rowCxToRx(&E.row[E.g_cy()], E.g_cx()));
    }
    if (E.g_cy() < E.g_rowoff())
    {
        E.s_rowoff(E.g_cy());
    }
    if (E.g_cy() >= E.g_rowoff() + E.g_screenrows())
    {
        E.s_rowoff(E.g_cy() - E.g_screenrows() + 1);
    }
    if (E.g_rx() < E.g_coloff())
    {
        E.s_coloff(E.g_rx());
    }
    if (E.g_rx() >= E.g_coloff() + E.g_screencols())
    {
        E.s_coloff(E.g_rx() - E.g_screencols() + 1);
    }
}

/*** input ***/
void move_cursor(int key)
{
    erow *row;
    if ((E.g_cy() >= E.g_numrows()))
    {
        row = NULL;
    }
    else
    {
        row = &E.row[E.g_cy()];
    }
    if (key == -10)
    {
        if (E.g_cy() != 0)
        {
            int a = E.g_cy();
            E.s_cy(a - 1);
        }
    }
    else if (key == -9)
    {
        if (E.cy < E.numrows)
        { // adds scroll feature on press down key
            int a = E.g_cy();
            E.s_cy(a + 1);
        }
    }
    else if (key == -15)
    { //]
        if (row && E.cx < row->size)
        {
            int a = E.g_cx();
            E.s_cx(a + 1);
        }
        else if (row && E.cx == row->size)
        {
            int a = E.g_cy();
            E.s_cy(a + 1);
            E.s_cx(0);
        }
    }
    else if (key == -16)
    { //[
        if (E.cx != 0)
        {
            int a = E.g_cx();
            E.s_cx(a - 1);
        }
        else if (E.cy > 0)
        {
            // if x coordinate was 0, and we pressed left arrow key,, then adjust y cooredinat ealso
            int a = E.g_cy();
            E.s_cy(a - 1);
            E.s_cx(E.row[E.g_cy()].size);
        }
    }
    else if (key == -1)
    {
        if (E.rowoff != E.cy)
        {
            int a = E.g_cy();
            E.s_cy(a - 1);
        }
    }
    else if (key == -2)
    {
        if (E.cy < E.rowoff + E.screenrows - 1)
        {
            int a = E.g_cy();
            E.s_cy(a + 1);
            if (E.g_cy() > E.g_numrows())
                E.s_cy(E.g_numrows());
        }
    }
    else if ((key == -14) || (key == -13))
    {
        int temp = E.g_cy();
        bool flag_up = false;
        if (key == -14)
        { // up
            flag_up = true;
            E.s_cy(E.g_rowoff());
        }
        else if (key == -13)
        { // down
            E.s_cy(E.g_rowoff() + E.g_screenrows() - 1);
            if (E.g_cy() > E.g_numrows())
                E.s_cy(E.g_numrows());
        }
        if (key == -14)
            move_cursor(-10);
        else if (key != -14)
            move_cursor(-9);
    }
    if (E.g_cy() >= E.g_numrows())
    { // do adjust ment to not go beyond the line
        row = NULL;
    }
    else
    {
        row = &E.row[E.g_cy()];
    }
    int rowlen;
    if (row)
    {
        rowlen = row->size;
    }
    else
    {
        rowlen = 0;
    }
    if (E.g_cx() > rowlen)
    {
        E.s_cx(rowlen);
    }
}

void processKeyPress()
{
    int c = readKey();
    if (c == -19)
    {
        write(STDOUT_FILENO, "\x1b[2J", 4); // clear scrren before exit
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);
    }
    else if (c == -5)
    {
        E.s_cx(0);
    }
    else if (c == -6)
    {
        if (E.g_cy() < E.g_numrows())
            E.s_cx(E.row[E.g_cy()].size);
    }
    else if ((c == -7) || (c == -8))
    {
        if (c == -7)
        {
            E.s_cy(E.g_rowoff());
        }
        else if (c == -8)
        {
            E.s_cy(E.g_rowoff() + E.g_screenrows() - 1);
            if (E.g_cy() > E.g_numrows())
                E.s_cy(E.g_numrows());
        }
        int times = E.g_screenrows();
        if (c == -7)
            while (times--)
                move_cursor(-10);
        if (c != -7)
            while (times--)
                move_cursor(-9);
    }

    else if ((c == -10) || (c == -9))
    {
        move_cursor(c);
    }
    else if ((c == -21))
    {
        enterDir(c);
    }
    else if ((c == -17))
    {
        backspaceDir(c);
    }
    else if ((c == -3))
    {
        move_left(c);
    }
    else if ((c == -4))
    {
        move_right(c);
    }
    else if ((c == -18))
    {
        move_home(c);
    }
    else if ((c == -20))
    {
        commandMode(c);
    }
    else if ((c == -15))
    { //]
        move_cursor(c);
    }
    else if ((c == -16))
    { //[
        move_cursor(c);
    }
    else if ((c == -1))
    { //-
        move_cursor(c);
    }
    else if ((c == -2))
    { //+
        move_cursor(c);
    }
    else if ((c == -13))
    { // k
        move_cursor(c);
    }
    else if ((c == -14))
    { // l
        move_cursor(c);
    }
}

void abAppend(struct abuf *ab, const char *s, int len)
{
    char *new_ = (char *)realloc(ab->b, ab->len + len);
    if (new_ == NULL)
    {
        die("Unable to allocate Memory For Display");
        return;
    }
    else if (new_ != NULL)
    {
        memcpy(&new_[ab->len], s, len);
        ab->b = new_;
        int temp = ab->len;
        ab->len = temp + len;
        return;
    }
    return;
}

/***Free Buffer ***/
void abFree(struct abuf *ab)
{
    free(ab->b);
    return;
}
/*** Message Bar***/

void drawMessageBar(struct abuf *ab)
{
    abAppend(ab, "\x1b[K", 3);
    abAppend(ab, "\x1b[7m", 4); // invert color
    int msglen = strlen(E.statusmsg);
    if (msglen > E.g_screencols())
    {
        msglen = E.g_screencols();
    }
    abAppend(ab, E.statusmsg, msglen);
    if (msglen > E.g_screencols())
    {
        msglen = E.g_screencols();
    }
    while (msglen < E.g_screencols())
    {
        abAppend(ab, " ", 1); // append blank space after texts
        msglen++;
    }
    abAppend(ab, "\x1b[m", 3); // change color back to what it was
}

/*** Set the content of Status Bar ***/
void setStatusMessage(const char *fmt, ...)
{
    va_list ap; // to handle all the arguments seent to this function & to print them all instead of few
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
}

/*** Transformation Needed so That TAB SPACE does not cause issue ***/
int rowCxToRx(erow *row, int cx)
{
    int rx = 0;
    int j;
    for (j = 0; j < cx; j++)
    {
        if (row->chars[j] == '\t')
            rx += (TAB_SPACE - 1) - (rx % TAB_SPACE);
        rx++;
    }
    return rx;
}
/*** Draw Rows On Screen ***/
void draw_rows(struct abuf *ab)
{
    int y;
    int screen_size = E.g_screenrows();
    for (y = 0; y < screen_size; y++)
    {                                   // print till dcreen size only,itterate over all row of screen
        int filerow = y + E.g_rowoff(); // for scrolling part, filerow tell wich row of file to print
        if (y + E.g_rowoff() >= E.g_numrows())
        { // number of rows to be is more than screen row size
            if (E.g_numrows() != 0)
            {
                abAppend(ab, "~", 1);
                // for those line where Directory content is not to be displayed just print ~
            }
        }
        else
        {
            // Print buffer content
            int len = E.row[filerow].rsize - E.g_coloff();
            if (len < 0)
                len = 0;
            if (len > E.g_screencols())
            {
                len = E.g_screencols();
            }
            abAppend(ab, &E.row[filerow].render[E.g_coloff()], len);
        }
        abAppend(ab, "\x1b[K", 3);
        abAppend(ab, "\r\n", 2);
    }
}

/*** Update Row Contents ***/
void updateRow(erow *row)
{
    int tabs = 0;
    int j;
    int row_sizes = row->size;
    for (j = 0; j < row_sizes; j++)
        if (row->chars[j] == '\t')
            tabs++;
    free(row->render);
    row->render = (char *)malloc(row->size + tabs * (TAB_SPACE - 1) + 1);
    int idx = 0;
    for (j = 0; j < row->size; j++)
    {
        if (row->chars[j] == '\t')
        {
            row->render[idx++] = ' ';
            while (idx % TAB_SPACE != 0)
                row->render[idx++] = ' ';
        }
        else
        {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
}

/*** Append in the row ***/
void appendRow(char *s, size_t len)
{
    E.row = (erow *)realloc(E.row, sizeof(erow) * (E.g_numrows() + 1));
    E.row[E.g_numrows()].size = len;
    E.row[E.g_numrows()].chars = (char *)malloc(len + 1);
    memcpy(E.row[E.g_numrows()].chars, s, len);
    E.row[E.g_numrows()].chars[len] = '\0';
    E.row[E.g_numrows()].rsize = 0;
    E.row[E.g_numrows()].render = NULL;
    updateRow(&E.row[E.g_numrows()]);
    int a = E.g_numrows();
    E.s_numrows(a + 1);
}
/*** Refresh screen ***/
void refreshScreen()
{
    scroller();
    struct abuf ab = {NULL, 0}; // initializing structure
    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);
    draw_rows(&ab);      // draw the buffer contents
    drawMessageBar(&ab); // for normal or command mode
    if (mode == false)
    {
        E.statusmsg[0] = 'N';
        E.statusmsg[1] = 'o';
        E.statusmsg[2] = 'r';
        E.statusmsg[3] = 'm';
        E.statusmsg[4] = 'a';
        E.statusmsg[5] = 'l';
        E.statusmsg[6] = ' ';
        E.statusmsg[7] = 'M';
        E.statusmsg[8] = 'o';
        E.statusmsg[9] = 'd';
        E.statusmsg[10] = 'e';
        E.statusmsg[11] = '\0';
    }
    char buf[32];
    int pos_x = (E.g_cy() - E.g_rowoff()) + 1;
    int pos_y = (E.g_rx() - E.g_coloff()) + 1;
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", pos_x, pos_y);
    abAppend(&ab, buf, strlen(buf));
    abAppend(&ab, "\x1b[?25h", 6);
    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}
int main(int argc, char *argv[])
{
    enableRawMode();
    init();
    const char *ab = ".";
    directoryOpen(ab);
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        history.push_back(cwd);
        home_dir = cwd;
    }
    else
    {
        die("Unable to get Cwd");
    }
    setStatusMessage("Normal Mode");
    while (1)
    {
        refreshScreen();
        processKeyPress();
    }
    return 0;
}