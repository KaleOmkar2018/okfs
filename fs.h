#ifndef FS_H
#define FS_H

#include<linux/types.h>

//Contains Useful Numbers used in driver and application.
enum FS_MISC_NUMBERS{
	OKFS_BLOCK_SIZE 	  = 4096,
	OKFS_MAGIC_NUMBER 	  = 0x12345678,
	OKFS_VERSION 		  = 1,
	OKFS_FILENAME_MAXLEN  = 255,
	OKFS_INODE_SIZE 	  = 32,
	OKFS_INODE_STORE_SIZE = 8,
};

//Default Data Blocks and Block information in our filesystem.
enum BLOCK_NUMBERS{
	OKFS_SUPERBLOCK_NUMBER,
	OKFS_INODE_STORE_NUMBER,
	OKFS_ROOTDIR_DATABLOCK_NUMBER,
	OKFS_DEFAULT_FILLED_BLOCKS = 3,
	OKFS_MAX_BLOCKS = 64, //4096 / 32 = 128
};

//Default Inodes in our filesystem
enum INODE_NUMBERS{
	OKFS_ROOTDIR_INODE_NUMBER = 1,
	OKFS_MAX_INODES = OKFS_DEFAULT_FILLED_BLOCKS - OKFS_MAX_BLOCKS, //61
};

//inode store structure
typedef union okfs_inodeStore_datablock{
//Use it as an inode
struct{
	mode_t mode;		
	unsigned long inode_no;
	unsigned long data_block_number;
	union{
		unsigned long file_size;
		unsigned long dir_children_count;
	};
}; 
//Use it as an inode store
struct{
	unsigned long value;
	char padding[OKFS_INODE_SIZE - OKFS_INODE_STORE_SIZE]; //Adding parameters to above structure will need changes here.
};

}okfs_inode_store;

//Superblock structure
struct okfs_super_block{
	unsigned long version;
	unsigned long magic;
	unsigned long block_size;
	unsigned long free_blocks;
	char padding[ (OKFS_BLOCK_SIZE - (4 * sizeof(unsigned long))) ];
};

#endif