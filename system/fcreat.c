#include <fs.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <thread.h>

int fcreat(char *filename, int mode)
{
	int i;                                        //counter
  int bm_blk = 0;                               //Directory block
  int length;
  int inode;
  int incore_inode;
  int filetablentry;
  int dev_desc;
  struct thrent *thrptr;
   
  if(mode != O_CREAT)
  {
    printf("\n\r Invalid Mode");
    return -1;
  }    
  
  //Check for max File descriptors for process 
  thrptr = &thrtab[thrcurrent];
  for(i=3; i<NDESC; i++)
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
  
	//Check filename length
	length = strnlen( filename, FILENAMELEN);
	if (length > FILENAMELEN) 
	{
		return -1;
	}
	
	//Read directory into memory
  //if(bread(0, bm_blk, 0, &fsd, sizeof(struct fsystem)) == SYSERR)
	//{
	//  return -1;
	//}
	
	//Check root directory for same filename
	if(get_inode_by_name(filename) != SYSERR)
	{
	  printf("\n\r File alrady exist");
	  return -1;
  }
	
	
  //Check number of used inode
  if(fsd.inodes_used >= DIRECTORY_SIZE)                                     //HARDCODING
  {
    printf("All inode are in use \n\r");
    return -1;
  }	
  
  //Create file in directory
  fsd.inodes_used = fsd.inodes_used + 1;
  fsd.root_dir.numentries = fsd.root_dir.numentries + 1;
  
  //Find empty directory entry from predefined list.
  for(i=0 ;i<DIRECTORY_SIZE; i++)
	{
	  if(fsd.root_dir.entry[i].inode_num == UNUSED )
	  {
	    break;
	  }
	}
	
	inode = i;
	
	//find free slot in incore list 
  for(i=0 ;i<FILETABLEN; i++)
	{
	    if(core_inode[i].id == UNUSED){
	       break;
	    }
	}
	
	if(i < FILETABLEN)
	{
    incore_inode = i;
  }
  else
  {
    printf("\n\r In core list is full");
    return -1;
  }
  
  core_inode[incore_inode].id         = inode ;
  core_inode[incore_inode].state      = UNLOCKED;
  core_inode[incore_inode].ref_count  = 1;
	core_inode[incore_inode].type       = TYPE_FILE;
  core_inode[incore_inode].nlink      = 1;
  core_inode[incore_inode].device     = 0;
  core_inode[incore_inode].size       = 0;
  for(i=0 ;i<FILEBLOCKS; i++)
	{
    core_inode[incore_inode].blocks[i] = UNUSED;
	}
	
	//store inode number
	fsd.root_dir.entry[inode].inode_num = inode;
	
	//copy file name 
	strncpy(fsd.root_dir.entry[inode].name,filename, length);
	
	//put inode in inode table 
  put_inode_by_num( 0, incore_inode);
  
  //Write directory into memory
  //if(bwrite(0, bm_blk, 0, &fsd, sizeof(struct fsystem)) == SYSERR)
	//{
	//  return -1;
	//}
    
  //find free slot in open file table
  for(i=0 ;i<FILETABLEN; i++)
	{
	    if(openfiletable[i].state == UNUSED){
	       break;
	    }
	}
	
	if(i < FILETABLEN)
	{
    filetablentry = i;
  }
  else
  {
    printf("\n\r Open file table full");
    return -1;
  }
  
  //Update open file table for new file 
  openfiletable[filetablentry].state = USED; 
  openfiletable[filetablentry].mode = O_RDWR;
  openfiletable[filetablentry].fptr = 0;
  openfiletable[filetablentry].in_core_inode_num  = incore_inode;
  
  thrptr->fdesc[dev_desc] = filetablentry;
  return dev_desc;
}
