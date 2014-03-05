#ifndef FS_H
#define FS_H

#define FILENAMELEN 32
#define FILEBLOCKS 12
#define DIRECTORY_SIZE 16
#define FILETABLEN 100

#define MDEV_BLOCK_SIZE 512
#define MDEV_NUM_BLOCKS 512
#define DEFAULT_NUM_INODES (MDEV_NUM_BLOCKS / 4)

/* Modes of file*/
#define O_CREAT 11

/* Flags of file*/
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2

/* Types of File*/
#define TYPE_FILE       0
#define TYPE_DIRECTORY  1

/* State of in core inode*/
#define LOCKED   0
#define UNLOCKED 1

#define UNUSED -9999
#define USED    9999

struct in_core_inode {
  int id;
  int state;
  int ref_count;
  short int type;
  short int nlink;
  int device;
  int size;
  int blocks[FILEBLOCKS];
};

struct inode {
  int id;
  short int type;
  short int nlink;
  int device;
  int size;
  int blocks[FILEBLOCKS];
};

struct fdesc {
  int state;
  int fptr;
  int mode;
  int in_core_inode_num;
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
  int nblocks;
  int blocksz;
  int ninodes;
  int inodes_used;
  int freemaskbytes;
  char *freemask;
  struct directory root_dir;
};

extern struct fsystem fsd;
extern struct fdesc openfiletable[FILETABLEN];
extern struct in_core_inode core_inode[FILETABLEN];

/* Inline function to convert inode number to block no*/
#define INODE2BLOCKNO(inode_num) ((inode_num/8) + 2)

/* Inline function to convert inode number to offset */
#define INODE2OFFSET(inode_num)  ((inode_num%8) * sizeof(struct inode))
 
/* file and directory functions */
int fopen(char *filename, int flags);
int fclose(int fd);
int fcreat(char *filename, int mode);
int fseek(int fd, int offset);
int fread(int fd, void *buf, int nbytes);
int fwrite(int fd, void *buf, int nbytes);

/* filesystem functions */
int mkfs(int dev, int num_inodes);
int mount(int dev);

/* filesystem internal functions */
int get_inode_by_name(char *filename);
int get_inode_by_num(int dev, int inode_number);
int put_inode_by_num(int dev, int incore_inode_index);
int allocate_block(int incore_inode_index, int datablock_no);
int block_offset_write(int dev, int block, int offset, void * buf, int len);
int block_no_from_datablock_no(int incore_inode_index, int datablock_no);
int setmaskbit(int b);
int clearmaskbit(int b);
int getmaskbit(int b);

/*
  Block Store functions
  bread, bwrite,
  bput, bget write entire blocks (macro with offset=0, len=blocksize)
 */
int mkbsdev(int dev, int blocksize, int numblocks);
int bread(int bsdev, int block, int offset, void *buf, int len);
int bwrite(int bsdev, int block, int offset, void * buf, int len);


/* debugging functions */
void printfreemask(void);
void print_fsd(void);

#endif /* FS_H */
