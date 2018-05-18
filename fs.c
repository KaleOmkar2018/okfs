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

/*### DRIVER OPERTAIONS ###*/
struct dentry* okfs_lookup(struct inode* parent_inode, struct dentry* child_dentry, unsigned int flags);
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

static const struct file_operations okfs_file_operations = {
	.owner = THIS_MODULE,
};

static struct inode_operations okfs_inode_ops = {
	.lookup = okfs_lookup,	
};

/* #################### OKFS_LOOKUP ####################### */
struct dentry* okfs_lookup(struct inode* parent_inode, struct dentry* child_dentry, unsigned int flags) 
{
	int i;
	struct buffer_head* bh;
	struct okfs_dir_record* record;
	static int done = 0; 
	
	okfs_inode_store* parent = parent_inode->i_private; //Pointer to the okfs_inode structure of the parent directory inode
	struct super_block* sb = parent_inode->i_sb; 		//This the VFS superblock structure
	
	//Check whether lookup is called only once for a child dentry
	if(done)
	{
		done = 0;
		return NULL;
	}
	
	//Check if parent inode is not NULL
	if(!parent)
	{
		printk(KERN_ALERT "Parent Inode NULL\n");
		return NULL;
	}
	
	//Each directory inode will store information about the files or directories in its data block.
	//Copy the entire buffer_head required for data of data block of parent_inode from disk.
	bh = sb_bread(sb, parent->data_block_number);
	if(!bh)
	{
		printk(KERN_ALERT "Cannot obtain data from disk\n");
		return NULL;
	}
	
	//Each data block for a file will store direcotry records : filename + inode
	record = (struct okfs_dir_record*)bh->b_data;
	for(i = 0; i < parent->dir_children_count; i++)
	{
		//We will get a negative dentry, we have a filename but we do not have an inode attached to the dentry.
		//Compare the filename and create an in-core inode and add the dentry.
		if(!strcmp(record->filename, child_dentry->d_name.name))
		{
			printk(KERN_ALERT "Match Found\n");
			struct inode* inode;
			okfs_inode_store* okfs_inode;

			//Assuming that a new inode will be present inside the root directory, we will get the inode number for that file.
			okfs_inode = okfs_get_root_dir_inode(sb, record->inode_no);
			if(!okfs_inode)
			{
				printk(KERN_ALERT "Record for requested inode not present\n");
				return NULL;
			}
			//Create an in-core inode and initialize it.
			inode = new_inode(sb);
			inode->i_ino = okfs_inode->inode_no;
			//Sets the parent directory as the ownwer and the mode depending upon whether it is a file or directory.
			inode_init_owner(inode, parent_inode, okfs_inode->mode);
			inode->i_sb = sb;
			//While accessing this inode, use the operations mentioned in below structure.
			inode->i_op = &okfs_inode_ops;
			
			//Check whether it is a file or directory to assign file or directory operations.
			if(S_ISDIR(inode->i_mode))
			{
				inode->i_fop = &okfs_dir_operations;
				printk(KERN_ALERT "Directory Operations\n");
			}
			else if(S_ISREG(inode->i_mode))
			{
				inode->i_fop = &okfs_file_operations;
				printk(KERN_ALERT "File Operations\n");
			}
			else
			{
				printk(KERN_ALERT "Mode Not Supported\n");
				return NULL;
			}
			
			inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
			//Store the filesystem structure so that it is easily accessible.
			inode->i_private = okfs_inode;
			//Now that we have an initialized inode, make a pair.
			d_add(child_dentry, inode);
			done = 1;
			return child_dentry;
		}
		record++;
	}	
	brelse(bh);
	return NULL;
}

/* #################### OKFS_ITERATE ####################### */
static int okfs_iterate(struct file *filp, struct dir_context *dir)
{
	int i;
	struct buffer_head* bh;
	okfs_inode_store* okfs_dir_inode;
	struct okfs_dir_record* record;
	struct inode* inode = filp->f_inode; //Take the directory inode
	struct super_block* sb = inode->i_sb;//And the superblock
	
	//Once directory is iterated, position will be updated as the kernel will know about the directory layout.	
	if(dir->pos > 0)
	{
		printk(KERN_ALERT "Directory Contents Read\n");
		return 0; //return 0 as contents have already been read and it is not an error.
	}
	
	okfs_dir_inode = (okfs_inode_store*)inode->i_private;
	if(!okfs_dir_inode)
	{
		printk(KERN_ALERT "Inode is Null\n");
		return -1;//Return value not checked
	}
	if(!S_ISDIR(okfs_dir_inode->mode))
	{
		printk(KERN_ALERT "Inode is not a directory\n");
		return -1; //Return Value Not Checked
	}
	
	bh = sb_bread(sb, okfs_dir_inode->data_block_number);
	record = (struct okfs_dir_record*)bh->b_data;
	
	//Iterate over directory children and let the kernel know about the directory layout.
	for(i = 0; i < okfs_dir_inode->dir_children_count; i++)
	{
		dir_emit(dir, record->filename, OKFS_FILENAME_MAXLEN, record->inode_no, DT_UNKNOWN);
		dir->pos += sizeof(record);
		record++;
	}
	
	//Release the buffer
	brelse(bh);
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
