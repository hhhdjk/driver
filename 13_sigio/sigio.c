#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <dt-bindings/soc/s5p4418-base.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fcntl.h>
#include <linux/signal.h>


#define s5pxx18_led_MAJOR 101
#define s5pxx18_led_MINOR 2
#define s5pxx18_led_device_name "key"
#define KEYON		1
#define KEYOFF		0

struct s5pxx18_led_cdrv 
{
	dev_t dev_id;
	int MAJOR;
	int MINOR;
	void * s5pxx18_led_prv;
} *s5pxx18_led_device;

struct key_desc
{
    int gpio;
    char *name;
};

struct key_desc key_info[] = {
    [0] = {
            .gpio = 63,
            .name = "K1",
        },
};

struct cdev s5pxx18_led_cdev;			/* 字符设备初始化 */
struct class *s5pxx18_led_class;		/* 该设备类 */
struct device * s5pxx18_led_devices;	/* 该类下设备 */
struct device_node *nd; 				/* 设备节点 */
struct property *comppro; 				/* 节点属性值 */
struct timer_list time;					/* 内核定时器 */
struct work_struct key_work;			/* 中断下半部处理--工作队列方式 */
wait_queue_head_t	r_wait;				/* 等待队列头 */
struct fasync_struct *fasync;			/* 异步通知 */

char led_statue[] = {0,};				/*  */
spinlock_t lock;						/* 自旋锁 */
static atomic_t	kvalue;					/* 键值 */
int gpio_num;							/* gpio编号 */
static int 	rvalue;						/* 键值 */

struct key_desc *Pdata = NULL;

static void timer_fn(unsigned long arg)
{
	struct key_desc *pdata = (struct key_desc *)arg;
    int status = gpio_get_value(pdata->gpio);
	if(status == 0){
		atomic_set(&kvalue , KEYON);
	}else if(status == 1) {
		rvalue = 1;
	}

	if(rvalue)
		kill_fasync(&fasync , SIGIO , POLL_IN);
		//wake_up_interruptible(&r_wait);
}
		

static void work_handler(struct work_struct *work )
{
	time.data = (unsigned long)Pdata;
	Pdata = NULL;
	mod_timer(&time , jiffies + msecs_to_jiffies(12));
}

static irqreturn_t IRQ_handler(int irq, void *dev_id)
{
	spin_lock(&lock);
	Pdata = (struct key_desc * )dev_id;
	spin_unlock(&lock);
	schedule_work(&key_work);
    return IRQ_HANDLED;
}


static int init_key(void)
{

	int err;
    int i , j , irq_num;
	/* 获取设备树节点 */
    nd = of_find_node_by_path("/gpio-leds/led@2");
	if(nd == NULL)
	{
		printk(KERN_WARNING "of_find_node_by_path get node failed\r\n");
		err  = -EINVAL;
		goto failed_nd;
	}    

    /* 获取LED所对应的GPIO */
    gpio_num = of_get_named_gpio(nd, "gpios", 0);
    if(gpio_num < 0)
    {
        printk("can not find led gpio\r\n");
		err  = -EINVAL;
		goto failed_nd;
    }
    printk("gpio num=%d\r\n", gpio_num);


    err = gpio_request(gpio_num, "mygpio");
    if(err)
    {
        printk("Faile to request the led gpio\r\n");
    }else {
        printk("request gpio ok!\r\n");
    }

    /* 使用IO，设置为输入或输出 */
    err = gpio_direction_input(gpio_num);
    if(err)
    {
         printk("Faile to gpio_direction_output the led gpio\r\n");
		 goto free_gpio;
    }

    //申请中断
    for(i = 0 ; i < ARRAY_SIZE(key_info); i++){
        irq_num=gpio_to_irq(key_info[i].gpio);
        printk("irq_num:%d\r\n", irq_num);

        err = request_irq( irq_num,   //中断编号
                            IRQ_handler ,                   //中断回调函数
                            IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, //触发方式
                            key_info[i].name,               //中断名称
                            &key_info[i]);                              //数据参数

        if(err < 0){
            printk("request failed\r\n");
            goto request_irq_failed;
        }
    }

	return 0;

request_irq_failed:
    for(j = 0 ; j< i ; j++){
        free_irq(gpio_to_irq(key_info[j].gpio) , &key_info[j]);
    }

free_gpio:
	gpio_free(gpio_num);

failed_nd:
	return err;

}

int s5pxx18_led_open (struct inode *node, struct file *file)
{
	int err;
	printk("key APP open\r\n");
	err = init_key();
	if(err){
		printk("init_key failed\r\n");
		return -EINVAL;
	}

	return 0;
}

ssize_t s5pxx18_led_read (struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	int value = 0;
#if 0
	// wait_event_interruptible(r_wait ,  atomic_read(&kvalue));	//事件
	DECLARE_WAITQUEUE(wait , current);		// 定义等待队列项
	set_current_state(TASK_INTERRUPTIBLE);	// 设置可被信号打断状态
	add_wait_queue(&r_wait , &wait);		// 添加到等待队列头
	
	if(rvalue != 1){
		schedule();							// 进程切换
		set_current_state(TASK_RUNNING);
		remove_wait_queue(&r_wait, &wait);

		if(signal_pending(current)){
			return -ERESTARTSYS;
		}
	}
#endif	

	if(rvalue == 1) {
		value = atomic_read(&kvalue);
		if( 0 != (copy_to_user(buf , &value , 4))){
			printk("kernel read error\r\n");
			return -EINVAL;
		}
	}	
	rvalue = 0;					
	atomic_set(&kvalue , KEYOFF);
	
	return 0; 
}


ssize_t s5pxx18_led_write (struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{

	return size;
}

int key_fasync (int fd , struct file *file, int on);
int s5pxx18_led_release (struct inode *node, struct file *file)
{
	printk("keyAPP close\r\n");	
	gpio_free(gpio_num);
	key_fasync ( -1 , file, 0);
	return 0;
}

long s5pxx18_led_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
	printk("APP s5pxx18_led_ioctl\r\n");

	return 0;
}


unsigned int key_poll (struct file *file, struct poll_table_struct * wait)
{
	unsigned int mask = 0;

	/* 会将应用进程加入等待队列 (加入等待队列后并不会立即休眠，与mask返回值有关) */
	poll_wait(file , &r_wait , wait);
	printk("do_poll\r\n");
	if(rvalue == 1){
		mask |= POLLIN | POLLRDNORM;
	}
	return mask;
}

int key_fasync (int fd , struct file *file, int on)
{
	printk("key_fasync\r\n");
	return fasync_helper(fd , file , on , &fasync);
}

struct file_operations s5pxx18_led_ops = {
	.owner 			= THIS_MODULE,
	.open 			= s5pxx18_led_open,
	.read 			= s5pxx18_led_read,
	.write 			= s5pxx18_led_write,
	.release 		= s5pxx18_led_release,
	.unlocked_ioctl = s5pxx18_led_ioctl,
	.poll			= key_poll,
	.fasync			= key_fasync,
};



static int s5pxx18_led_register_cdrv( struct s5pxx18_led_cdrv* led_drv )
{
	int err = 0;
	struct s5pxx18_led_cdrv* led = led_drv;

	if(led->MAJOR)
	{
		if( register_chrdev_region(led->dev_id , 1 , s5pxx18_led_device_name) )
			goto dev_id;
	}
	else
	{
		if( alloc_chrdev_region(&led->dev_id , 0 , 1 , s5pxx18_led_device_name) ){
			goto dev_id;
		}else{
			led->MAJOR = MAJOR(led->dev_id);
			led->MINOR = MINOR(led->dev_id);
		}
	}
	
	s5pxx18_led_cdev.owner = s5pxx18_led_ops.owner;
	cdev_init(&s5pxx18_led_cdev, &s5pxx18_led_ops);

	if( cdev_add(&s5pxx18_led_cdev ,  led->dev_id , 1) ){
		goto cdev_add_failed;
	}

	//自动创建设备节点
	s5pxx18_led_class = class_create(s5pxx18_led_ops.owner, s5pxx18_led_device_name);
	if (IS_ERR(s5pxx18_led_class)) {
		err = PTR_ERR(s5pxx18_led_class);
		goto out_chrdev;
	}

	s5pxx18_led_devices = device_create(
		s5pxx18_led_class, 
		NULL , 
		led->dev_id, 
		NULL , 
		s5pxx18_led_device_name );
	
	if (IS_ERR(s5pxx18_led_devices)) {
		err = PTR_ERR(s5pxx18_led_devices);
		goto out_devices;
	}

	/* 初始化自旋锁 */
	spin_lock_init(&lock);

	/* 初始化原子变量 */
	atomic_set(&kvalue, KEYOFF);

	//初始化定时器
	init_timer(&time);
	time.function = timer_fn;
	time.expires = jiffies + msecs_to_jiffies(0);

	/* 初始化工作队列 */
	INIT_WORK(&key_work , work_handler);

	/* 初始化等待队列头 */
	init_waitqueue_head(&r_wait);

	return 0;

out_devices:
	class_destroy(s5pxx18_led_class);

out_chrdev:
	cdev_del(&s5pxx18_led_cdev);

cdev_add_failed:
	unregister_chrdev_region(led->dev_id , 1);

dev_id:
	return err;

}

static void s5pxx18_led_unregister_cdrv(struct s5pxx18_led_cdrv* led_drv)
{

	device_destroy(s5pxx18_led_class , led_drv->dev_id);
	class_destroy(s5pxx18_led_class);
	cdev_del(&s5pxx18_led_cdev);
	unregister_chrdev_region(led_drv->dev_id , 1);
}


static int s5pxx18_led_cdrv_init( void )
{
	int err =0;
	printk("s5pxx18_led_cdrv_init\r\n");
	s5pxx18_led_device = kzalloc( sizeof(struct s5pxx18_led_cdrv), GFP_KERNEL);
	if( !s5pxx18_led_device ){
		printk(KERN_WARNING s5pxx18_led_device_name "can't get kzalloc\r\n");
		goto failed;
	}
	s5pxx18_led_device->MAJOR = s5pxx18_led_MAJOR;
	s5pxx18_led_device->MINOR = s5pxx18_led_MINOR;
	s5pxx18_led_device->dev_id = MKDEV(s5pxx18_led_MAJOR, s5pxx18_led_MINOR);

	if( -EINVAL == (err = s5pxx18_led_register_cdrv( s5pxx18_led_device )) ){
		printk(KERN_WARNING s5pxx18_led_device_name "s5pxx18_led_register_cdrv register failed!\r\n");
		goto register_failed;
	}

	return err;

register_failed:
	;

failed:
	kfree(s5pxx18_led_device);
	return -EINVAL;

}


static int __init s5pxx18_led_drv_init(void)
{
	printk("led_drv_init\r\n");
	s5pxx18_led_cdrv_init();
	return 0;
}

static void __exit s5pxx18_led_drv_exit(void)
{
    int i;
	printk("led_drv_exit\r\n");
	s5pxx18_led_unregister_cdrv(s5pxx18_led_device);

    for(i = 0 ; i< ARRAY_SIZE(key_info) ; i++){
        free_irq(gpio_to_irq(key_info[i].gpio) , &key_info[i]);
    }
    kfree(s5pxx18_led_device);
	return;
}

module_init(s5pxx18_led_drv_init);
module_exit(s5pxx18_led_drv_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("HUANGZHUYU");	 				

