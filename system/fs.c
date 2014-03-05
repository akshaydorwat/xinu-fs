#include <kernel.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <bufpool.h>

#if FS
#include <fs.h>

/*create initial directory*/
struct fsystem fsd;

/*create open file descriptor table  */	
struct fdesc openfiletable[FILETABLEN];

/*Create core inode list*/
struct in_core_inode core_inode[FILETABLEN];

int dev0_numblocks;	
int dev0_blocksize;
char *dev0_blocks;
int dev0 = 0;

int mkbsdev(int dev, int blocksize, int numblocks) {

  if (dev != 0) {
    printf("Unsupported device: %d\n", dev);
    return SYSERR;
  }

  if (blocksize != 0) {
    dev0_blocksize = blocksize;
  }
  else {
    dev0_blocksize = MDEV_BLOCK_SIZE;
  }

  if (numblocks != 0) {
    dev0_numblocks =  numblocks;
  }
  else {
    dev0_numblocks =  MDEV_NUM_BLOCKS;
  }

  if ( (dev0_blocks = memget(dev0_numblocks * dev0_blocksize)) == SYSERR) {
    printf("mkbsdev memgetfailed\n");
    return SYSERR;
  }

  return OK;

}

int mkfs(int dev, int num_inodes) {
	int i;
    int bm_blk = 0;
    int inodes_per_block;
    int inode_blocks;
       
	struct inode node,target;
	
    if (dev == 0) {
        fsd.nblocks = dev0_numblocks;
        fsd.blocksz = dev0_blocksize;
    }
    else {
        printf("Unsupported device\n");
        return SYSERR;
    }

    if(num_inodes < 1 || num_inodes > MDEV_NUM_BLOCKS){
        printf("Invalid number of inodes\n");
    }
    else {
        fsd.ninodes = num_inodes;
        fsd.inodes_used = 0;
    } 
  
	i = fsd.nblocks;
 	while ( (i % 8) != 0) {i++;}
 	fsd.freemaskbytes = i / 8; 

 	if ((fsd.freemask = memget(fsd.freemaskbytes)) == SYSERR) 
	{
 		printf("mkfs memget failed.\n");
  	return SYSERR;
	}

	/* zero the free mask */
 	for(i=0;i<fsd.freemaskbytes;i++) {
  	fsd.freemask[i] = '\0';
 	}
 	
  /*Initialize inode number in File system directory to UNSUED*/
  for(i=0 ;i<DIRECTORY_SIZE; i++)
	{
	  fsd.root_dir.entry[i].inode_num = UNUSED;
	}
	
	/*Initialize the opefiletable state to unused*/
	for(i=0; i<FILETABLEN; i++)
	{
	   openfiletable[i].state = UNUSED;
	}
	
	/*Initialize inode number in File system directory to -1*/
    for(i=0; i<FILETABLEN; i++)
	{
	    core_inode[i].id = UNUSED;
	}
		
 	/* write the fsystem block to block 0, mark block used */
 	setmaskbit(0);
 	bwrite(dev0, bm_blk, 0, &fsd, sizeof(struct fsystem));

 	/* Computation for number of blocks required to stote inode*/
	inodes_per_block = MDEV_BLOCK_SIZE / sizeof(struct inode);
	inode_blocks = num_inodes / inodes_per_block;
	
	
	/* Reserve blocks in memory for inode table*/
	for(i=2; i< inode_blocks + 2;i++ )                                                  //HARDCODED VLAUES HERE
	{
	  setmaskbit(i);
	}
	
	/* write the freemask in block 0, mark block used */
 	setmaskbit(1);
 	bwrite(dev0, bm_blk, 1, fsd.freemask, fsd.freemaskbytes);
    
    return OK;
}

int bread(int dev, int block, int offset, void *buf, int len) {
    char *bbase;

    if (dev != 0) {
        printf("Unsupported device\n");
        return SYSERR;
    }
    
    if (offset >= dev0_blocksize) {
        printf("Bad offset\n");
        return SYSERR;
    }

    bbase = &dev0_blocks[block * dev0_blocksize];

    memcpy(buf, (bbase+offset), len);

    return OK;

}

int bwrite(int dev, int block, int offset, void * buf, int len) {
    char *bbase;

    if (dev != 0) {
        printf("Unsupported device\n");
        return SYSERR;
    }
    if (offset >= dev0_blocksize) {
        printf("Bad offset\n");
        return SYSERR;
    }

    bbase = &dev0_blocks[block * dev0_blocksize];

    memcpy((bbase+offset), buf, len);
  
    return OK;

}

int block_offset_write(int dev, int block, int offset, void * buf, int len)
{
    char tmp_buf[MDEV_BLOCK_SIZE];
    
    // Read inode block from store
    if (bread( dev, block, 0, tmp_buf, MDEV_BLOCK_SIZE) == SYSERR){
        return SYSERR;
    }
   
    // Put inode in the correct block 
    memcpy((tmp_buf+offset), buf, len);
  
    // Write inode block to store
    if (bwrite( dev, block, 0, tmp_buf, MDEV_BLOCK_SIZE) == SYSERR){
        return SYSERR;
    }
    
    return OK;
}

int block_offset_read(int dev, int block, int offset, void * buf, int len)
{
    char tmp_buf[MDEV_BLOCK_SIZE];
    
    // Read inode block from store
    if (bread( dev, block, 0, tmp_buf, MDEV_BLOCK_SIZE) == SYSERR){
        return SYSERR;
    }
   
    // Put inode in the correct block 
    memcpy( buf, (tmp_buf+offset), len);
  
    return OK;
}

int get_inode_by_num(int dev, int inode_number)
{
    char buf[MDEV_BLOCK_SIZE];    //Temporary buffer to hold inode block
    int free_index = -1;
    struct inode in;
    int i;
  
    if (dev != 0) 
    {
        printf("Unsupported device\n");
        return SYSERR;
    }
  
    if(inode_number < 0 || inode_number > MDEV_NUM_BLOCKS) 
    {
        printf("Invalid inode nubmer \n");
        return SYSERR;
    }
  
    // if requested inode is already in memory, return the same
    for (i=0; i < FILETABLEN; i++)
    {
        if (core_inode[i].id == inode_number)
        {
            printf("\n\rFound incore inode %d at %d", inode_number, i);
            return i;
        }
        else if ((free_index == -1) && (core_inode[i].id == UNUSED))
        {
            free_index = i;
        }
    }

    // if in core inode table is full
    if ((i == FILETABLEN) && (free_index == -1))
    {
        printf("\n\rNot enough room to bring inode %d incore.", inode_number);
        return SYSERR;
    }
    
    // Read inode block from backing store
    if (bread( dev, INODE2BLOCKNO(inode_number), 0, buf, MDEV_BLOCK_SIZE) == SYSERR)
    {
        return SYSERR;
    }
  
    // extract exact inode from inode block
    memcpy(&in, (buf+INODE2OFFSET(inode_number)), sizeof(struct inode));
    core_inode[free_index].id = in.id;
    core_inode[free_index].type = in.type;
    core_inode[free_index].nlink = in.nlink;
    core_inode[free_index].device = in.device;
    core_inode[free_index].size = in.size;
    for (i=0; i < FILEBLOCKS; i++)
    {
        core_inode[free_index].blocks[i] = in.blocks[i];
    }
     
    return free_index;
}

int put_inode_by_num(int dev, int incore_inode_index){
    
    char buf[MDEV_BLOCK_SIZE];    //Temporary buffer to hold inode block
    int i;
    struct inode in;
    
    in.id = core_inode[incore_inode_index].id;
    in.type = core_inode[incore_inode_index].type;
    in.nlink = core_inode[incore_inode_index].nlink;
    in.device = core_inode[incore_inode_index].device;
    in.size = core_inode[incore_inode_index].size;
    
    for (i=0; i < FILEBLOCKS; i++)
    {
         in.blocks[i]  = core_inode->blocks[i];
    }
    if (dev != 0) {
        printf("Unsupported device\n");
        return SYSERR;
    }
  
    if((incore_inode_index < 0) || (incore_inode_index > FILETABLEN)) {
        printf("Invalid incore inode nubmer \n");
        return SYSERR;
    }
    
    //bwrite(dev, INODE2BLOCKNO(inode_number), INODE2OFFSET(inode_number), in, sizeof(struct inode));
    
    // Read inode block from store
    if (bread( dev, INODE2BLOCKNO(in.id), 0, buf, MDEV_BLOCK_SIZE) == SYSERR){
        return SYSERR;
    }
   
    // Put inode in the correct block 
    memcpy((buf+INODE2OFFSET(in.id)), &in, sizeof(struct inode));
  
    // Write inode block to store
    if (bwrite( dev, INODE2BLOCKNO(in.id), 0, buf, MDEV_BLOCK_SIZE) == SYSERR){
        return SYSERR;
    }
    
    return OK;
}


int get_inode_by_name(char *filename)
{
    int length;
    int i;
    
    //Check filename length
    length = strnlen(filename, FILENAMELEN+1);
    if (length > FILENAMELEN)
    {  
        printf("\n\rFile name cannot be longer than %d.", FILENAMELEN); 
        return SYSERR;
    }

    //Read directory into memory
    //if(bread(0, bm_blk, 0, &fsd, sizeof(struct fsystem)) == SYSERR)
    //{
    //  return -1;
    //}
         
    //Check root directory for same filename
    for (i=0; i < DIRECTORY_SIZE; i++)
    {
        if ((fsd.root_dir.entry[i].inode_num != UNUSED) &&
        (strncmp(filename,fsd.root_dir.entry[i].name, FILENAMELEN) == 0))
        {
            return get_inode_by_num(0, i);    
        }
    }

    return SYSERR;
}

/* specify the block number to be set in the mask */
int setmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  fsd.freemask[mbyte] |= (0x80 >> mbit);
}

/* specify the block number to be read in the mask */
int getmaskbit(int b) {
  int mbyte, mbit;
  mbyte = b / 8;
  mbit = b % 8;

  return( ( (fsd.freemask[mbyte] << mbit) & 0x80 ) >> 7);
}

/* specify the block number to be unset in the mask */
int clearmaskbit(int b) {
  int mbyte, mbit, invb;
  mbyte = b / 8;
  mbit = b % 8;

  invb = ~(0x80 >> mbit);
  invb &= 0xFF;

  fsd.freemask[mbyte] &= invb;
}

/* This is maybe a little overcomplicated since the first block is indicated in the
   high-order bit.  Shift the byte by j positions to make the match in bit7 (the 8th 
   bit) and then shift that value 7 times to the low-order bit to print.  Yes, it
   could be the other way...  */
void printfreemask(void) {
  int i,j;

  for (i=0; i < fsd.freemaskbytes; i++) {
    for (j=0; j < 8; j++) {
      printf("%d", ((fsd.freemask[i] << j) & 0x80) >> 7);
    }
    if ( (i % 8) == 7) {
      printf("\n");
    }
  }
  printf("\n");
}

int allocate_next_free_block ()
{
    int i = 0;
  	for (i=0; getmaskbit(i) == 1 && i<MDEV_NUM_BLOCKS; i++);
  	if(i==MDEV_NUM_BLOCKS)
  	{
  	  printf("\n\rNo more free blocks in filesystem.");
  	  return SYSERR;
  	}
  	setmaskbit(i);
  	return i;
}

int allocate_block(int incore_inode_index, int datablock_no)
{
    int no_blocks_1_indirection = MDEV_BLOCK_SIZE / sizeof(int);
    int no_blocks_2_indirection = (MDEV_BLOCK_SIZE / sizeof(int)) * (MDEV_BLOCK_SIZE / sizeof(int));
    int first_level_datablock_no;
    int second_level_datablock_no;

    int tmp_buf[no_blocks_1_indirection];
    int tmp_buf2[no_blocks_1_indirection];
    int i, allocated_block_no;
    
    // direct block pointer 
    if (datablock_no < 10)
		{
		    if(core_inode[incore_inode_index].blocks[datablock_no] == UNUSED)
			  {
			      return (core_inode[incore_inode_index].blocks[datablock_no] = allocate_next_free_block()) ;
			  }
			 
		}
		
		// first level indirection pointers
		else if (datablock_no < (10 + no_blocks_1_indirection)) 	
		{
			  if(core_inode[incore_inode_index].blocks[10] == UNUSED)
			  {
			      allocated_block_no = allocate_next_free_block();
			      if (allocated_block_no == SYSERR)
			      {
			        return SYSERR;
			      }
			     core_inode[incore_inode_index].blocks[10] = allocated_block_no;
			     
			     //bread(0, core_inode[incore_inode_index].blocks[10], 0, tmp_buf, MDEV_BLOCK_SIZE);
			     for (i=0; i < no_blocks_1_indirection; i++)
			     {
			        tmp_buf[i] = UNUSED;
			     }
			     bwrite(0, core_inode[incore_inode_index].blocks[10], 0, tmp_buf, MDEV_BLOCK_SIZE);
			  }
				
				bread(0, core_inode[incore_inode_index].blocks[10], 0, tmp_buf, MDEV_BLOCK_SIZE);
				allocated_block_no = allocate_next_free_block();
			      if (allocated_block_no == SYSERR)
			      {
			        return SYSERR;
			      }
    		tmp_buf[datablock_no-10] = allocated_block_no;
    		bwrite(0, core_inode[incore_inode_index].blocks[10], 0, tmp_buf, MDEV_BLOCK_SIZE);
    		
			  return (tmp_buf[datablock_no-10]);
		}
		
		//second level indirection allocation
		else if(datablock_no < (10 + no_blocks_1_indirection + no_blocks_2_indirection))
		{
		    if(core_inode[incore_inode_index].blocks[11] == UNUSED)
			  {
			      allocated_block_no = allocate_next_free_block();
			      if (allocated_block_no == SYSERR)
			      {
			        return SYSERR;
			      }
			     core_inode[incore_inode_index].blocks[11] = allocated_block_no;
			     //bread(0, core_inode[incore_inode_index].blocks[10], 0, tmp_buf, MDEV_BLOCK_SIZE);
			     for (i=0; i < no_blocks_1_indirection; i++)
			     {
			        tmp_buf[i] = UNUSED;
			     }
			     bwrite(0, core_inode[incore_inode_index].blocks[11], 0, tmp_buf, MDEV_BLOCK_SIZE);
			  }
			  
			  bread(0, core_inode[incore_inode_index].blocks[11], 0, tmp_buf, MDEV_BLOCK_SIZE);
			  
			  // calculate first level datablock no. to read
			  first_level_datablock_no = (datablock_no - (10 + no_blocks_1_indirection)) / no_blocks_1_indirection ;
			  //printf("\n\r First level data block no is : %d",first_level_datablock_no);
			  // check first level for UNUSED
			  if(tmp_buf[first_level_datablock_no] == UNUSED)
			  { 
			      allocated_block_no = allocate_next_free_block();
			      if (allocated_block_no == SYSERR)
			      {
			        return SYSERR;
			      }
			      tmp_buf[first_level_datablock_no] = allocated_block_no;
			      //bread(0, core_inode[incore_inode_index].blocks[10], 0, tmp_buf, MDEV_BLOCK_SIZE);
			      for (i=0; i < no_blocks_1_indirection; i++)
			      {
			         tmp_buf2[i] = UNUSED;
			      }
			      
			      // update previous block 
			      bwrite(0, core_inode[incore_inode_index].blocks[11], 0, tmp_buf, MDEV_BLOCK_SIZE);
			      
			      // Save UNUSED initialized block
			      bwrite(0, tmp_buf[first_level_datablock_no], 0, tmp_buf2, MDEV_BLOCK_SIZE);
			  }
			  
			  // read second level block
			  bread(0, tmp_buf[first_level_datablock_no], 0, tmp_buf2, MDEV_BLOCK_SIZE);
			   
			  // calculate second level block number
			  second_level_datablock_no = (datablock_no - (10 + no_blocks_1_indirection)) % no_blocks_1_indirection;
			  
			  // check second level for UNUSED
			  allocated_block_no = allocate_next_free_block();
			      if (allocated_block_no == SYSERR)
			      {
			        return SYSERR;
			      }
			  tmp_buf2[second_level_datablock_no] = allocated_block_no;
			  bwrite(0, tmp_buf[first_level_datablock_no], 0, tmp_buf2, MDEV_BLOCK_SIZE);
			  
			  return tmp_buf2[second_level_datablock_no]; 
		}
		return SYSERR;
}

int block_no_from_datablock_no(int incore_inode_index, int datablock_no)
{
    int no_blocks_1_indirection = (MDEV_BLOCK_SIZE / sizeof(int));
    int no_blocks_2_indirection = (MDEV_BLOCK_SIZE / sizeof(int)) * (MDEV_BLOCK_SIZE / sizeof(int));
    int first_level_datablock_no;
    int second_level_datablock_no;
    int tmp_buf[no_blocks_1_indirection];
    
    // direct block pointer 
    if (datablock_no < 10)
		{
		    if(core_inode[incore_inode_index].blocks[datablock_no] == UNUSED)
			  {
			      //printf("\n\r Data block is UNUSED");
			      return UNUSED;
			  }
			  else
			  {		
			      return core_inode[incore_inode_index].blocks[datablock_no];
			  }
		}
		
		// first level indirection pointers
		else if (datablock_no < (10 + no_blocks_1_indirection)) 	
		{
			  if(core_inode[incore_inode_index].blocks[10] == UNUSED)
			  {
			      //printf("\n\r Data block is UNUSED");
			      return UNUSED;
			  }else
			  {
			      bread(0, core_inode[incore_inode_index].blocks[10], 0, tmp_buf, MDEV_BLOCK_SIZE);
			      if(tmp_buf[datablock_no-10] == UNUSED)
			      {
			          return UNUSED;
			      }
			      else
			      {
			          return tmp_buf[datablock_no-10];
			      }
			  }		
		}
		// second level indirection pointers
		else if(datablock_no < (10 + no_blocks_1_indirection + no_blocks_2_indirection))
		{
		    if(core_inode[incore_inode_index].blocks[11] == UNUSED)
			  {
			      //printf("\n\r Data block is UNUSED 3");
			      return UNUSED;
			  }
			  
			  bread(0, core_inode[incore_inode_index].blocks[11], 0, tmp_buf, MDEV_BLOCK_SIZE);
			  
			  // calculate first level datablock no. to read
			  first_level_datablock_no = (datablock_no - (10 + no_blocks_1_indirection)) / no_blocks_1_indirection ;
			  
			  // check first level for UNUSED
			  if(tmp_buf[first_level_datablock_no] == UNUSED)
			  { 
			      //printf("\n\r Data block is UNUSED 2");
			      return UNUSED;
			  }
			  
			  // read second level block
			  bread(0, tmp_buf[first_level_datablock_no], 0, tmp_buf, MDEV_BLOCK_SIZE);
			   
			  // calculate second level block number
			  second_level_datablock_no = (datablock_no - (10 + no_blocks_1_indirection)) % no_blocks_1_indirection;
			  
			  // check first level for UNUSED
			  if(tmp_buf[second_level_datablock_no] == UNUSED)
			  { 
			      //printf("\n\r Data block is UNUSED 1");
			      return UNUSED;
			  }
			  else
			  {
			        return tmp_buf[second_level_datablock_no];
			  }
			  
		}
		return SYSERR;
}

#endif /* FS */
