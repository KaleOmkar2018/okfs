#include<linux/init.h>
#include<linux/module.h> 
#include<linux/fs.h>
#include<linux/buffer_head.h>
#include<linux/slab.h>

#include "fs.h"

/*##################### MACROS ################ */
#define okfs_flag  FS_REQUIRES_DEV //This filesystem supports only block devices.

/*##################### FUNCTION DECLARATIONS ####################### */
/*### HELPER FUNCTIONS ###*/
static int okfs_superfill_callback(struct super_block* sb, void* data, int silent);
static okfs_inode_store* okfs_get_root_dir_inode(struct super_block* sb, unsigned long inode_no);

/*##################### DRIVER OPERTAIONS ########################## */
static struct dentry* okfs_lookup(struct inode* parent_inode, struct dentry* child_dentry, unsigned int flags);
static int okfs_iterate(struct file *filp, struct dir_context *dir);
static void okfs_kill_superblock(struct super_block* sb);
static struct dentry* okfs_mount(struct file_system_type *fs_type, int flags, const char* dev_name, void* data);
static int okfsInit(void);
static void okfsExit(void);

/*##################### FILE_SYSTEM_STRUCTURES ###################### */
struct file_system_type okfs_type = {
	.owner 	  = THIS_MODULE,
	.name     = "okfs", 
	.mount    = okfs_mount, 
	.kill_sb  = okfs_kill_superblock,
	.fs_flags = okfs_flag,
	.next	  = NULL,
};

static const struct file_operations okfs_dir_operations = {
	.owner   = THIS_MODULE,
	.iterate = okfs_iterate,
};

static struct inode_operations okfs_inode_ops = {
	.lookup = okfs_lookup,	
};

/* #################### OKFS_LOOKUP ####################### */

struct dentry* okfs_lookup(struct inode* parent_inode, struct dentry* child_dentry, unsigned int flags) 
{
	//not added : Should create an in-core inode corresponding to required dentry, associate it with child_dentry and return the dentry.
	return NULL;
}

/* #################### OKFS_ITERATE ####################### */
static int okfs_iterate(struct file *filp, struct dir_context *dir)
{
	//Not added : New method to retrieve directory contents
	return 0;
}

/* #################### OKFS_KILL_SUPERBLOCK ####################### */
static void okfs_kill_superblock(struct super_block* sb)
{
	printk(KERN_ALERT "Kill Superblock called, now unmounting works\n");
	return;	
}
/* #################### OKFS_GET_ROOT_DIR_INODE ####################### */
static okfs_inode_store* okfs_get_root_dir_inode(struct super_block* sb, unsigned long inode_no)
{
	okfs_inode_store* iter_store  = NULL;
	okfs_inode_store* found_inode = NULL;

	struct buffer_head* bh = NULL;
	
	int iterator = 0;

	//Copy the entire inode store block into buffer_head structure.
	bh = sb_bread(sb, OKFS_INODE_STORE_NUMBER);
	if(!bh)
	{
		printk(KERN_ALERT "Could not read data from inode store on disk into buffer head\n");
		return NULL; 
	}

	printk(KERN_ALERT "Data read into buffer head\n");

	//Now, we copy the data for iteration.
	iter_store = (okfs_inode_store*)bh->b_data;
	if(!iter_store)
	{
		printk(KERN_ALERT "Could not copy data from buffer into our structure\n");
		return NULL;
	}
	
	printk(KERN_ALERT "Data read into iter_store structure\n");
	printk(KERN_ALERT "inode_count: [%lu]\n", iter_store->value);

	//Application does not increment the inode count, increment the inode count.
	if(!iter_store->value)
	{
		iter_store->value = 1; //Make it one if not done already.
	}

	iter_store++;//Point to first inode
	for(iterator = 1; iterator <= OKFS_MAX_INODES; iterator++)
	{
		if(iter_store->inode_no == inode_no)
		{
			found_inode = kmalloc(sizeof(okfs_inode_store), GFP_KERNEL);
			break;
		}
	}
	return found_inode;
}
/* #################### OKFS_SUPERFILL_CALLBACK ############## */
static int okfs_superfill_callback(struct super_block* sb, void* data, int silent)
{
	struct inode* root_inode;
	struct buffer_head* bh; 
	struct okfs_super_block* sb_disk; 
 
 	//Retrieve the superblock structure from disk/device into a buffer head structure : copies maximum one page of data.
 	bh = sb_bread(sb, OKFS_SUPERBLOCK_NUMBER);

 	//Copy the device superblock contents into memory 
	sb_disk = (struct okfs_super_block*) bh->b_data;

	printk(KERN_ALERT "Magic Number: [%lu]\n", sb_disk->magic);
	printk(KERN_ALERT "Start Filling The Superblock\n");
	sb->s_magic = OKFS_MAGIC_NUMBER;

	//Store the structure so that it can be easily accessible
	sb->s_fs_info = sb_disk;
	
	printk(KERN_ALERT "Start Filling The root inode\n");
	//Create a new inode	
	root_inode = new_inode(sb);
	root_inode->i_ino = OKFS_ROOTDIR_INODE_NUMBER;
	inode_init_owner(root_inode, NULL, S_IFDIR);
	root_inode->i_sb = sb;
	root_inode->i_op = &okfs_inode_ops;

	//The first inode is a directory. So, we need to assign directory operations structure.
	root_inode->i_fop = &okfs_dir_operations;
	root_inode->i_atime = root_inode->i_mtime = root_inode->i_ctime = current_time(root_inode);
	
	//Store the okfs_inode structure in i_private for easy accessibility.
	root_inode->i_private = okfs_get_root_dir_inode(sb, OKFS_ROOTDIR_INODE_NUMBER); //1
	
	//Allocates a root dentry("/").
	sb->s_root = d_make_root(root_inode); 
	if(!sb->s_root)
	{
		return -ENOMEM;
	}
	printk(KERN_ALERT "Superblock Filled\n");

	//Job Done. Release the Buffer.
	brelse(bh);
	printk(KERN_ALERT "Buffer Released\n");

	return 0;
}
/* #################### OKFS_MOUNT ########################### */
static struct dentry* okfs_mount(struct file_system_type* fs_type, int flags, const char* dev_name, void* data)
{
	struct dentry* ret;
	//Mount a filesystem on a block device and use okfs_supergill_callback
	ret = mount_bdev(fs_type, flags, dev_name, data, okfs_superfill_callback); 

 	if(unlikely(IS_ERR(ret)))
 	{
  		printk(KERN_ALERT "Error Mounting okfs\n");
 	}	
 	else
 	{
  		printk(KERN_ALERT "okfs succesfully mounted on [%s]\n", dev_name);
  	}
 	return ret;
}
/* #################### OKFS_INIT ########################## */
static int okfsInit(void)
{
	int ret = 0;

	ret = register_filesystem(&okfs_type);
	if(likely(ret == 0))
	{
		printk(KERN_ALERT "Successfully registered okfs\n");
	}
	else
    {
		printk(KERN_ALERT "Failed to register okfs. Error : [%d]", ret);
    }
 
 	return ret;
}

/* #################### OKFS_EXIT ########################## */
static void okfsExit(void)
{
	int ret = 0;
	
	ret = unregister_filesystem(&okfs_type);
	if(likely(ret == 0))
	{
		printk(KERN_ALERT "Successfully unregistered okfs\n");
	}
	else
	{
		printk(KERN_ALERT "Failed to register okfs. Error : [%d]", ret);
	}
}

module_init(okfsInit); 
module_exit(okfsExit); 

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OK");
