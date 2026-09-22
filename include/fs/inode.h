#ifndef INODE_H
#define INODE_H

#include <stddef.h>
#include <stdint.h>

#define MAX_FILENAME_SIZE 100

struct inode {
    uint32_t inumber;
    char filename[MAX_FILENAME_SIZE];
    size_t size;
    // we'll need several MBs at least per file, this is an
    // estimate for now. each block is 256 bytes
    // TODO bottleneck for displaying at a good frame rate
    // is going to be memory accesses and writing speed to VRAM.
    // actually, should just be VRAM by a bit.
    // for now, each block will be 1kb instead of 256 bytes.
    // but that makes this blocks array 256Kbs, making the inode
    // struct much larger.
    uint32_t blocks[64000];
};

#endif
