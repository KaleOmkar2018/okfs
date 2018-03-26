#include<linux/init.h>
#include<linux/module.h> 

static int okfsInit(void)
{
	int ret = 0;
	printk(KERN_INFO "Hello World : Lets Begin\n");
 	return ret;
}

static void okfsExit(void)
{
	printk(KERN_ALERT "GoodBye World\n");
}

module_init(okfsInit); 
module_exit(okfsExit); 

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OK");
