#include <fs.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <thread.h>

int fread(int fd, void *buf, int nbytes)
{
    struct thrent *thrptr;
    int incore_inode_index;
    int filetablentry;
    int seek;
    int data_block_no;
    int data_block_offset;
    int no_bytes_read;
    int byes_to_read;
    int block_no;
    int start_poistion;
    int remaining;
    int i;
    int size;
    
    if (nbytes == 0)
    {
      printf("\n\rInvalid number of bytes to read.");
      return 0;
    }
    
    if (buf == NULL)
    {
      printf("\n\rInvalid buffer pointer ");
      return 0;
    }
    
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
    
    seek = 0;
    seek = openfiletable[filetablentry].fptr;
    incore_inode_index = openfiletable[filetablentry].in_core_inode_num;
    if(core_inode[incore_inode_index].id == UNUSED)
    {
        printf("\n\rCore Inode table entry not found");
        return 0;
    }
    
    size = core_inode[incore_inode_index].size;
    
    //logic to write partially filled block
    data_block_no = seek / MDEV_BLOCK_SIZE;
    data_block_offset = seek % MDEV_BLOCK_SIZE;
    no_bytes_read = 0;
    remaining  = ((seek + nbytes) > size) ? (size - seek) : nbytes;

    while(remaining > 0)
    {
        byes_to_read = ( remaining < (MDEV_BLOCK_SIZE - data_block_offset))? remaining : (MDEV_BLOCK_SIZE - data_block_offset);
        start_poistion  =  data_block_offset;
        
        block_no = block_no_from_datablock_no( incore_inode_index,  data_block_no);
        if((block_no == UNUSED) || (block_no == SYSERR))
        {
            printf("\n\r Invalid data block");
            return 0;  
        }
              
        //printf("\n\rReading with parameters:%d, %d, %d", block_no, start_poistion, byes_to_read);  
        if (block_offset_read( 0, block_no, start_poistion, buf+no_bytes_read, byes_to_read) == SYSERR)
        {
            printf("\n\r Error while reading data block");
            return 0;
        }
        
        no_bytes_read += byes_to_read;
        remaining -= byes_to_read;
        data_block_offset = 0;
        data_block_no++; 
    }
    
    openfiletable[filetablentry].fptr += no_bytes_read ;
    return no_bytes_read;
}
