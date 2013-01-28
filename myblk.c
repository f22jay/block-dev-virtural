
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h>   /* kmalloc() */
#include <linux/fs.h>   /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/timer.h>
#include <linux/types.h> /* size_t */
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/hdreg.h> /* HDIO_GETGEO */
#include <linux/kdev_t.h>
#include <linux/vmalloc.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h> /* invalidate_bdev */
#include <linux/bio.h>

static int sbull_major = 0;
static int nsectors= 4096;
static int ndevices=2;
static int hardsector_size = 512;
static struct request_queue *blkdev_queue=NULL;
static struct gendisk *blkdev_disk;
static char *data=NULL;
static int dev_size=512*4096;

#define BLKDEV_NAME "sbull"
#define KERNEL_SECTOR_SIZE 512
spinlock_t lock;                /* For mutual exclusion */



struct sbull{
		int size;
		char *data;
		short users;
        short media_change;             /* Flag a media change? */
        spinlock_t lock;                /* For mutual exclusion */
        struct request_queue *queue;    /* The device request queue */
        struct gendisk *gd;             /* The gendisk structure */
		};

		
static void sbull_transfer( unsigned long sector,
   unsigned long nsect, char *buffer, int write)
{
unsigned long offset = sector*hardsector_size;
unsigned long nbytes = nsect*hardsector_size;

if ((offset + nbytes) > dev_size) {
   printk (KERN_NOTICE "Beyond-end write (%ld %ld)\n", offset, nbytes);
   return;
}
if (write)
   memcpy(data + offset, buffer, nbytes);
else
   memcpy(buffer, data + offset, nbytes);
}

/*
* The simple form of the request function.
*/
static void sbull_request(struct request_queue *q)
{
struct request *req;

req = blk_fetch_request(q);
while (req != NULL) {
  // struct sbull_dev *dev = req->rq_disk->private_data;
   if (! blk_fs_request(req)) {
    printk (KERN_NOTICE "Skip non-fs request\n");
    __blk_end_request_all(req, -EIO);
    continue;
   }
    //    printk (KERN_NOTICE "Req dev %d dir %ld sec %ld, nr %d f %lx\n",
    //        dev - Devices, rq_data_dir(req),
    //        req->sector, req->current_nr_sectors,
    //        req->flags);
   sbull_transfer( blk_rq_pos(req), blk_rq_cur_sectors(req), req->buffer, rq_data_dir(req));
   /* end_request(req, 1); */
   if(!__blk_end_request_cur(req, 0)) {
    req = blk_fetch_request(q);
   }
}
}


/*
* The device operations structure.
*/
static struct block_device_operations sbull_ops = {
.owner           = THIS_MODULE,
};



static int __init sbull_init(void)
{
/*
* Get registered.
*/
	int ret=0;
	data=vmalloc(nsectors*hardsector_size);
	if(!data)
	{
		printk("data vmalloc failed\n");
	    goto out_unregister;
	}
	sbull_major = register_blkdev(sbull_major, "sbull");
	if (sbull_major <= 0) {
    printk(KERN_WARNING "sbull: unable to get major number\n");
	   goto out_unregister;
   }
   blkdev_queue=blk_init_queue(sbull_request,&lock);
   blkdev_disk=alloc_disk(1);
   if(!blkdev_disk)	
   {
	   ret=-ENOMEM;
	   printk(KERN_WARNING "sbull: unable to get major gendisk\n");
	   goto out_unregister;
   }
   strcpy(blkdev_disk->disk_name,BLKDEV_NAME);
   blkdev_disk->major=sbull_major;
   blkdev_disk->first_minor=0;
   blkdev_disk->fops=&sbull_ops;
   blkdev_disk->queue=blkdev_queue;
   set_capacity(blkdev_disk,nsectors);
   add_disk(blkdev_disk);

   return 0;

out_unregister:
unregister_blkdev(sbull_major, BLKDEV_NAME);
return -ENOMEM;
}

static void sbull_exit(void)
{

	del_gendisk(blkdev_disk);
    put_disk(blkdev_disk);
   	if (blkdev_queue)
		 blk_cleanup_queue(blkdev_queue);
	unregister_blkdev(sbull_major, "sbull");
}

module_init(sbull_init);
module_exit(sbull_exit);

