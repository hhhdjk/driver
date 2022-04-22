#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_irq.h>
#include <asm-generic/gpio.h>
#include <asm/gpio.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

int 	major;		/* 主设备号 */
int 	minor;		/* 次设备还 */
dev_t 	d_id;		/* 设备id */
int 	gpio;		/* 哪一个gpio */
int 	irq ; 		/* gpio中断 */
int 	time_out;	/* 超时处理 */
int 	Flag_dat;	/* 数据有无标志 */

struct cdev 		d_cdev;			/* 字符设备 */
struct class 		*d_class;		/* 字符设备类 */
struct device 		*d_device;		/* 字符设备节点 */
struct timer_list	timer;			/* 软定时器 */
wait_queue_head_t	r_wait;			/* 等待队列头 */
struct work_struct 	work;			/* 中断下半部处理--工作队列 */
spinlock_t			lock;			/* 自旋锁 */

#define		D_NAME	"DS18B20."		/* 字符设备名 */
#define 	minor_num	1	
#define 	HIGH		1
#define 	LOW			0	
#define		Conver_Time	760
#define 	START_CONV	1
#define 	STOP_CONV	0


static char	DS18B20_reset (void)
{
	u8 D_ACK = LOW;
	time_out = 24;
	gpio_direction_output(gpio , LOW);
	gpio_set_value(gpio , LOW) ;
	/* delay 600us */
	udelay(600);
	gpio_set_value(gpio , HIGH);
	udelay(15);
	gpio_direction_input(gpio);
	while( gpio_get_value(gpio) ){
		udelay(10);
		time_out--;
		if(time_out < 0)
			return 1;	
	}
	time_out = 24;
	while( !gpio_get_value(gpio) ){
		udelay(10);
		time_out--;
		if(time_out < 0){
			printk("error\r\n");
			return 1;
		}		
	}
	//D_ACK = gpio_get_value(gpio);
	return D_ACK;
} 

static void DS18B20_write(u8 data)
{	
	u8 i ;
	u8	tmp = data;
	gpio_direction_output(gpio , LOW);
	for(i=0 ; i<8 ; i++ ){	
		tmp  = data&0x01;
        data = data>>1;
		if(  tmp ){
			gpio_set_value(gpio , LOW);
			udelay(2);
			gpio_set_value(gpio , HIGH);
			udelay(60);
		}else{
			gpio_set_value(gpio , LOW);
			udelay(60);
			gpio_set_value(gpio , HIGH);
			udelay(2);
		}
	}
}

static u8 DS18B20_Read_Bit(void) 			 // read one bit
{
  u8 data;
	gpio_direction_output(gpio , LOW);
  gpio_set_value(gpio , LOW); 
	udelay(2);
  gpio_set_value(gpio , HIGH);
	gpio_direction_input(gpio);
	udelay(12);
	if( gpio_get_value(gpio) ) data=1;
  else data=0;	 
  udelay(50);           
  return data;
}

static u8 DS18B20_read(void)    // read one byte
{        
    u8 i,j,dat;
    dat=0;
	for (i=1;i<=8;i++) 
	{
        j=DS18B20_Read_Bit();
        dat=(j<<7)|(dat>>1);
    }						    
    return dat;
}

/* 开始温度转换 */
void DS18B20_Start(void)	
{   						               
    DS18B20_reset();	 
    DS18B20_write(0xcc);		// skip rom
    DS18B20_write(0x44);		// convert
} 

static int DS18B20_init(void)
{
	int err ;
	err = DS18B20_reset();
	if( err != 0){
		printk(KERN_ERR"ds18b20 reset failed\r\n");
		return -EINVAL;
	}else printk(KERN_INFO"ds18b20 reset ok\r\n");

	return 0;
}

short DS18B20_Get_Temp(void)
{
    u8 temp;
    u8 TL,TH;
	short tem;
    DS18B20_reset(); 
    DS18B20_write(0xcc);			// skip rom
    DS18B20_write(0xbe);			// convert	    
    TL=DS18B20_read(); 				// LSB   
    TH=DS18B20_read(); 				// MSB   
    if(TH>7)
    {
        TH=~TH;
        TL=~TL; 
        temp=0;
    }else temp=1;	  	  
    tem=TH;
    tem<<=8;    
    tem+=TL;    
	if(temp)return tem; 
	else return -tem;    
}

/* 使用内核定时器控制转换时间,12位精度转换时间为750ms */
static void time_fn (unsigned long arg)
{
	Flag_dat = true;
	wake_up_interruptible(&r_wait);
} 

// static void work_handler(struct work_struct *work )
// {
// 	mod_timer(&timer , jiffies + msecs_to_jiffies(Conver_Time));	// 开启定时器等待转换
// }

int p_open (struct inode *inode, struct file *file)
{
	int err;
	printk("open ok\r\n");
	err = DS18B20_init();
	if(err)
		return -EINVAL;

	Flag_dat = false;
	init_timer(&timer);
	timer.expires	= jiffies + msecs_to_jiffies(Conver_Time);
	timer.function	= time_fn;

	return 0;
}

int p_release (struct inode *inode, struct file *file)
{
	printk("release ok\r\n");
	del_timer(&timer);
	return 0;
}

ssize_t p_read (struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	short temp = 0;
	unsigned long irqflag;
	/* 定义等待队列项 */
	DECLARE_WAITQUEUE(wait , current);
	
	/* 设置可被信号打断 */
	set_current_state(TASK_INTERRUPTIBLE);

	/* 将当前进程添加到等待队列 */
	add_wait_queue(&r_wait , &wait);

	if(Flag_dat == false){
		schedule();
		set_current_state(TASK_RUNNING);
		remove_wait_queue(&r_wait , &wait);

		if(signal_pending(current)){
			return -ERESTARTSYS;
		}
	}

	if(Flag_dat == true){
		spin_lock_irqsave(&lock , irqflag);
		temp = DS18B20_Get_Temp();
		spin_unlock_irqrestore(&lock , irqflag);
		if( 0 != copy_to_user(buf , &temp , sizeof(short)) ){
			printk("copy_to_user error\r\n");
		}
	}

	Flag_dat = false;
	return count;
}
ssize_t p_write (struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	int conver_start = 0;
	unsigned long irqflag;
	if( copy_from_user (&conver_start , buf , sizeof(int)) ) {
		printk("copy_from_user error\r\n");
	}

	if( conver_start == START_CONV ){
		spin_lock_irqsave(&lock , irqflag);
		DS18B20_Start();               // ds1820 start convert
		//schedule_work(&work);
		mod_timer(&timer , jiffies + msecs_to_jiffies(Conver_Time));	// 开启定时器等待转换
		spin_unlock_irqrestore(&lock , irqflag);
	}else if( conver_start == STOP_CONV ){
		//del_timer_sync(&timer);
	}

	return count;
}

static struct file_operations f_operations = {
	.owner		= THIS_MODULE,
	.read 		= p_read,
	.write 		= p_write,
	.open 		= p_open,
	.release 	= p_release,
};


int p_probe(struct platform_device *pdev)
{
	int err;
	int i, j;
	struct device *dev;
	struct device_node	*nd;
	dev = &pdev->dev;
	nd = dev->of_node;
	
	printk("led driver probe called pdev %d\r\n" , pdev->num_resources);

	/* 获取gpio */
	gpio = of_get_named_gpio(nd, "ds18b20,gpio", 0);
	if (!gpio_is_valid(gpio)) {
		dev_err(dev, "cannot get ds18b20,gpio %d\n", gpio);
		return -ENODEV;
	}

	/* 获取irq */
	irq = platform_get_irq(pdev , 0);
	if(irq < 0){
		printk("platform_get_irq error\r\n");
	}else printk("irq = %d , gpio = %d\r\n" , irq , gpio);

	/* 请求gpio */
	err = devm_gpio_request(dev, gpio, "ds18b20");
	if(err)
		printk(KERN_ERR"gpio request error\r\n");


	/* 创建设备 */
	for(i=0 ; i < minor_num ; i++){
		d_device = device_create(d_class , NULL , MKDEV(major , i) , NULL , D_NAME"%d" , i);
		if(IS_ERR(d_device)){
			for(j=0 ; j < i; j++){
				device_destroy(d_class , MKDEV(major , j));
			}
				class_destroy(d_class);
				cdev_del(&d_cdev);
				unregister_chrdev_region(d_id , 1);
		}
	}

	/* 初始化工作队列 */
	// INIT_WORK(&work , work_handler);

	/* 初始化等待队列头 */
	init_waitqueue_head(&r_wait);

	/* 初始化自旋锁 */
	spin_lock_init(&lock);

	return 0;
}
int p_remove(struct platform_device *pdev)
{
	printk("p_remove called\n");
	devm_gpio_free(&pdev->dev, gpio);
	device_destroy(d_class , MKDEV(major , 0));
	class_destroy(d_class);
	cdev_del(&d_cdev);
	unregister_chrdev_region(d_id , 1);
	return 0;
}


static struct of_device_id pp_test[] = {
	{.compatible = "nexell,ds18b20"},
	{/* sentinel */}
};

static struct platform_driver led = {
	.probe		= p_probe,
	.remove		= p_remove,
	.driver = {
		.name = "nexell,ds18b20",
		.of_match_table = pp_test,
	},
};


static int __init pf_init(void)
{
	int ret;
	/* 设备号 */
	if(major){
				d_id = MKDEV(major , minor);
				ret = register_chrdev_region(d_id , 1 ,D_NAME);
	}else{
				ret = alloc_chrdev_region(&d_id , 0 ,minor_num, D_NAME);
				major = MAJOR(d_id);
				minor = MINOR(d_id);
	}
	if(ret  < 0){
				printk("register fail!\n");
				return -EINVAL;
	}
	printk("major=%d, minor=%d\n",major, minor);

	/* 注册字符设备 */
	d_cdev.owner = f_operations.owner;
	cdev_init(&d_cdev , &f_operations);
	if( cdev_add(&d_cdev ,d_id ,  1) ){
		printk("cdev_add err\r\n");
		unregister_chrdev_region(d_id , 1);
		return -EINVAL;
	}

	/* 创建设备类 */
	d_class = class_create(THIS_MODULE , "DS18B20_CLASS");
	if(IS_ERR(d_class)){
		printk("class_create err\r\n");
		cdev_del(&d_cdev);
		unregister_chrdev_region(d_id , 1);
		return PTR_ERR(d_class);
	}
	
	return platform_driver_register(&led);
}

static void __exit pf_exit(void)
{	
	platform_driver_unregister(&led);
	return;
}

module_init(pf_init);
module_exit(pf_exit);

MODULE_AUTHOR("hzy");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ds18b20 driver");
MODULE_VERSION("1.1");


