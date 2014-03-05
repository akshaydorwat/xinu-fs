#include <fs.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <thread.h>

int fopen(char *filename, int flags)
{
    int i;                                        //counter
    int incore_inode_index;
    int filetablentry;
    int dev_desc;
    struct thrent *thrptr;

    switch(flags)
    {
    
      case O_RDONLY: break;
      
      case O_WRONLY: break;
      
      case O_RDWR:   break;
      
      default:  printf("\n\r Invalid flag"); 
                return -1;
    }
    
    i = get_inode_by_name(filename);

    if (i == SYSERR)
    {
        printf("\n\r%s: No such file or directory.", filename);
        return -1;
    }
    else
    {
        incore_inode_index = i; 
    }
	  
	  /*In core inode changes*/
	  core_inode[incore_inode_index].state = UNLOCKED;
    core_inode[incore_inode_index].ref_count ++;
    
    //find free slot in open file table
    for (i=0; i < FILETABLEN; i++)
	  {
	    if (openfiletable[i].state == UNUSED)
      {
	        break;
	    }
	  }
	
	  if (i < FILETABLEN)
	  {
        filetablentry = i;
    }
    else
    {
        printf("\n\r Open-file table is full.");
        return -1;
    }
  
    //Update open file table for new file
    openfiletable[filetablentry].state = USED; 
    openfiletable[filetablentry].mode = flags;
    openfiletable[filetablentry].fptr = 0;
    openfiletable[filetablentry].in_core_inode_num   = incore_inode_index;
  
    /* truncate logic here */

    //File descriptor 
    thrptr = &thrtab[thrcurrent];
    for (i=3; i < NDESC; i++)
    {
        if(thrptr->fdesc[i] == UNUSED)
        {
            break;
        }
    }
  
    if(i < NDESC)
	  {
        dev_desc = i;
    }
    else
    {
        printf("\n\r Maximum open file descriptor limit reached");
        return -1;
    }
  
    thrptr->fdesc[dev_desc] = filetablentry;
  
    return dev_desc;
}
