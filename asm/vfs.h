#ifndef VFS_H
#define VFS_H

#include <kernel/types.h>
#include <stdint.h>
#include <stdatomic.h>

#define FILENAME_LEN 64
#define BITMAP_SIZE 10
#define NUM_DATA_BLOCKS 1024
#define NUM_INODES 1024

#define MAX_OPEN_FILES 10

// TODO(research) why is it that:
// in most unix filesystems, this is 2
#define ROOT_INODE_NO 0
#define BLOCK_SIZE 4096 // 4kb, same as a (normal) page

// most files are small, so inodes have both direct and indirect (nested) pointers
// to data in memory.
#define NUM_DIRECT_POINTERS 16

#ifdef __x86_64__
#include <arch/x86_64/fs/vfs.h>
#else
#include <arch/i386/fs/vfs.h>
#endif

/* 'syscalls' (we don't have userspace yet, so these are still kernel functions) */
int open(const char *path, uint32_t permissions, uint32_t flags);
uint32_t read(int fd, void *dest, uint32_t size);
uint32_t write(int fd, void *src, uint32_t size);

/* plumbing */
void vfs_init(); // allocate memory for vfs metadata

/*
 * Read from a block in the virtual filesystem.
 *
 * @returns number of bytes successfully read.
 *
 */
uint32_t block_read(int inode_index, void *dest);
/*
 * Write to a block in the virtual filesystem.
 *
 * @returns number of bytes successfully written.
 */
uint32_t block_write(int inode_index, void *src);

// linked lists would be cleaner, but I don't like the idea of committing to
// linear random access lookup. so we use a tree of pointers instead.
//
// metadata size: p_m * (1 + b + b^2 + ... + b^{n - 1}) * pointer size
// where p_m is number of pointers, b is the branching size (physical data range per pointer),
// and pointer size is number of bytes per pointer (4)
//
// addressable range: number of leaf pointers * range per pointer
// p_m * b^n * page size
//
// 16 * 1024^2 * 4kb > 67GB addressable per block

struct superblock {
	uint32_t size;
};

// unix uses something similar, but has extra bits that we just won't use for now
// because there is no need to. so, they're reserved.
// https://man7.org/linux/man-pages/man7/inode.7.html
#define VFS_TYPE				0xF000
#define VFS_F_FILE			0x8000
#define VFS_F_DIR		    0x4000

#define VFS_PERMISSIONS 0x0F00
#define VFS_F_PROTECTED 0x0800 // needs to be superuser to access

#define VFS_IS_FILE(flags)				((flags & VFS_TYPE) == VFS_F_FILE)
#define VFS_IS_DIR(flags)					((flags & VFS_TYPE) == VFS_F_DIR)
#define VFS_IS_PROTECTED(flags)   ((flags & VFS_PERMISSIONS) == VFS_F_PROTECTED)

struct inode {
	uint16_t mode;
	uint16_t reserved;      // posix uses links here (not to be confused with refcount)
													// in file descriptor)
	uint32_t size;          // measured in bytes
	uint32_t num_blocks;    // number of blocks allocated to this file

	uint32_t time;          // time of last access
	uint32_t ctime;         // time this file was last changed
	uint32_t mtime;         // time this file was last modified
	uint32_t dtime;         // time this inode was deleted

	uint32_t direct[NUM_DIRECT_POINTERS];	// pointers to blocks containing file data.
																				// if an entry is 0, then it is not allocated.
																				// 1024 * 67GB -> much larger than what our current
																				// 4-layer paging can give us
	uint32_t indirect;
} __attribute__((packed));

struct file {
	char name[FILENAME_LEN];
};

struct file_descriptor {
	struct inode *inode_ptr;
	uint32_t file_offset;
	atomic_int refcount;
};

struct bitmap {
	uint32_t map[BITMAP_SIZE];
};

// very simple file system, from ostep
struct vsfs {
	struct superblock super;
	struct bitmap inode_bitmap;
	struct bitmap data_bitmap;
	struct inode inodes[NUM_INODES];
};

#endif
