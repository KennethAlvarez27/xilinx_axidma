/**
 * @file axidma_chrdev.c
 * @date Saturday, November 14, 2015 at 12:03:13 PM EST
 * @author Brandon Perez (bmperez)
 * @author Jared Choi (jaewonch)
 *
 * This file contains the implementation of the character device for the AXI DMA
 * module.
 *
 * @bug No known bugs.
 **/

// Kernel dependencies
#include <linux/device.h>       // Device and class creation functions
#include <linux/cdev.h>         // Character device functions
#include <linux/fs.h>           // File operations and file types
#include <linux/mm.h>           // Memory types and remapping functions

// Local dependencies
#include "axidma.h"             // Local definitions

// TODO: Maybe this can be improved?
static struct axidma_device *axidma_dev;

/*----------------------------------------------------------------------------
 * File Operations
 *----------------------------------------------------------------------------*/

static int axidma_open(struct inode *inode, struct file *file)
{
    // Only the root user can open this device, and it must be exclusive
    if (!capable(CAP_SYS_ADMIN)) {
        axidma_err("Only root can open this device.");
        return -EPERM;
    } else if (!(file->f_flags & O_EXCL)) {
        axidma_err("O_EXCL must be specified for open()\n");
        return -EINVAL;
    }

    // Place the axidma structure in the private data of the file
    file->private_data = (void *)axidma_dev;
    return 0;
}

static int axidma_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;
    return 0;
}

static int axidma_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long alloc_size;
    void *addr;
    int rc;

    // Determine the allocation size, and set the region as uncached
    alloc_size = vma->vm_end - vma->vm_start;
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    // Allocate the requested region
    // TODO:
    //axidma_malloc();
    addr = axidma_dev->dma_base_vaddr;

    // Map the region into userspace
    rc = remap_pfn_range(vma, vma->vm_start, __phys_to_pfn(virt_to_phys(addr)),
                         alloc_size, vma->vm_page_prot);
    if (rc < 0) {
        axidma_err("Unable to allocate address 0x%08lx, size %lu",
                   vma->vm_start, alloc_size);
        return rc;
    }

    return 0;
}

// The file operations for the AXI DMA device
static const struct file_operations axidma_fops = {
    .owner = THIS_MODULE,
    .open = axidma_open,
    .release = axidma_release,
    .mmap = axidma_mmap,
};

/*----------------------------------------------------------------------------
 * Initialization and Cleanup
 *----------------------------------------------------------------------------*/

int axidma_chrdev_init(struct axidma_device *dev)
{
    int rc;

    // Store a global pointer to the axidma device
    axidma_dev = dev;

    // Allocate a major and minor number region for the character device
    rc = alloc_chrdev_region(&dev->dev_num, dev->minor_num, dev->num_devices,
                             dev->chrdev_name);
    if (rc < 0) {
        axidma_err("Unable to allocate character device region.\n");
        goto ret;
    }

    // Create a device class for our device
    dev->dev_class = class_create(THIS_MODULE, dev->chrdev_name);
    if (IS_ERR(dev->dev_class)) {
        axidma_err("Unable to create a device class.\n");
        rc = PTR_ERR(dev->dev_class);
        goto free_chrdev_region;
    }

    /* Create a device for our module. This will create a file on the
     * filesystem, under "/dev/dev->chrdev_name". */
    dev->device = device_create(dev->dev_class, NULL, dev->dev_num, NULL,
                                dev->chrdev_name);
    if (IS_ERR(dev->device)) {
        axidma_err("Unable to create a device.\n");
        rc = PTR_ERR(dev->device);
        goto class_cleanup;
    }

    // Register our character device with the kernel
    cdev_init(&dev->chrdev, &axidma_fops);
    rc = cdev_add(&dev->chrdev, dev->dev_num, dev->num_devices);
    if (rc < 0) {
        axidma_err("Unable to add a character device.\n");
        goto device_cleanup;
    }

    return 0;

device_cleanup:
    device_destroy(dev->dev_class, dev->dev_num);
class_cleanup:
    class_destroy(dev->dev_class);
free_chrdev_region:
    unregister_chrdev_region(dev->dev_num, dev->num_devices);
ret:
    return rc;
}

void axidma_chrdev_exit(struct axidma_device *dev)
{
    // Cleanup all related character device structures
    cdev_del(&dev->chrdev);
    device_destroy(dev->dev_class, dev->dev_num);
    class_destroy(dev->dev_class);
    unregister_chrdev_region(dev->dev_num, dev->num_devices);

    return;
}
