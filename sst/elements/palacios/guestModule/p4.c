//#include <linux/config.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#ifdef MODVERSIONS
#  include <linux/modversions.h>
#endif
#include <linux/io.h>

/* character device structures */
static dev_t p4_dev;
static struct cdev p4_cdev;

/* methods of the character device */
static int p4_open(struct inode *inode, struct file *filp);
static int p4_release(struct inode *inode, struct file *filp);
static int p4_mmap(struct file *filp, struct vm_area_struct *vma);

/* the file operations, i.e. all character device methods */
static struct file_operations p4_fops = {
        .open = p4_open,
        .release = p4_release,
        .mmap = p4_mmap,
        .owner = THIS_MODULE,
};

/* character device open method */
static int p4_open(struct inode *inode, struct file *filp)
{
        return 0;
}
/* character device last close method */
static int p4_release(struct inode *inode, struct file *filp)
{
        return 0;
}

/* character device p4 method */
static int p4_mmap(struct file *filp, struct vm_area_struct *vma)
{
#if 0
    printk("%s() %#lx %#lx\n",__func__,
                    vma->vm_start,vma->vm_end - vma->vm_start);
#endif
    if ( remap_pfn_range( vma, vma->vm_start, 0xfffff000>>PAGE_SHIFT, 
                    vma->vm_end - vma->vm_start, vma->vm_page_prot ) ) {
        return -EAGAIN;
    }
    return 0;
}

/* module initialization - called at module load time */
static int __init p4_init(void)
{
        int ret = 0;

        /* get the major number of the character device */
        if ((ret = alloc_chrdev_region(&p4_dev, 0, 1, "p4")) < 0) {
                printk(KERN_ERR "could not allocate major number for p4\n");
                goto out;
        }
        /* initialize the device structure and register the device with the kernel */
        cdev_init(&p4_cdev, &p4_fops);
        if ((ret = cdev_add(&p4_cdev, p4_dev, 1)) < 0) {
                printk(KERN_ERR "could not allocate chrdev for p4\n");
                goto out_unalloc_region;
        }

        return ret;
        
  out_unalloc_region:
        unregister_chrdev_region(p4_dev, 1);
  out:
        return ret;
}

/* module unload */
static void __exit p4_exit(void)
{
        /* remove the character deivce */
        cdev_del(&p4_cdev);
        unregister_chrdev_region(p4_dev, 1);
}

module_init(p4_init);
module_exit(p4_exit);
MODULE_DESCRIPTION("P4 demo driver");
MODULE_AUTHOR("Michael Levenhagen");
MODULE_LICENSE("BSD");

