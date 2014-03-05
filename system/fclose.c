#include <fs.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <thread.h>

int fclose(int fd)
{
    struct thrent *thrptr;
    int openftab_index = -1;
    int cinode_index   = -1;
    int i;
    
    if ((fd < 3) || (fd > NDESC))
    {
        printf("\n\rInvalid file descriptor.");
        return EOF;
    }

    /* File descriptor to openfiletable index */
    thrptr = &thrtab[thrcurrent];
    if (thrptr->fdesc[fd] == UNUSED)
    {
        printf("\n\rFile descriptor %d not already in use.", fd);
        return EOF;
    }
    else
    {
        openftab_index = thrptr->fdesc[fd];
        thrptr->fdesc[fd] = UNUSED;
    }
    
    /* openfiletable index to core inode */
    if(openfiletable[openftab_index].state == UNUSED)
    {
        printf("\n\rOpen file table entry not found");
        return EOF;
    }else
    {
        openfiletable[openftab_index].state = UNUSED;
        openfiletable[openftab_index].mode = -1;
        openfiletable[openftab_index].fptr = 0;
        cinode_index = openfiletable[openftab_index].in_core_inode_num;
        openfiletable[openftab_index].in_core_inode_num = UNUSED;
    }
        
    if(core_inode[cinode_index].id == UNUSED)
    {
        printf("\n\rCore Inode table entry not found");
        return EOF;
    }
    else
    {
        core_inode[cinode_index].ref_count--;
        if(0 == core_inode[cinode_index].ref_count)
        {
            /* Write core inode back to disk */
            put_inode_by_num(0, cinode_index);
        
            /* Evacuate the Core inode for future use*/
            core_inode[cinode_index].id         = UNUSED ;
            core_inode[cinode_index].state      = UNLOCKED;
            core_inode[cinode_index].ref_count  = 0;
	          core_inode[cinode_index].type       = 0;
            core_inode[cinode_index].nlink      = 0;
            core_inode[cinode_index].device     = 0;
            core_inode[cinode_index].size       = 0;
        
            for(i=0 ;i<FILEBLOCKS; i++)
	          {
                core_inode[cinode_index].blocks[i] = UNUSED;
	          }
	      }
    }
    return OK;
}
