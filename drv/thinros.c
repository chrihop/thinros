#include <linux/init.h>
#include <linux/module.h>
// ---
#include <stdarg.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/stddef.h>
#include <linux/gfp.h>
#include <linux/atomic.h>

#include "lib/thinros_core.h"
#include "thinros_config.h"

#define DRIVER_AUTHOR	"Hao Chen <hao.chen@yale.edu>"
#define DRIVER_DESC		"Thin-ROS driver for Linux"

#define DEVID	"thinros"

#define KDEBUG

#ifdef KDEBUG

#warning "build debug version!"

#define debug(FMT, ...)		pr_info(DEVID ": " FMT, ##__VA_ARGS__)
#else

#define debug(FMT, ...)
#endif

#define info(FMT, ...)		pr_info(DEVID ": " FMT, ##__VA_ARGS__)
#define panic(FMT, ...)		do { pr_alert(DEVID ": " FMT, ##__VA_ARGS__); dump_stack(); } while(0)
#define assert(x)		do { if (unlikely(!(x))) { panic("`%s` not satisfied.\n", #x); } } while (0)


#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0)

typedef int			vm_fault_t;
typedef struct file_operations	proc_ops_t;
#define proc_ops_init(x, _open, _read, _write, _release, _mmap) \
	proc_ops_t x = { \
		.owner   = THIS_MODULE, \
		.open    = _open, \
		.read    = _read, \
		.write   = _write, \
		.release = _release, \
		.mmap    = _mmap, \
	}

#else

typedef struct proc_ops		proc_ops_t;
#define proc_ops_init(x, open, read, write, release, mmap) \
	proc_ops_t x = { \
		.proc_open    = open, \
		.proc_read    = read, \
		.proc_write   = write, \
		.proc_release = release, \
		.proc_mmap    = mmap, \
	}

#endif


#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)

#define vmf_address(vmf)	((unsigned long) (vmf)->virtual_address)

#else

#define vmf_address(vmf)	((unsigned long) (vmf)->address)

#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 17, 0)

typedef int vm_fault_t;

#else


#endif


#ifdef CONFIG_ARM64

#else

#endif


struct thinros_shm_channel
{
	unsigned long	paddr;
	atomic_t	ref_count;
	atomic_t	total;
};

// TODO: replace with thinros queue
struct thinros_shm_agent
{
	int		sid;
};

static struct thinros_shm_channel shm;

static void thinros_shm_link(struct vm_area_struct * vma)
{
	int err;
	struct thinros_shm_agent * agent;

	agent = (struct thinros_shm_agent *) vma->vm_private_data;

	debug("remap_pfn_range vma 0x%lx pfn 0x%lx size %lu protection 0x%llx\n",
		vma->vm_start, shm.paddr >> PAGE_SHIFT, NONSECURE_PARTITION_SIZE,
		(unsigned long long) vma->vm_page_prot.pgprot);

	err = remap_pfn_range(vma, vma->vm_start, shm.paddr >> PAGE_SHIFT,
				NONSECURE_PARTITION_SIZE, vma->vm_page_prot);
	if (err)
	{
		panic("error %d: failed to map physical addr 0x%lx to virtual addr 0x%lx (size %lu)\n", 
			err, shm.paddr, vma->vm_start, (size_t) NONSECURE_PARTITION_SIZE);
	}

	atomic_inc(&shm.ref_count);
	agent->sid = atomic_inc_return(&shm.total);
}

static void thinros_shm_unlink(struct vm_area_struct * vma)
{
	struct thinros_shm_agent * agent;
	/* TODO: unmap the shared memory region from the user address space */

	agent = (struct thinros_shm_agent *) vma->vm_private_data;
	agent->sid = 0;
	atomic_dec(&shm.ref_count);
}

static void thinros_vma_close(struct vm_area_struct * vma)
{
	debug("untrusted agent close the channel (0x%lx - 0x%lx).\n", vma->vm_start, vma->vm_end);
	// TODO: notify thinros the session is closed.

	thinros_shm_unlink(vma);
}


static vm_fault_t _thinros_vma_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct page * page;
	struct thinros_shm_agent * agent;
	/* usually not happen, check the range and try to map it again */

	if (shm.paddr <= vmf_address(vmf) && vmf_address(vmf) < shm.paddr + NONSECURE_PARTITION_SIZE)
	{
		debug("page fault @ 0x%lx, remap the page\n", vmf_address(vmf));

		agent = (struct thinros_shm_agent *) vma->vm_private_data;
		if (agent->sid > 0)
		{
			/* map a single page */
			unsigned long offset = PAGE_ALIGN(vmf_address(vmf)) - vma->vm_start;
			unsigned long pfn = (shm.paddr + offset) >> PAGE_SHIFT;
			remap_pfn_range(vma, PAGE_ALIGN(vmf_address(vmf)), pfn, PAGE_SIZE, vma->vm_page_prot);
			page = pfn_to_page(pfn);
			get_page(page);
			vmf->page = page;

		}
		else
		{
			thinros_shm_link(vma);
		}
		return 0;
	}
	else
	{
		panic("error: 0x%lx out of bound!\n", vmf_address(vmf));
		/* send sigbus to user */
		return VM_FAULT_SIGBUS;
	}
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)

static vm_fault_t thinros_vma_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	return _thinros_vma_fault(vma, vmf);
}

#else

static vm_fault_t thinros_vma_fault(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	return _thinros_vma_fault(vma, vmf);
}

#endif


static void thinros_vma_open(struct vm_area_struct *vma)
{
	debug("untrusted agent try to access the channel (0x%lx - 0x%lx).\n", vma->vm_start, vma->vm_end);
	debug("authenticate the node ...\n");
	/* TODO: connect with thinros authenticate */

	debug("create shared memory channel ...\n");
	thinros_shm_link(vma);
}

static struct vm_operations_struct vm_ops = 
{
	.open  = thinros_vma_open,
	.close = thinros_vma_close,
	.fault = thinros_vma_fault,
};

static int thinros_pfs_mmap(struct file * filp, struct vm_area_struct *vma)
{
	debug("thinros_pfs_mmap vma start 0x%lx end 0x%lx\n", vma->vm_start, vma->vm_end);
	vma->vm_ops = &vm_ops;
	/* no swap */
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_private_data = filp->private_data;
	thinros_vma_open(vma);

	return 0;
}

static int thinros_pfs_open (struct inode * inode, struct file * filp)
{
	struct thinros_shm_agent * agent;
	agent = kmalloc(sizeof(struct thinros_shm_agent), GFP_KERNEL);
	agent->sid = 0;
	filp->private_data = agent;
	debug("thinros_pfs_open is called\n");
	
	return 0;
}

static ssize_t thinros_pfs_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
	ssize_t rd;

	debug("thinros_pfs_read 0x%p size %lu offset %llu\n", buf, len, *off);

	if ((size_t) NONSECURE_PARTITION_SIZE <= *off)
	{
		rd = 0;
	}
	else
	{
		rd = min(len, (size_t) NONSECURE_PARTITION_SIZE - (size_t) *off);
		if (copy_to_user(buf, (void *)(shm.paddr + *off), rd))
		{
			rd = - EFAULT;
		}
		else
		{
			*off += rd;
		}
	}

	return rd;
}

static ssize_t thinros_pfs_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
	unsigned long to_copy;
	debug("thinros_pfs_write 0x%p size %lu offset %llu\n", buf, len, *off);

	to_copy = min(len, (size_t) NONSECURE_PARTITION_SIZE);
	if (copy_from_user((void *)(shm.paddr + *off), buf, to_copy))
	{
		return -EFAULT;
	}
	else
	{
		*off += to_copy;
		return len;
	}
}

static int thinros_pfs_release(struct inode *inode, struct file *filp)
{
	struct thinros_shm_agent * agent;
	debug("thinros_pfs_release file closed\n");
	agent = filp->private_data;
	if (agent != NULL)
	{
		kfree(agent);
		filp->private_data = NULL;
	}
	return 0;
}

static const proc_ops_init(fops,
	thinros_pfs_open,
	thinros_pfs_read, 
	thinros_pfs_write, 
	thinros_pfs_release, 
	thinros_pfs_mmap);

static int thinros_dev_init(void)
{
	debug("start to load thinros kernel module...\n");
	proc_create(n_devname, 0666, NULL, &fops);
	shm.paddr = NONSECURE_PARTITION_LOC;
	atomic_set(&shm.ref_count, 0);
	atomic_set(&shm.total, 1);
	return 0;
}

static void thinros_dev_exit(void)
{
	remove_proc_entry(n_devname, NULL);
	debug("thinros kernel module exit!\n");
}


module_init(thinros_dev_init);
module_exit(thinros_dev_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
