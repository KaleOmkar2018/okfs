#include<linux/init.h>
#include<linux/module.h> 
#include<linux/fs.h>

struct file_system_type okfs_fs_type = {
	.owner 	 = THIS_MODULE,
	.name    = "okfs", 
};

static int okfsInit(void)
{
	int ret = 0;

	ret = register_filesystem(&okfs_fs_type);
	if(likely(ret == 0))
	{
		printk(KERN_ALERT "Successfully registered okfs\n");
	}
	else
    {
		printk(KERN_ERR "Failed to register okfs. Error : [%d]", ret);
    }
 
 	return ret;
}

static void okfsExit(void)
{
	int ret = 0;
	
	ret = unregister_filesystem(&okfs_fs_type);
	if(likely(ret == 0))
	{
		printk(KERN_ALERT "Successfully unregistered okfs\n");
	}
	else
	{
		printk(KERN_ERR "Failed to register okfs. Error : [%d]", ret);
	}
}

module_init(okfsInit); 
module_exit(okfsExit); 

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OK");
