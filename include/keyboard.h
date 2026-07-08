#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

void init_kb();

void close_kb();

int kbhit();

char getch();

#endif 