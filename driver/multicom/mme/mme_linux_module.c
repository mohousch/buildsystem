/*
 * The source for the mme module and device driver implementation
 */

#include <asm/uaccess.h>

#include <linux/fs.h>
#include <linux/netdevice.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/highmem.h>
#include <linux/mm.h>

#include "embx_osinterface.h"

#include <linux/device.h>
#include <linux/cdev.h>

#include "mme_linux.h"
#include "mme_hostP.h"

/* ==========================================================================
 * Exports
 * ========================================================================== 
 */

MODULE_DESCRIPTION("MME Module");
MODULE_AUTHOR("STMicroelectronics Ltd");
#ifdef MULTICOM_GPL
MODULE_LICENSE("GPL");
#else
MODULE_LICENSE("Copyright 2004 STMicroelectronics, All rights reserved");
#endif /* MULTICOM_GPL */

EXPORT_SYMBOL( MME_GetTransformerCapability );
EXPORT_SYMBOL( MME_DeregisterTransformer );
EXPORT_SYMBOL( MME_DeregisterTransport );
EXPORT_SYMBOL( MME_SendCommand );
EXPORT_SYMBOL( MME_NotifyHost );
EXPORT_SYMBOL( MME_AbortCommand );
EXPORT_SYMBOL( MME_AllocDataBuffer );
EXPORT_SYMBOL( MME_RegisterTransport );
EXPORT_SYMBOL( MME_FreeDataBuffer );
EXPORT_SYMBOL( MME_Init );
EXPORT_SYMBOL( MME_ModifyTuneable );
EXPORT_SYMBOL( MME_Term );
EXPORT_SYMBOL( MME_RegisterTransformer );
EXPORT_SYMBOL( MME_TermTransformer );
EXPORT_SYMBOL( MME_InitTransformer );
EXPORT_SYMBOL( MME_KillTransformer );
EXPORT_SYMBOL( MME_KillCommand );
EXPORT_SYMBOL( MME_KillCommandAll );
EXPORT_SYMBOL( MME_IsStillAlive );

#ifdef EMBX_RECEIVE_CALLBACK
EXPORT_SYMBOL( MME_InitTransformer_Callback );
#endif /* EMBX_RECEIVE_CALLBACK */

/* ==========================================================================
 * Module parameters
 * ========================================================================== 
 */

static int major = MME_MAJOR_NUM;	/* Can set to 0 for dynamic */
module_param     (major, int, 0);
MODULE_PARM_DESC (major, "MME Major device number");

static char *    transport0 = NULL;
module_param     (transport0, charp, S_IRUGO);
MODULE_PARM_DESC (transport0, "Name of EMBX transport 0 to be used by MME");

static char *    transport1 = NULL;
module_param     (transport1, charp, S_IRUGO);
MODULE_PARM_DESC (transport1, "Name of EMBX transport 1 to be used by MME");

static char *    transport2 = NULL;
module_param     (transport2, charp, S_IRUGO);
MODULE_PARM_DESC (transport2, "Name of EMBX transport 2 to be used by MME");

static char *    transport3 = NULL;
module_param     (transport3, charp, S_IRUGO);
MODULE_PARM_DESC (transport3, "Name of EMBX transport 3 to be used by MME");

static int       doNotInit = 0;
module_param     (doNotInit, int, S_IRUGO);
MODULE_PARM_DESC (doNotInit, "Do not initialize MME - when using MME API in pure kernel mode");

#define MAX_TRANSPORTS 4
static  char** transports[MAX_TRANSPORTS] = { &transport0, &transport1, 
                                              &transport2, &transport3 };

/* ==========================================================================
 * Declarations
 * ========================================================================== 
 */
static int InitTransformer(struct file* filp, unsigned long arg);
static int TermTransformer(struct file* filp, unsigned long arg);
static int WaitTransformer(struct file* filp, unsigned long arg);
static int SendCommand    (struct file* filp, unsigned long arg);
static int AbortCommand   (struct file* filp, unsigned long arg);
static int GetCapability  (struct file* filp, unsigned long arg);
static int AllocDataBuffer(struct file* filp, unsigned long arg);
static int FreeDataBuffer (struct file* filp, unsigned long arg);

static int mme_open   (struct inode* inode, struct file* filp);
static int mme_release(struct inode* inode, struct file* filp);
static int mme_ioctl  (struct inode* inode, struct file* filp, unsigned int command, unsigned long arg);
static int mme_mmap   (struct file* filp, struct vm_area_struct* vma);

/* ==========================================================================
 * Types
 * ========================================================================== 
 */

typedef struct AllocBuf {
	unsigned long         offset;
	void*                 allocAddress;
	unsigned              size;
	MME_AllocationFlags_t flags;
	Transformer_t*        transformer;
	struct AllocBuf*       next;
} AllocBuf_t;

typedef struct InstanceInfo_s {
	Transformer_t*   transformer[MAX_TRANSFORMER_INSTANCES];
	
	AllocBuf_t*      allocatedBufs;
} InstanceInfo_t;

typedef struct {
	MME_Command_t    command;
	MME_Command_t*   userCommand;
        void*            userPrivateStatus;

	atomic_t         refCount;	/* Bugzilla 4956 */
} DriverCommand_t;

/* ==========================================================================
 * instantiations
 * ========================================================================== 
 */
static struct file_operations mme_ops = {
	.owner =   THIS_MODULE,
	.open  =   mme_open,
	.release = mme_release,
	.ioctl =   mme_ioctl,
	.mmap =    mme_mmap
};


static struct cdev mme_cdev;
static dev_t       mme_devid;
#ifdef MULTICOM_GPL
static struct class *mme_class;
#ifdef __TDT__
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 30)
static struct class_device *mme_class_dev;
#else /* >= stlinux24 */
static struct device *mme_class_dev;
#endif
#else /* !__TDT__ */
static struct class_device *mme_class_dev;
#endif
#endif

/* ==========================================================================
 * 
 * Called by the kernel when the module is loaded
 *
 * ========================================================================== 
 */

#define DEBUG_FUNC_STR "init_module"
int init_module(void)
{
	int i, result;
	MME_ERROR status;

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Initializing /dev/%s - major num %d\n", 
					 MME_DEV_NAME, major));
#ifdef __TDT__
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
	/* the macro does not exit after stlinux23 (there seems to be no replacement) */
        SET_MODULE_OWNER(&mme_ops);
#endif
#else
        SET_MODULE_OWNER(&mme_ops);
#endif

	/* If we want to use the API entirely in kernel mode, skip the init */
	if (doNotInit) {
	        return 0;
        }
	status = MME_Init();

	if (MME_SUCCESS != status) {
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_ERROR_STR "Failed MME_Init - status %d\n", status));
		return -1;
	}

	for (i=0; i<MAX_TRANSPORTS; i++) {
		if (*(transports[i])) {
			status = MME_RegisterTransport(*(transports[i]));
			if (MME_SUCCESS != status) {
				MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_ERROR_STR 
				         "Failed to register transport %s, status %d\n", 
                                         *(transports[i]), status));
				return -1;
			}
		}
	}

	if (major) {
		/* Static major number allocation */
		mme_devid = MKDEV(major, 0);
		result =  register_chrdev_region(mme_devid, MME_DEV_COUNT, MME_DEV_NAME);
	}
	else {
		/* Dynamic major number allocation */
		result = alloc_chrdev_region(&mme_devid, 0, MME_DEV_COUNT, MME_DEV_NAME);
	}
	
	if (result) {
		printk(KERN_ERR "mme: register_chrdev failed : %d\n", result);
		goto err_register;
	}
	
	cdev_init(&mme_cdev, &mme_ops);
	mme_cdev.owner = THIS_MODULE;
	result = cdev_add(&mme_cdev, mme_devid, MME_DEV_COUNT);

	if (result) {
		printk(KERN_ERR "mme: cdev_add failed : %d\n", result);
		goto err_cdev_add;
	}
	
#ifdef MULTICOM_GPL
	/* It appears we have to create a class in order for udev to work */
	mme_class = class_create(THIS_MODULE, MME_DEV_NAME);
	if((result = IS_ERR(mme_class))) {
		printk(KERN_ERR "mme: class_create failed : %d\n", result);
		goto err_class_create;
	}
	
	/* class_device_create() causes the /dev/mme file to appear when using udev
	 * however it is a GPL only function.
	 */
#if  defined(__TDT__) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30))
	/* class_device_create does not exist anymore after stlinux23 */
	mme_class_dev = device_create(mme_class, NULL, mme_devid, NULL, MME_DEV_NAME);
#else
	mme_class_dev = class_device_create(mme_class, NULL, mme_devid, NULL, MME_DEV_NAME);
#endif
	if((result = IS_ERR(mme_class_dev)))
	{
		printk(KERN_ERR "mme: class_device_create failed : %d\n", result);
		goto err_class_device_create;
	}
#endif /* MULTICOM_GPL */

        MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Driver '%s' registered : major %d\n",
					 MME_DEV_NAME, MAJOR(mme_devid)));
	
	return result;

#ifdef MULTICOM_GPL
err_class_device_create:
	class_destroy(mme_class);
#endif /* MULTICOM_GPL */

#ifdef MULTICOM_GPL
err_class_create:
#endif /* MULTICOM_GPL */
	cdev_del(&mme_cdev);

err_cdev_add:
	unregister_chrdev_region(mme_devid, MME_DEV_COUNT);

err_register:

        MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Driver NOT registered : %d\n", result));

	return result;
}
#undef DEBUG_FUNC_STR

/* ==========================================================================
 *
 * Called by the kernel when the module is unloaded
 *
 * ========================================================================== 
 */
#define DEBUG_FUNC_STR "release_module"
void cleanup_module(void)
{
	MME_ERROR status;
	int i;

	/* If we want to use the API entirely in kernel mode, skip the init */
	if (doNotInit) {
	        return;
        }

	for (i=0; i<MAX_TRANSPORTS; i++) {
		if (*(transports[i])) {
			status = MME_DeregisterTransport(*(transports[i]));
			if (MME_SUCCESS != status) {
				printk("ERROR: could not terminate MME\n");
				MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_ERROR_STR 
					 "Failed to deregister transport %s, status %d\n", 
                                         *(transports[i]), status));
				return;
			}
		}
	}

	status = MME_Term();
        if (MME_SUCCESS != status) {
		printk("ERROR: could not terminate MME\n");
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_ERROR_STR
                   "Failed to terminate MME, status %d\n", status));
	}
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "unregistering driver %s\n", MME_DEV_NAME));

#ifdef MULTICOM_GPL
	class_destroy(mme_class);
#endif /* MULTICOM_GPL */
	cdev_del(&mme_cdev);

	unregister_chrdev_region(mme_devid, MME_DEV_COUNT);

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "<<<<\n"));
}
#undef DEBUG_FUNC_STR

/* ==========================================================================
 *
 * Called by the kernel when the device is opened by an app (the mme user lib)
 *
 * ========================================================================== 
 */

#define DEBUG_FUNC_STR "mme_open"
static int mme_open(struct inode* inode, struct file* filp) 
{
	InstanceInfo_t* instanceInfo;

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR ">>>>\n"));

	instanceInfo = (InstanceInfo_t*) EMBX_OS_MemAlloc(sizeof(InstanceInfo_t));
	if (!instanceInfo) {
		goto nomem;
        }

	memset(instanceInfo, 0, sizeof(InstanceInfo_t));

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Instance 0x%08x\n", (int)instanceInfo));

	filp->private_data = instanceInfo;

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "<<<< (0)\n"));
	return 0;

nomem:
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "<<<< (-ENOTTY)\n"));
	return -ENOTTY;
}
#undef DEBUG_FUNC_STR 

/* ==========================================================================
 *
 * Called by the kernel when the device is closed by an app (the mme user lib)
 *
 * ========================================================================== 
 */
#define DEBUG_FUNC_STR "mme_release"
static int mme_release(struct inode* inode, struct file* filp) 
{
	InstanceInfo_t* instanceInfo = (InstanceInfo_t*)filp->private_data;
	
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR ">>>>\n"));

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Instance 0x%08x\n", (int)instanceInfo));

#if 0
	/* XXXX This doesn't work because all the blocking EMBX_OS_EVENT_WAIT() calls
	 * exit abnormally when a signal is pending against the user process.
	 * Not sure how to fix it, perhaps switch to non-interruptible EVENT_WAIT calls ??
	 */
	for (i = 0; i < MAX_TRANSFORMER_INSTANCES; i++)
	{
		Transformer_t * transformer;

		if ((transformer = instanceInfo->transformer[i]))
		{
			MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Term transformer %p handle 0x%08x\n", transformer, transformer->handle));
			
			while (MME_TermTransformer(transformer->handle) == MME_COMMAND_STILL_EXECUTING)
			{
				EMBX_OS_Delay(1);
			}
			
			instanceInfo->transformer[i] = NULL;
		}
	}
#endif

	EMBX_OS_MemFree(filp->private_data);

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "<<<< (0)\n"));
	return 0;
}
#undef DEBUG_FUNC_STR 

/* ==========================================================================
 * 
 * Called by the kernel when an ioctl sys call is made
 *
 * ========================================================================== 
 */
#define DEBUG_FUNC_STR "mme_ioctl"
static int mme_ioctl(struct inode* inode, struct file* filp, unsigned int command, unsigned long arg)
{
	int magic  = _IOC_TYPE(command);
	int op     = _IOC_NR(command);
	int result = -ENOTTY;
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR ">>>> command 0x%08x, op 0x%08x\n", command, op));

	if (MME_IOCTL_MAGIC != magic) {
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_ERROR_STR "<<<< -ENOTTY - command 0x%08x\n", command));
		return -ENOTTY;
	}

	switch (op) {
	case WaitTransformer_c:
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Command WaitTransformer\n"));
		result = WaitTransformer(filp, arg);
		break;
	case InitTransformer_c:
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Command InitTransformer\n"));
		result = InitTransformer(filp, arg);
		break;
	case TermTransformer_c:
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Command TermTransformer\n"));
		result = TermTransformer(filp, arg);
		break;
	case SendCommand_c:
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Command SendCommand\n"));
		result = SendCommand(filp, arg);
		break;
	case AbortCommand_c:
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Command AbortCommand\n"));
		result = AbortCommand(filp, arg);
		break;
	case TransformerCapability_c:
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Command TransformerCapability\n"));
		result = GetCapability(filp, arg);
		break;
	case AllocDataBuffer_c:
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Command AllocDataBuffer\n"));
		result = AllocDataBuffer(filp, arg);
		break;
	case FreeDataBuffer_c:
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Command FreeDataBuffer\n"));
		result = FreeDataBuffer(filp, arg);
		break;
	default:
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_ERROR_STR "Invalid ioctl command\n"));
		result =  -ENOTTY;
		break;
	}

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "<<<< (%d)\n", result));
	return result;
}
#undef DEBUG_FUNC_STR 

/* ==========================================================================
 * 
 * mme_mmap()
 * Called via mmap sys call from the mme user library
 *
 * ========================================================================== 
 */

#define DEBUG_FUNC_STR "mme_mmap"
static int mme_mmap(struct file* filp, struct vm_area_struct* vma)
{
	int                        result;
	InstanceInfo_t*            instanceInfo = (InstanceInfo_t*)filp->private_data;
	unsigned long              size         = vma->vm_end-vma->vm_start;
	unsigned long              offset       = vma->vm_pgoff * PAGE_SIZE;

	AllocBuf_t*                allocBuf;

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "offset %lx size %ld\n", offset, size));

	/* Lookup the supplied address in the allocated buffer list
	 * This has two effects;
	 *   a) It means we can stop the user from arbritarily mapping any bit of physical memory
	 *   b) We can find out the MME_ALLOCATION_[UN]CACHED flags in order to create the correct mapping
	 */
	for (allocBuf = instanceInfo->allocatedBufs; allocBuf; allocBuf = allocBuf->next) {
		if (allocBuf->offset == offset && allocBuf->size == size)
			/* Found it! */
			break;
	}
	
	if (allocBuf == NULL) {
		result = -EINVAL;
		goto exit;
	}
	
	/* Set the VM_IO flag to signify the buffer is mmapped
           from an allocated data buffer */
	vma->vm_flags |= VM_RESERVED | VM_IO;

        if (allocBuf->flags & MME_ALLOCATION_UNCACHED) {
	        /* Mark the physical area as uncached */
	        vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
        }

	if (remap_pfn_range(vma, vma->vm_start, offset>>PAGE_SHIFT, size, vma->vm_page_prot) < 0)
	{
 	        MME_Info( MME_INFO_LINUX_USER, (DEBUG_ERROR_STR 
                          "Failed remap_page_range - start 0x%08x, phys 0x%08x, size %d\n", 
                          (int)(vma->vm_start), (int)offset, (int)size));
		result = -EAGAIN;
		goto exit;
	}

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Mapped virt 0x%08x len %d to phys 0x%08x\n", 
                 (int)vma->vm_start, (int)size, (int)offset));

	result = 0;

exit:
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "<<<< (%d)\n", result));		
	return result;
}
#undef DEBUG_FUNC_STR 

#define DEBUG_FUNC_STR "FreeCommand"
static void FreeCommand(DriverCommand_t *driverCommand)
{
	MME_Command_t *command = &driverCommand->command;
	int i;
	
	/* Bugzilla 4956: Only free the data structure once the refCount hits zero */
	if (!atomic_dec_and_test(&driverCommand->refCount))
		return;
	
	for (i=0; i<command->NumberInputBuffers + command->NumberOutputBuffers; i++) {
		MME_DataBuffer_t* dataBuffer = command->DataBuffers_p[i];

		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "buffer 0x%08x\n", 
						 (int) dataBuffer ));
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Num scatter pages %d\n",
						 dataBuffer->NumberOfScatterPages));
        	/* Free the array of kernel scatter pages */
		EMBX_OS_MemFree(dataBuffer->ScatterPages_p);
	}

	if (i) {
		/* There are some data buffers */
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Freeing buffer 0x%08x\n",
						 (int) command->DataBuffers_p[0]));

		/* Free the data buffer - the first ptr has it's start address */
		EMBX_OS_MemFree(command->DataBuffers_p[0]);

		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR 
			 "Freeing buffer ptrs array  0x%0xx\n",
			 (int) command->DataBuffers_p));
		/* Free the buffer pointers */
		EMBX_OS_MemFree(command->DataBuffers_p);
	}

        /* Free the user data */
 	if (command->Param_p && command->ParamSize) {
		EMBX_OS_MemFree(command->Param_p);
        }

	if (command->CmdStatus.AdditionalInfo_p && command->CmdStatus.AdditionalInfoSize) {
		EMBX_OS_MemFree(command->CmdStatus.AdditionalInfo_p);
        }

	/* Free the command */
	EMBX_OS_MemFree(driverCommand);

	return;
}

#undef DEBUG_FUNC_STR
/* ==========================================================================
 * 
 * Called via an ioctl to wait for transformer events
 *
 * ========================================================================== 
 */
#define DEBUG_FUNC_STR "WaitTransformer"
static int WaitTransformer(struct file* filp, unsigned long arg)
{
	TransformerWait_t* waitInfo = (TransformerWait_t*)arg; 
	Transformer_t*     transformer;
	MME_Event_t        event;
	int                returnCode=0;
	MME_ERROR          status;
	MME_Command_t*     userCommand;
	DriverCommand_t*   driverCommand;
	unsigned int	   nBufs;

	InstanceInfo_t* instanceInfo = (InstanceInfo_t*)filp->private_data;

	if (copy_from_user(&transformer, &(waitInfo->instance), sizeof(Transformer_t*))) {
		return -EFAULT;
	}

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR ">>>>\n"));
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Instance 0x%08x\n", (int)instanceInfo));
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Transformer 0x%08x\n", (int)transformer));

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR
                 "Calling MME_WaitForMessage -transformer 0x%08x\n", (int)transformer));

	status = MME_WaitForMessage(transformer, &event, (MME_Command_t **) &driverCommand);

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR
                 "Completed MME_WaitForMessage -transformer 0x%08x, status %d\n", (int)transformer, status));

	if (MME_SUCCESS != status) {
		returnCode = (MME_SYSTEM_INTERRUPT == status ? -EINTR : -ENOTTY);
		goto exit;
	}

	MME_Assert(driverCommand);
	userCommand = driverCommand->userCommand;

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR 
	         "Copying to user space status %d, event 0x%08x, command 0x%08x\n",
                 (int)status, (int)event, (int)userCommand));

	if (put_user(event, &(waitInfo->event)) ||
  	    put_user(userCommand, &(waitInfo->command)) ||
	    put_user(driverCommand->command.CmdStatus.State, &(userCommand->CmdStatus.State)) ||
	    put_user(driverCommand->command.CmdStatus.Error, &(userCommand->CmdStatus.Error))) {
		printk("Failed to copy to user command info\n");
		returnCode = -EFAULT;
		goto exit;
        }

        /* Copy the private status data */
	if (driverCommand->command.CmdStatus.AdditionalInfo_p && 
	    driverCommand->command.CmdStatus.AdditionalInfoSize &&
	    copy_to_user(driverCommand->userPrivateStatus, 
			 driverCommand->command.CmdStatus.AdditionalInfo_p,
			 driverCommand->command.CmdStatus.AdditionalInfoSize)) {
		printk("Failed to copy user status\n");
		returnCode = -EFAULT;
		goto exit;
	}

	/* Copy scatter page meta data back into the userspace buffer (and cope with cache aliasing
	 * on SH4
	 */
	nBufs = driverCommand->command.NumberInputBuffers + driverCommand->command.NumberOutputBuffers;
	if (nBufs) {
		MME_DataBuffer_t **userBufs;
		unsigned int iBuf;

		if (get_user(userBufs, &(userCommand->DataBuffers_p))) {
			printk("Could not follow user supplied pointer (DataBuffers_p)\n");
			returnCode = -EFAULT;
			goto exit;
		}

		for (iBuf=0; iBuf<nBufs; iBuf++) {
			MME_DataBuffer_t *driverBuf = driverCommand->command.DataBuffers_p[iBuf];
			MME_DataBuffer_t *userBuf;
			MME_ScatterPage_t *userPages;
			unsigned int iPage;

			if (get_user(userBuf, &(userBufs[iBuf])) ||
			    get_user(userPages, &(userBuf->ScatterPages_p))) {
				printk("Could not follow user supplied pointer (*DataBuffers_p or ScatterPages_p)\n");
				returnCode = -EFAULT;
				goto exit;
			}
		
			for (iPage=0; iPage<driverBuf->NumberOfScatterPages; iPage++) {
				MME_ScatterPage_t *driverPage = driverBuf->ScatterPages_p + iPage;
				MME_ScatterPage_t *userPage = userPages + iPage;

				if (put_user(driverPage->BytesUsed, &(userPage->BytesUsed)) |
				    put_user(driverPage->FlagsOut, &(userPage->FlagsOut))) {
					printk("Could not copy scatter page meta data to user memory\n");
					returnCode = -EFAULT;
					goto exit;
				}

#ifdef __SH4__
				/* Local Transformer fixup */
				if (transformer->info == NULL) {
#ifdef CONFIG_32BIT
					/* We cleared this flag in MapSingleBuffer so that we know to flush the cache
					 * as the kernel transformer would have been using a cached kernel addresses
					 * and the user may have an uncached mapping to the memory
					 */
					if (!(driverPage->FlagsIn & (MME_DATA_CACHE_COHERENT|MME_DATA_PHYSICAL)))
						EMBX_OS_PurgeCache(driverPage->Page_p, driverPage->Size);
#else
					/* if we have been working with a P1 pointer then synchronise potentially 
					 * cache aliased views of memory
					 */
					if (driverPage->Page_p == P1SEGADDR(driverPage->Page_p)) {
						unsigned int virtAddr;
						struct vm_area_struct *	vma;
						
						/* suppress warnings by lying to get_user about the type of Page_p.
						 * this is a little danagerous since it might hide real warnings in
						 * future versions. we try to migigate this by adding a sizeof check
						 * to introduce an obvious bug in this case.
						 */
						if (get_user(virtAddr, (unsigned int *) &(userPage->Page_p)) ||
						    sizeof(unsigned int) != sizeof(userPage->Page_p)) {
							printk("Could not read Page_p from user memory\n");
							returnCode = -EFAULT;
							goto exit;
						}
						
						vma = find_vma(current->mm, virtAddr);
						
						/* synchronise potentially cache aliased views of memory */
						flush_cache_range(vma, virtAddr, virtAddr + driverPage->Size);
					}
#endif
				}
#endif /* __SH4__ */
			}
		}
	}

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Done copying\n" ));

	if (MME_COMMAND_COMPLETED_EVT == event) {
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Doing FreeCommand \n" ));
		/* Do cleanup for this command */
		FreeCommand(driverCommand);
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Done FreeCommand \n" ));
	}
	
	if (put_user(status, &(waitInfo->status))) {
		returnCode = -EFAULT;
	}
exit:
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "<<<< (%d)\n", returnCode));

	return returnCode;
}

#undef DEBUG_FUNC_STR

/* ==========================================================================
 * 
 * InitTransformer()
 * Called via an ioctl to initalize a transformer
 *
 * ========================================================================== 
 */
#define DEBUG_FUNC_STR "InitTransformer"
static int InitTransformer(struct file* filp, unsigned long arg)
{
        InitTransformer_t            initInfo;
	char                         name[MME_MAX_TRANSFORMER_NAME];
	int                          returnCode = -ENODEV;

	InstanceInfo_t*              instanceInfo = (InstanceInfo_t*)filp->private_data;
	MME_GenericParams_t          localParams = NULL;

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR ">>>>\n"));
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Instance 0x%08x\n", (int)instanceInfo));

	/* copy the whole struct and the name */
	if (copy_from_user(&initInfo, (void*)arg, sizeof(initInfo)) || 	
	    copy_from_user(&name, initInfo.name, initInfo.nameLength)) {
		returnCode = -EFAULT;
		goto exit;
	}
	name[initInfo.nameLength] = '\0';

	/* Deal with init parameters - copy to kernel space */
	if (initInfo.params.TransformerInitParams_p && initInfo.params.TransformerInitParamsSize) {
		localParams = EMBX_OS_MemAlloc(initInfo.params.TransformerInitParamsSize);
		if (!localParams) {
			returnCode = -ENOMEM;
			goto exit;
                }

		if (copy_from_user(localParams, initInfo.params.TransformerInitParams_p,
				   initInfo.params.TransformerInitParamsSize)) {
			returnCode = -EFAULT;
			goto exitFree;
                }
		initInfo.params.TransformerInitParams_p = localParams;
        }

	/* Init the transformer without the worker thread */
	initInfo.status = MME_InitTransformerInternal(name, &initInfo.params, 
                                                      &initInfo.handle, EMBX_FALSE);

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR
                 "Transformer handle 0x%08x, instance 0x%08x, status %d\n",
                 (int)initInfo.handle, (int)initInfo.instance, (int)initInfo.status));

	if (MME_SUCCESS != initInfo.status) {

		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_ERROR_STR
                         "Failed to create transformer\n" ));
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_ERROR_STR "<<<< (%d)\n", -ENODEV ));
		returnCode = -ENODEV;
		goto exitFree;
	}

	initInfo.status = MME_FindTransformerInstance(initInfo.handle, 
                                                      (Transformer_t**)&initInfo.instance);

	if (MME_SUCCESS == initInfo.status) {
		unsigned i;
		
		/* Stash the transformer id against the fd info */
		for (i = 0; i < MAX_TRANSFORMER_INSTANCES; i++)
		{
			if (instanceInfo->transformer[i] == NULL)
			{
				instanceInfo->transformer[i] = initInfo.instance;
				break;
			}
		}

		returnCode = 0;
	} else {
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_ERROR_STR "Internal error\n" ));
		returnCode = -EFAULT;
	}

exitFree:
	if (localParams) {
		EMBX_OS_MemFree(localParams);
        }
exit:
	/* copy the lot back - scope for optimisation */
	if (copy_to_user((void*)arg, &initInfo,  sizeof(initInfo))) {
		returnCode = -EFAULT;
	}

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "<<<< (%d) - MME_ERROR (%d)\n", 
                 returnCode, initInfo.status));
	return returnCode;
}
#undef DEBUG_FUNC_STR 

#define DEBUG_FUNC_STR "ValidatePagesContiguous"
static int ValidatePagesContiguous(unsigned virtAddress, unsigned size, unsigned* physBase) {
	/* Iterate over the pages ensuring they are physically contiguous */
        int result = 0;

	unsigned virtBase  = (virtAddress / PAGE_SIZE) * PAGE_SIZE;
	unsigned offset    = virtAddress - virtBase;
	unsigned range     = offset + size;
	unsigned pageCount = range/PAGE_SIZE + ((range%PAGE_SIZE)?1:0);
	unsigned lastPhys  = 0xffffffff;
	unsigned thisPage;

	for (thisPage=0; thisPage<pageCount; thisPage++, virtBase += PAGE_SIZE) {
		EMBX_ERROR res;
		EMBX_UINT phys;
		
		/* Perform a Virtual to physical translation of the user address */
		res = EMBX_OS_VirtToPhys((EMBX_VOID *)virtBase, &phys);
		
		if (res == EMBX_INVALID_ARGUMENT) {
			MME_Info(MME_INFO_LINUX_MODULE, 
                		(DEBUG_ERROR_STR "Page lookup failed for address 0x%08x\n", virtBase));
                	printk("Page lookup failed for address 0x%08x\n", virtBase);
			result = -EFAULT;
			break;
		}
		
		if (lastPhys == 0xffffffff) {
			lastPhys  = phys;
			*physBase = offset + lastPhys;
#ifdef NO_PAGE_CHECK
			return 0;
#endif
		} else if (phys == (lastPhys + PAGE_SIZE)) {
			lastPhys += PAGE_SIZE;		
		} else {
                	printk("Page discontinuous virtual 0x%08x, last phys 0x%08x, phys 0x%08x, pageCount %d\n", 
                	       virtBase, lastPhys, phys, pageCount);
                	
			result = -EFAULT;
			break;
		}
	}        

	return result;
}
#undef DEBUG_FUNC_STR

#define DEBUG_FUNC_STR "MapSingleBuffer"
static int MapSingleBuffer(Transformer_t* transformer, InternalDataBuffer_t* dstBuffer, MME_DataBuffer_t* srcBuffer)
{
	int returnCode, i;
        /* Need to copy bytes in InternalDataBuffer_t following the MME_DataBuffer_t */
	int extraBytes = sizeof(InternalDataBuffer_t) - sizeof(MME_DataBuffer_t);
	unsigned dst   = (unsigned) dstBuffer + sizeof(MME_DataBuffer_t);
	unsigned src   = (unsigned) srcBuffer + sizeof(MME_DataBuffer_t);

	if (copy_from_user((void*)dst, (void*)src, extraBytes)) {
		returnCode = -EFAULT;
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_ERROR_STR "Failed to copy buffer\n"
						 "Kernel 0x%08x, User 0x%08x, Size %d\n",
						 (int)dst, (int)src, extraBytes));
		goto exit;
	}
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Copied internal buffer extra - "
					 "Kernel 0x%08x, User 0x%08x, extra %d\n",
					 dst, src, extraBytes));

        /* Now iterate over all the scatter pages
	 * checking that they are contiguous and setting the appropriate cache flags
	 */
        for (i=0; i<dstBuffer->buffer.NumberOfScatterPages; i++) {
 		unsigned virtAddr = (unsigned) (unsigned)dstBuffer->buffer.ScatterPages_p[i].Page_p;

		unsigned physicalBase = 0;

   		/* Use the vma to get the cache status */
		struct vm_area_struct *	vma = find_vma(current->mm, virtAddr);

		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "vma 0x%08x, virt address 0x%08x\n", (int)vma, virtAddr ));
 		if (!vma) {
	        	MME_Info(MME_INFO_LINUX_MODULE, 
                                 (DEBUG_ERROR_STR "VMA lookup failed for address 0x%08x\n", virtAddr ));
			returnCode = -EFAULT;
			goto exit;
		}
		
		if (ValidatePagesContiguous(virtAddr, dstBuffer->buffer.ScatterPages_p[i].Size, &physicalBase)) {
			printk("ValidatePagesContiguous() - error\n");
			returnCode = -EFAULT;
			goto exit;
		}

		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "virtAddr 0x%08x len %d physicalBase 0x%08x\n",
						 virtAddr, dstBuffer->buffer.ScatterPages_p[i].Size, physicalBase));


#ifdef __SH4__
		/* Local Transformer fixup
		 * Here we can run into the SH4 cache aliasing issue as we will be
		 * operating on the same physical memory via two different virtual addresses
		 */
		if (transformer->info == NULL) {

			if (pgprot_val(vma->vm_page_prot) & _PAGE_CACHABLE) {

				/* synchronise potentially cache aliased views of memory */
				flush_cache_range(vma, virtAddr, virtAddr + dstBuffer->buffer.ScatterPages_p[i].Size);

#ifdef CONFIG_32BIT
				/* Convert phys addr back to a (cached) kernel vaddr */
				physicalBase = (unsigned) phys_to_virt(physicalBase);

				/* Clear the CACHE_COHERENT flag so we know we need to flush the cache on completion */
				dstBuffer->buffer.ScatterPages_p[i].FlagsIn &= ~MME_DATA_CACHE_COHERENT;
#else
				physicalBase = P1SEGADDR(physicalBase);
#endif

			} else {

#ifdef CONFIG_32BIT
				/* XXXX : What can we do here to get an uncached kernel vaddr ?? */
				/* Convert phys addr back to a (cached) kernel vaddr */
				physicalBase = (unsigned) phys_to_virt(physicalBase);

				/* Clear the CACHE_COHERENT flag so we know we need to flush the cache on completion */
				dstBuffer->buffer.ScatterPages_p[i].FlagsIn &= ~MME_DATA_CACHE_COHERENT;
#else
				physicalBase = P2SEGADDR(physicalBase);
#endif
			}

			dstBuffer->buffer.ScatterPages_p[i].Page_p = (void*)(physicalBase);
		}
#endif

        }

	returnCode = 0;
exit:
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "<<<< (%d) \n", returnCode));
	return returnCode;
}
#undef DEBUG_FUNC_STR

#define DEBUG_FUNC_STR "CopyBuffer"
	static int CopyBuffer(Transformer_t* transformer, InternalDataBuffer_t* dstBuffer, MME_DataBuffer_t* srcBuffer)
{
	int returnCode;
	MME_ScatterPage_t* scatterPages;
	
	/* Copy the MME_DataBuffer_t from user space */
	if (copy_from_user(dstBuffer, srcBuffer, sizeof(MME_DataBuffer_t))) {
		returnCode = -EFAULT;
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_ERROR_STR "Failed to copy buffer\n"
						 "Kernel 0x%08x, User 0x%08x, Size %d\n",
						 (int)dstBuffer, (int)srcBuffer, sizeof(MME_DataBuffer_t)));
		goto exit;
	}
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Copied buffer - "
					 "Kernel 0x%08x, User 0x%08x, Num scatter pages %d\n",
					 (int)dstBuffer, (int)srcBuffer, dstBuffer->buffer.NumberOfScatterPages));

	/* Allocate space for the array of scatter page descriptors */
	scatterPages = EMBX_OS_MemAlloc(sizeof(MME_ScatterPage_t) * dstBuffer->buffer.NumberOfScatterPages);
	if (!scatterPages) {
		returnCode = -ENOMEM;
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Failed to allocate scatter page prt list\n"));
		goto exit;
	}

	/* Copy the array of scatter pages */
	if (copy_from_user(scatterPages, dstBuffer->buffer.ScatterPages_p, 
                           sizeof(MME_ScatterPage_t) * dstBuffer->buffer.NumberOfScatterPages)) {
		returnCode = -EFAULT;
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Failed to copy scatter pages\n"
                                "Kernel 0x%08x, User 0x%08x, Size %d\n",
                                 (int)scatterPages, (int)dstBuffer->buffer.ScatterPages_p, 
                                 sizeof(MME_ScatterPage_t) * dstBuffer->buffer.NumberOfScatterPages));
		goto exitFree;
	}
        /* Point to the array of new pages in kernel space */
        dstBuffer->buffer.ScatterPages_p = scatterPages;
	
	returnCode = MapSingleBuffer(transformer, dstBuffer, srcBuffer);

	goto exit;
exitFree:
	EMBX_OS_MemFree(scatterPages);
exit:
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "<<<< (%d) \n", returnCode));
	return returnCode;
}

#undef  DEBUG_FUNC_STR

#define DEBUG_FUNC_STR "MapBuffers"
	static int MapBuffers(Transformer_t* transformer, MME_Command_t* commandInfo)
{
	int returnCode = 0;
	int i;
	unsigned buffers       = commandInfo->NumberInputBuffers + commandInfo->NumberOutputBuffers;
	unsigned bufferPtrSize = buffers * sizeof(MME_DataBuffer_t*);
	MME_DataBuffer_t** dataBufferPtrs;
	InternalDataBuffer_t* dataBuffers;
 		                 
	if (0 == buffers) {
		/* No buffers with this command */
		goto exit;
	}

	dataBufferPtrs = EMBX_OS_MemAlloc(bufferPtrSize);
        if (!dataBufferPtrs) {
                returnCode = -ENOMEM;
                goto exit;
        }
	dataBuffers = EMBX_OS_MemAlloc(sizeof(InternalDataBuffer_t) *
				       (commandInfo->NumberInputBuffers + 
					commandInfo->NumberOutputBuffers));
        if (!dataBuffers) {
                EMBX_OS_MemFree(dataBufferPtrs);
                returnCode = -ENOMEM;
                goto exit;
        }

	/* Get the array of pointers to data buffers */
	if (copy_from_user(dataBufferPtrs, commandInfo->DataBuffers_p, bufferPtrSize)) {
		returnCode = -EFAULT;
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Failed to copy buffer ptrs \n"
                         "Kernel 0x%08x, User 0x%08x, Size %d\n", 
                         (int)dataBufferPtrs, (int)commandInfo->DataBuffers_p, (int)bufferPtrSize ));
		goto exit;
	}

	/* Point to the array in kernel space */
	commandInfo->DataBuffers_p = dataBufferPtrs;
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Buffer ptrs at 0x%08x\n", 
                 (int)commandInfo->DataBuffers_p ));

	/* Iterate over all input and output buffers */
	for (i=0; i<buffers; i++) {
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Data buffer %d\n", i));

		returnCode = CopyBuffer(transformer, &dataBuffers[i], dataBufferPtrs[i]);
		if (returnCode) {
			goto exit;
		}
		
		/* Point to the kernel copy */
		dataBufferPtrs[i] = (MME_DataBuffer_t*)(&dataBuffers[i]);
		MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Done buffer at 0x%08x\n", 
                         (int)dataBufferPtrs[i]));
	}
	/* TODO - error cases */
	/* TODO - deallocation */
	/* TODO - remove hack */

exit:
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "<<<< (%d) \n", returnCode));
	return returnCode;
}
#undef DEBUG_FUNC_STR

/* ==========================================================================
 * 
 * SendCommand()
 * Called via an ioctl to send a command
 *
 * ========================================================================== 
 */
#define DEBUG_FUNC_STR "SendCommand"
static int SendCommand(struct file* filp, unsigned long arg)
{
        MME_ERROR mmeStatus = MME_INTERNAL_ERROR;
	int returnCode = 0;
	InstanceInfo_t* instanceInfo = (InstanceInfo_t*)filp->private_data;
	MME_TransformerHandle_t handle;
	SendCommand_t* sendCommandInfo = (SendCommand_t*)arg;

	DriverCommand_t* driverCommand = EMBX_OS_MemAlloc(sizeof(DriverCommand_t));
	MME_Command_t*   commandInfo   = &driverCommand->command;

	Transformer_t*   transformer;

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR ">>>>\n"));
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Instance 0x%08x\n", (int) instanceInfo));

	/* Bugzilla 4956 
	 * We set the reference count of this data structure to 2 so that it
	 * will not be freed by WaitTransformer until we have done the
	 * copy_to_user() below
	 */
	atomic_set(&driverCommand->refCount, 2);

	if (get_user(handle, &(sendCommandInfo->handle)) ||
	    get_user(driverCommand->userCommand, &(sendCommandInfo->commandInfo)) || 
	    copy_from_user(commandInfo, driverCommand->userCommand, sizeof(*commandInfo))) {
		returnCode = -EFAULT;
		goto errorFreeDriver;
	}

	if (sizeof(MME_Command_t) != commandInfo->StructSize) {
		returnCode = -EFAULT;
		goto errorFreeDriver;
	}

        /* Allocate kernel mem for the user private data and copy across */

	if (commandInfo->Param_p && commandInfo->ParamSize) {
                void* userParam_p = commandInfo->Param_p;
		commandInfo->Param_p = EMBX_OS_MemAlloc(commandInfo->ParamSize);

		if (NULL == commandInfo->Param_p) {
			returnCode = -ENOMEM;
			goto errorFreeDriver;
		}
		if (copy_from_user(commandInfo->Param_p, userParam_p, 
				   commandInfo->ParamSize)) {
			returnCode = -EFAULT;
			goto errorFreeParam;
                }
       	}

	if (commandInfo->CmdStatus.AdditionalInfo_p && commandInfo->CmdStatus.AdditionalInfoSize) {
                /* Need the user address to copy back when the command comlpetes */
                driverCommand->userPrivateStatus = commandInfo->CmdStatus.AdditionalInfo_p;
		commandInfo->CmdStatus.AdditionalInfo_p = EMBX_OS_MemAlloc(commandInfo->CmdStatus.AdditionalInfoSize);

		if (NULL == commandInfo->CmdStatus.AdditionalInfo_p) {
			returnCode = -ENOMEM;
			goto errorFreeParam;
		}
		if (copy_from_user(commandInfo->CmdStatus.AdditionalInfo_p, driverCommand->userPrivateStatus, 
				   commandInfo->CmdStatus.AdditionalInfoSize)) {
			returnCode = -EFAULT;
			goto errorFree;
                }
       	}

	mmeStatus = MME_FindTransformerInstance(handle, &transformer);
	if (MME_SUCCESS != mmeStatus) {
		MME_Info(MME_INFO_LINUX_USER, (DEBUG_ERROR_STR 
					       "Cannot find transformer instance 0x%08x\n", 
					       (int) handle));
		returnCode = -ENODEV;
		goto errorFree;
	}
	
	returnCode = MapBuffers(transformer, commandInfo);

	if (0 != returnCode) {
                if (-ENXIO == returnCode) {
        	        mmeStatus = MME_INVALID_ARGUMENT;
                } else {
		        returnCode = -EFAULT;
                }
		goto errorFree;
	}

	/* Call the MME api */
	mmeStatus = MME_SendCommand(handle, commandInfo);
        if (MME_SUCCESS != mmeStatus) {
                goto errorFree;
        }

	/* Copy the command status structure back - contains command id, state etc */
	if (copy_to_user(&(driverCommand->userCommand->CmdStatus), 
                         &(commandInfo->CmdStatus), 
			 offsetof(MME_CommandStatus_t, AdditionalInfo_p))) { /* Yuk - need to improve this */
		returnCode = -EFAULT;
        }


	/* Bugzilla 4956: Call FreeCommand which will 
	 * only free the structure when the refCount hits 0
	 * WaitTransformer() will also call it and the winner will
	 * free off the memory
	 */
	FreeCommand(driverCommand);

        goto exit;

errorFree:
	if (commandInfo->CmdStatus.AdditionalInfo_p && commandInfo->CmdStatus.AdditionalInfoSize) {
		EMBX_OS_MemFree(commandInfo->CmdStatus.AdditionalInfo_p);
        }

errorFreeParam:
 	if (commandInfo->Param_p && commandInfo->ParamSize) {
		EMBX_OS_MemFree(commandInfo->Param_p);
        }

errorFreeDriver:
        EMBX_OS_MemFree(driverCommand);

exit:
	if (put_user(mmeStatus, &(sendCommandInfo->status))) {
		returnCode = -EFAULT;
	}

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "<<<< (%d) - MME_ERROR (%d)\n", 
                 returnCode, mmeStatus));
	return returnCode;
}
#undef DEBUG_FUNC_STR 

/* ========================================================================== */

#define DEBUG_FUNC_STR "TermTransformer"
static int TermTransformer(struct file* filp, unsigned long arg)
{
	MME_ERROR status = MME_SUCCESS;
	int returnCode = 0;
	InstanceInfo_t* instanceInfo = (InstanceInfo_t*)filp->private_data;
	TermTransformer_t* termInfo = (TermTransformer_t*)arg;

	MME_TransformerHandle_t handle;

	if (get_user(handle, &(termInfo->handle))) {
		returnCode = -EFAULT;
		goto exit;
	}

	status = MME_TermTransformer(handle);

	/* On success, clear down the stashed transformer info */
	if (status == MME_SUCCESS)
	{
		unsigned i;
		
		/* Clear down the corresponding transformer info */
		for (i = 0; i < MAX_TRANSFORMER_INSTANCES; i++)
			if (instanceInfo->transformer[i] && instanceInfo->transformer[i]->handle == handle)
				instanceInfo->transformer[i] = NULL;
	}

        if (put_user(status, &(termInfo->status))) {
		returnCode = -EFAULT;
		goto exit;
	}
exit:
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "<<<< (%d) - MME_ERROR (%d)\n", returnCode, status));
	return returnCode;
}
#undef DEBUG_FUNC_STR 

/* ========================================================================== */

#define DEBUG_FUNC_STR "AbortCommand"
static int AbortCommand(struct file* filp, unsigned long arg)
{
	MME_ERROR status = MME_SUCCESS;
	int returnCode = 0;
	Abort_t* abortInfo = (Abort_t*)arg;
	MME_CommandId_t command;
	MME_TransformerHandle_t handle;

	if (get_user(handle, &(abortInfo->handle))) {
		returnCode = -EFAULT;
		goto exit;
	}
	if (get_user(command, &(abortInfo->command))) {
		returnCode = -EFAULT;
		goto exit;
	}

	status = MME_AbortCommand(handle, command);

        if (put_user(status, &(abortInfo->status))) {
		returnCode = -EFAULT;
		goto exit;
	}

exit:
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "<<<< (%d) - MME_ERROR (%d)\n", returnCode, status));
	return returnCode;
}
#undef DEBUG_FUNC_STR 


/* ========================================================================== */

#define DEBUG_FUNC_STR "GetCapability"
static int GetCapability(struct file* filp, unsigned long arg)
{
	MME_ERROR status = MME_SUCCESS;
	int returnCode = 0;
	Capability_t* capInfo = (Capability_t*)arg;

	char                         name[MME_MAX_TRANSFORMER_NAME];
	int                          length;
	char*                        namePtr;
	MME_TransformerCapability_t  capability;
	MME_TransformerCapability_t* capPtr;
	MME_GenericParams_t          paramsPtr;  

	if (get_user(length, &(capInfo->length))) {
		returnCode = -EFAULT;
		goto exit;
	}
	if (get_user(namePtr, &(capInfo->name))) {
		returnCode = -EFAULT;
		goto exit;
	}
	if (get_user(capPtr, &(capInfo->capability))) {
		returnCode = -EFAULT;
		goto exit;
	}

	/* Copy the name */
	/* Copy the capability */
	if (copy_from_user(name, namePtr, length) || 
	    copy_from_user(&capability, capPtr, sizeof(capability))) {
		returnCode = -EFAULT;
		goto exit;
        }
	name[length] = '\0';

	/* Allcate the params */
	paramsPtr = capability.TransformerInfo_p;
	if (paramsPtr) {
		capability.TransformerInfo_p = EMBX_OS_MemAlloc(capability.TransformerInfoSize);
		if (!capability.TransformerInfo_p) {
			returnCode = -ENOMEM;
			goto exit;
                }
        }

	status = MME_GetTransformerCapability(name, &capability);

	/* Copy the data back to user space */
	if (MME_SUCCESS == status) {
		if (copy_to_user(paramsPtr, capability.TransformerInfo_p, capability.TransformerInfoSize)        ||
		    copy_to_user(&(capPtr->Version), &(capability.Version), sizeof(capability.Version))          ||
		    copy_to_user(&(capPtr->InputType), &(capability.InputType), sizeof(capability.InputType))    ||
		    copy_to_user(&(capPtr->OutputType), &(capability.OutputType), sizeof(capability.OutputType))) {
			returnCode = -EFAULT;
			goto exit;
		}
        }

        if (put_user(status, &(capInfo->status))) {
		returnCode = -EFAULT;
		goto exit;
	}
exit:
	EMBX_OS_MemFree(capability.TransformerInfo_p);
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "<<<< (%d) - MME_ERROR (%d)\n", returnCode, status));
	return returnCode;
}
#undef DEBUG_FUNC_STR 

/* ========================================================================== */
#define DEBUG_FUNC_STR "AllocDataBuffer"
static int AllocDataBuffer(struct file* filp, unsigned long arg)
{
	InstanceInfo_t*         instanceInfo = (InstanceInfo_t*)filp->private_data;
	int                     result = 0;
	AllocDataBuffer_t*      userAllocInfo = (AllocDataBuffer_t*)arg;
	MME_TransformerHandle_t transformerHandle;
	Transformer_t*          transformer;
	unsigned                size;
	void*                   allocAddress = NULL;
	void*                   alignedBuffer;
	MME_AllocationFlags_t   flags;
	
	AllocBuf_t*             allocBuf = NULL;
	int                     allocSize;
	unsigned                mapsize;
	int                     onEmbxHeap;

	MME_ERROR               mmeStatus;
	EMBX_UINT               offset;
	
        if (get_user(transformerHandle, &userAllocInfo->handle)) {
		result = -EFAULT;
		goto exit;
	}
        if (get_user(size, &userAllocInfo->size)) {
		result = -EFAULT;
		goto exit;
	}
        if (get_user(flags, &userAllocInfo->flags)) {
		result = -EFAULT;
		goto exit;
	}

	mmeStatus = MME_FindTransformerInstance(transformerHandle, &transformer);
	if (MME_SUCCESS != mmeStatus) {
 	        MME_Info(MME_INFO_LINUX_USER, (DEBUG_ERROR_STR 
					      "Cannot find transformer instance 0x%08x\n", 
                                              (int) transformerHandle));
		result = -ENODEV;
		goto exit;
	}

	allocBuf = (AllocBuf_t *) EMBX_OS_MemAlloc(sizeof(AllocBuf_t));
	if (!allocBuf) {
		result =  -ENOMEM;
		goto exit;
	}
	
	/* returned buf must be page-aligned for mmap and be an exact number of pages*/
        /* over-allocate then round up to page boundary */
	allocSize = (size<=PAGE_SIZE)?(2*PAGE_SIZE):(2*PAGE_SIZE + size);

	if (transformer->info) {
		/* A remote transformer */
		EMBX_ERROR embxStatus = EMBX_Alloc(transformer->info->handle, allocSize, 
						   (EMBX_VOID **) &allocAddress);
		if (MME_SUCCESS != (enum MME_ERROR)embxStatus) {
			MME_Info( MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "Cannot EMBX_Alloc(%d)\n", 
							allocSize));
			result =  -ENOMEM;
			goto exit;
		}
		
        } else {
		/* A kernel local transformer */
		allocAddress = bigphysarea_alloc_pages((allocSize-1)/PAGE_SIZE + 1, 0, GFP_KERNEL);
		
		if (!allocAddress) {
			MME_Info( MME_INFO_LINUX_USER, (DEBUG_ERROR_STR "Cannot EMBX_OS_MemAlloc(%d)\n", 
							allocSize));
			result =  -ENOMEM;
			goto exit;
		}
        }

	if (0 == ((unsigned)allocAddress & (PAGE_SIZE-1))) {
		/* Already aligned */
		alignedBuffer = allocAddress;
	        mapsize = allocSize;
	} else {
		/* Round up to next page */
		alignedBuffer = (void*)(PAGE_SIZE + ((unsigned)allocAddress & ~(PAGE_SIZE-1)));
		mapsize = allocSize - ((unsigned)alignedBuffer - (unsigned)allocAddress);
        }
        /* And make the mapping size a multiple of page size - the nasty stuff below
           should boil down to bit shifting!
         */
	mapsize = (mapsize / PAGE_SIZE) * PAGE_SIZE;

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "Allocated EMBX data buffer (0x%08x) flags 0x%x\n", 
					 (int)alignedBuffer, flags));

	if (transformer->info)
	{
		/* Remote Transformer */
		
		EMBX_ERROR embxStatus;
		
		embxStatus = EMBX_Offset(transformer->info->handle, alignedBuffer, &offset);
		
		if (!flags || flags == MME_ALLOCATION_PHYSICAL)
		{
			/* 
			 * The user didn't specify any cache options so we have to assume
			 * they wanted affinity with the EMBX heap.
			 *
			 * Query the transport offset result, which also tells us the coherency mapping
			 * of the Heap so we can set the affinity flags correctly
			 */
			if (embxStatus == EMBX_INCOHERENT_MEMORY)
				flags |= MME_ALLOCATION_CACHED;
			else
				flags |= MME_ALLOCATION_UNCACHED;
		}

		MME_Info(MME_INFO_LINUX_MODULE,
			 (DEBUG_NOTIFY_STR "Converted EMBX data addr %p, flags 0x%x to offset 0x%08x\n", 
			  alignedBuffer, flags, offset));
	}
	else
	{
		/* Local transformer: Convert offset to physical for later mmap() call */
		mmeStatus = EMBX_OS_VirtToPhys(alignedBuffer, &offset);
		
		if (mmeStatus != (enum MME_ERROR)EMBX_SUCCESS && mmeStatus != (enum MME_ERROR)EMBX_INCOHERENT_MEMORY) {
			MME_Info(MME_INFO_LINUX_MODULE, 
				 (DEBUG_NOTIFY_STR "Failed to convert addr 0x%08x to physical\n", (int)alignedBuffer));
			result = -EINVAL;
			goto exit;
		}
	}

	onEmbxHeap = (transformer->info)?1:0;
        if (put_user(offset,             &userAllocInfo->offset)       || 
            put_user(mapsize,            &userAllocInfo->mapSize)      ||
            put_user((int)allocAddress,  &userAllocInfo->allocAddress) ||
            put_user(onEmbxHeap,         &userAllocInfo->onEmbxHeap)   ||
            put_user(MME_SUCCESS,        &userAllocInfo->status)) {
		result = -EFAULT;
		goto exit;
	}
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "offset 0x%08x, mapsize 0x%08x\n",
                 (int)offset, (int)mapsize));

	/* Stash the allocated buffer info and save it on a linked list */
	allocBuf->offset       = offset;
	allocBuf->allocAddress = allocAddress;
	allocBuf->size         = mapsize;
	allocBuf->flags        = flags;
	allocBuf->transformer  = transformer;
	allocBuf->next         = instanceInfo->allocatedBufs;
	
	instanceInfo->allocatedBufs = allocBuf;
exit:

	if (result) {
		/* Failure cases */
		
		if (allocAddress) {
			if (transformer->info)
				EMBX_Free(allocAddress);
			else
				bigphysarea_free_pages(allocAddress);
		}
		
		if (allocBuf)
			EMBX_OS_MemFree(allocBuf);
	}

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "<<<< (%d)\n", result));		
	return result;

}
#undef DEBUG_FUNC_STR 

/* ========================================================================== */

#define DEBUG_FUNC_STR "FreeDataBuffer"
static int FreeDataBuffer(struct file* filp, unsigned long arg)
{
	InstanceInfo_t*   instanceInfo = (InstanceInfo_t*)filp->private_data;
	FreeDataBuffer_t* userFreeInfo = (FreeDataBuffer_t*)arg;
	void*             buffer;
	int               onEmbxHeap;
	int               result = -EINVAL;

	AllocBuf_t        **bufp;
	
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR ">>>>\n"));

        if (get_user(buffer, &userFreeInfo->buffer)) {
		result = -EFAULT;
		goto exit;
	}
        if (get_user(onEmbxHeap, &userFreeInfo->onEmbxHeap)) {
		result = -EFAULT;
		goto exit;
	}

	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "buffer %p\n", buffer));

	/* Find the specified buffer on the allocated list */
	for (bufp = &instanceInfo->allocatedBufs; *bufp; bufp = &(*bufp)->next)
	{
		AllocBuf_t *buf = *bufp;

		if (buf->allocAddress == buffer)
		{
			/* Found a match so remove it from the list */
			(*bufp) = buf->next;
			
			/* Free off the allocBuf container */
			EMBX_OS_MemFree(buf);

			if (onEmbxHeap) {
				if (EMBX_SUCCESS != EMBX_Free(buffer)) {
					result = -ENOMEM;
					goto exit;
				}
			} else {
				bigphysarea_free_pages(buffer);
			}
			
			if (put_user(MME_SUCCESS, &userFreeInfo->status)) {
				result = -EFAULT;
				goto exit;
			}
			
			result = 0;
			
			break;
		}
	}
	
exit:	
	MME_Info(MME_INFO_LINUX_MODULE, (DEBUG_NOTIFY_STR "<<<< (%d)\n", result));
        return result;
}
#undef DEBUG_FUNC_STR 

/*
 * Local Variables:
 *  tab-width: 8
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 */
