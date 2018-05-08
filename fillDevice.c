/*######### HOW OUR DEVICE WILL LOOK LIKE #########
  Block 0 : Superblock structure(internal padding used)
  Block 1 : Inode Store -> inode count + inodes in okfs_inode_structure.
  Block 2 : Not Added but this will store okfs_dir_record.
#################################################*/

#include<unistd.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdint.h>
#include<string.h>
#include<stdlib.h>

#include "fs.h"

int main(int argc, char* argv[])
{
	int fd; 		//File Descriptor
	int nbytes; 	//Bytes received
	ssize_t ret;	

	struct okfs_super_block sb;			//Store super block		 
	okfs_inode_store store_maintenance;	//Inode Count Maintenance
	okfs_inode_store root_inode;		//Root Inode

	char* block_padding = NULL;
	
	if(argc != 2) //needs one argument other than the name of the program
	{
			printf("Need parameters\n");
			return -1;
	}
	
	fd = open(argv[1], O_RDWR); //open a device file which is passed as the argument.
	if(fd == -1)
	{
		printf("Error opening our device\n");
		return -1;
	}
	
	//Superblock
	sb.version = 1;
	sb.magic = OKFS_MAGIC_NUMBER;
	sb.block_size = OKFS_BLOCK_SIZE; 
	sb.free_blocks = OKFS_MAX_BLOCKS - 3; //3 -> superblock, inode store, rootdir data block.

	//Fill the inode store count structure
	store_maintenance.value = 1; //root directory inode

	//Fill the root directory inode with required data and store it in block number 2.
	root_inode.inode_no = OKFS_ROOTDIR_INODE_NUMBER; //1
	root_inode.data_block_number = OKFS_ROOTDIR_DATABLOCK_NUMBER; //2
	root_inode.mode = S_IFDIR; //Root Directory
	root_inode.dir_children_count = 0; //Empty root directory
		
	//Write superblock
	ret = write(fd, (char*)&sb, sizeof(sb));
	if(ret != OKFS_BLOCK_SIZE)
	{
	 	printf("Bytes written [%d] are not equal to okfs block size\n", (int)ret);
	}
	else
 	{
 		printf("Super block written of size : [%d]\n", (int)ret);
 	}
 	
 	//Inode Store Maintenance
 	ret = write(fd, (char*)&store_maintenance, sizeof(store_maintenance));
 	if(ret != -1)
 	{
 		printf("Inode Count Structure Added of size : [%d]\n", (int)ret);
 	}

 	//Add Root Inode to Device
 	ret = write(fd, (char*)&root_inode, sizeof(root_inode));
 	if(ret != -1)
 	{
 		printf("Root directory inode written succesfully of size : [%d]\n", (int)ret);
 	}
 	
	nbytes = (OKFS_BLOCK_SIZE - sizeof(store_maintenance) - sizeof(root_inode));
	block_padding = malloc(nbytes); 
	
	//Inode Store Padding
 	ret = write(fd, block_padding, nbytes);
 	if(ret == -1)
 	{
 		printf("Only [%d] bytes were written\n", (int)ret);
 	}
 	else
 	{
 		printf("Inode Store Padding of size [%d] added\n", (int)ret);
 	}
 
  	if(block_padding)
  	{
  		free(block_padding);
  	}

  	close(fd); //close the file descriptor
  	return 0;
}

