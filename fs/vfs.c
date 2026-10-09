#include <asm/vfs.h>

// TODO(scheduling) make this per-process
struct file_descriptor ft_table[MAX_OPEN_FILES];

int open(const char *path, uint32_t permissions, uint32_t flags) {
  return 0;
}

uint32_t read(int inode_i, void *dest, uint32_t size) {
  return 0;
}

uint32_t write(int inode_i, void *src, uint32_t size) {
  return 0;
}

uint32_t block_read(int inode_index, void *dest) {
  return 0;
}

uint32_t block_write(int inode_index, void *src) {
  return 0;
}
