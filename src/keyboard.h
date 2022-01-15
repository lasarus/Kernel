#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "vfs.h"

void keyboard_feed_scancode(unsigned char scancode);
char keyboard_read_next();

void keyboard_init_inode(struct inode *inode);

#endif
