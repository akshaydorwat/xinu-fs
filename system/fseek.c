#include <fs.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <thread.h>

int fseek(int fd, int offset)
{
    struct thrent *thrptr;
    int incore_inode_index;
    int filetablentry;
    int seek;
    int size;
    
    thrptr = &thrtab[thrcurrent];
    if (thrptr->fdesc[fd] == UNUSED)
    {
      printf("\n\rInvalid File Descriptor %d.", fd);
      return 0;
    }
    
    filetablentry = thrptr->fdesc[fd];
    if (openfiletable[filetablentry].state == UNUSED)
    {
      printf("\n\rOpen file table entry not found.");
      return 0;
    }
    
    incore_inode_index = openfiletable[filetablentry].in_core_inode_num;
    if(core_inode[incore_inode_index].id == UNUSED)
    {
        printf("\n\rCore Inode table entry not found");
        return 0;
    }
    
    size = core_inode[incore_inode_index].size;
    seek = openfiletable[filetablentry].fptr;
    
    if (((seek+offset) >= 0) && ((seek+offset) <= size))
    {
        openfiletable[filetablentry].fptr = (seek+offset); 
        return seek+offset;
    }
    else
    {    
        openfiletable[filetablentry].fptr =  seek;
        return SYSERR;
    }
    
    return SYSERR;
}
