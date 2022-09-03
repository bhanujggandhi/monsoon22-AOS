#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

enum
{
    ESC_KEY = 27
};
enum
{
    EOF_KEY = 4
};

char f()
{
    char key;
    if (read(STDIN_FILENO, &key, 1) != 1)
    {
        fprintf(stderr, "read error or EOF\n");
        return -1;
    }
    if (key == EOF_KEY)
    {
        fprintf(stderr, "%d (control-D or EOF)\n", key);
        return -1;
    }

    // check if ESC
    if (key == 27)
    {
        fd_set set;
        struct timeval timeout;
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
        int selret = select(1, &set, NULL, NULL, &timeout);
        // printf("selret=%i\n", selret);
        if (selret == 1)
        {
        }
        else if (selret == -1)
        {
        }
        else
        {
            return key;
        }
    }
    else
        return key;
    if (key == 'x')
        return 'x';
}

int main(void)
{
    // put terminal into non-canonical mode
    struct termios old;
    struct termios newt;
    int fd = 0; // stdin
    tcgetattr(fd, &old);
    // memcpy(&new, &old, sizeof(old));
    newt = old;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(fd, TCSANOW, &newt);

    // loop: get keypress and display (exit via 'x')
    // int key;
    printf("Enter a key to see the ASCII value; press x to exit.\n");

    while (1)
    {
        char c = f();
        if (c == 'x')
            break;
        else
            printf("%i\n", f());
    }

    // set terminal back to canonical
    tcsetattr(fd, TCSANOW, &old);
    return 0;
}
