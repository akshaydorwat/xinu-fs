#include <fs.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <thread.h>

int fwrite(int fd, void *buf, int nbytes)
{
  struct thrent *thrptr;
  int incore_inode_index;
  int filetablentry;
  int seek;
  int data_block_no;
  int data_block_offset;
  int no_bytes_written;
  int byes_to_write;
  int block_no;
  int start_poistion;
  int remaining;
  int i;

  
  if (nbytes == 0)
  {
      printf("\n\rInvalid number of bytes to write.");
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
  if (openfiletable[filetablentry].mode == O_RDONLY)
  {
      printf("\n\rFile not open for write/append.");
      return 0;
  }
  
  seek = openfiletable[filetablentry].fptr;
  incore_inode_index = openfiletable[filetablentry].in_core_inode_num;
  if(core_inode[incore_inode_index].id == UNUSED)
  {
      printf("\n\rCore Inode table entry not found");
      return 0;
  }
  
  //logic to write partially filled block
  data_block_no = seek / MDEV_BLOCK_SIZE;
  data_block_offset = seek % MDEV_BLOCK_SIZE;
  no_bytes_written = 0;
  remaining = nbytes;
  
  
  while( no_bytes_written < nbytes )
  {
      byes_to_write = ( remaining < (MDEV_BLOCK_SIZE - data_block_offset))? remaining : (MDEV_BLOCK_SIZE - data_block_offset);
      start_poistion  =  data_block_offset;
      
      block_no = block_no_from_datablock_no( incore_inode_index,  data_block_no);
      
      if(block_no == UNUSED)
      {
          block_no = allocate_block(incore_inode_index,  data_block_no);
          //printf("\n\r Allocated block no is : %d",block_no);
      }
      
      if(block_no == SYSERR)
      {
          printf("\n\r Invalid Data block number");
          break;   
      }
      
      //printf("\n\r Data Block no is : %d  and block no is : %d",data_block_no,block_no);
      if (block_offset_write( 0, block_no, start_poistion, buf+no_bytes_written, byes_to_write) == SYSERR)
      {
          printf("\n\r Error while writing data block");
          break;
      }
      
      no_bytes_written += byes_to_write;
      remaining -= byes_to_write;
      data_block_offset = 0;
      data_block_no++; 
  }
  
   // update file size and seek pointer.
    core_inode[incore_inode_index].size +=  no_bytes_written;
      openfiletable[filetablentry].fptr += no_bytes_written ;
  
  return no_bytes_written;
}


