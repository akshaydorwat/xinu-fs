#ifndef FS_H
#define FS_H

#define FILENAMELEN 32
#define FILEBLOCKS 14
#define DIRECTORY_SIZE 16

#define MDEV_BLOCK_SIZE 512
#define MDEV_NUM_BLOCKS 512

#define 
struct inode {
  int id;
  int device;
  int nlink;
  int size;
  int blocks[FILEBLOCKS];
};

struct fdesc {
  int state;
  struct inode in;
  int fptr;
};

struct dirent {
  int inode_num;
  char name[FILENAMELEN];
};

struct directory {
  int numentries;
  struct dirent entry[DIRECTORY_SIZE];
};

struct fsystem {
  int blocks;
  int blocksz;
  int freemaskbytes;
  char *freemask;
  struct directory root_dir;
};

extern struct fsystem fsd;
extern struct fdesc openfiletable[DIRECTORY_SIZE];

int fopen(char *filename, int flags);
int fclose(int fd);
int fcreat(char *filename, int mode);
int fseek(int fd, int offset);
int fread(int fd, void *buf, int nbytes);
int fwrite(int fd, void *buf, int nbytes);


int mkfs(int dev);

/*
  Block Store
  bread, bwrite,
  bput, bget write entire blocks (macro with offset=0, len=blocksize)
 */
int mkbsdev(int dev, int blocksize, int numblocks);
int bread(int bsdev, int block, int offset, void *buf, int len);
int bwrite(int bsdev, int block, int offset, void * buf, int len);

/* debugging functions */
void printfreemask(void);
int setmaskbit(int b);
int getmaskbit(int b);

#endif /* FS_H */
