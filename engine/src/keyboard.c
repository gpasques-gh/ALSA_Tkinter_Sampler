#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/time.h>

#include "keyboard.h"

static struct termios old, new;

void init_kb()
{
    tcgetattr(STDIN_FILENO, &old);
    new = old;
    new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new);
}

void close_kb()
{
   tcsetattr(STDIN_FILENO, TCSANOW, &old);
}

int kbhit()
{
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
}

char getch()
{
    char c;
    read(STDIN_FILENO, &c, 1);
    return c;
}

