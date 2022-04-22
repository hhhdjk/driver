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
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/reset.h>

int 	major;		/* 主设备号 */
int 	minor;		/* 次设备还 */
dev_t 	d_id;		/* 设备id */
int 	Gpio_Trig;		/* 哪一个gpio */
int 	Gpio_Echo;		/* 哪一个gpio */
int 	irq ; 		/* gpio中断 */
int 	time_out;	/* 超时处理 */
int 	Flag_dat;	/* 数据有无标志 */
void __iomem *base;	/* va */
// u64		Sdata;
// u64		Edata;
// u64		Ddata;

struct clk 			*base_clk;		/* clocks */
struct cdev 		d_cdev;			/* 字符设备 */
struct class 		*d_class;		/* 字符设备类 */
struct device 		*d_device;		/* 字符设备节点 */
wait_queue_head_t	r_wait;			/* 等待队列头 */
struct work_struct 	work;			/* 中断下半部处理--工作队列 */
spinlock_t			lock;			/* 自旋锁 */

#define		D_NAME	"Hc_Sr04."		/* 字符设备名 */
#define 	minor_num	1	
#define 	HIGH		1
#define 	LOW			0	


void __iomem *TIMER_TCFG0;
void __iomem *TIMER_TCFG1;
void __iomem *TIMER_TCON;
void __iomem *TIMER_TCNTB4;	  	
void __iomem *TIMER_TCNTO4;	  
void __iomem *IP_RSTCON1;		

int p_open (struct inode *inode, struct file *file)
{
	int val;
	printk("open ok\r\n");
	val = readl(TIMER_TCFG0);
	val &= ~(0xff<<8);
	val |= ((200-1)<<8);
	writel(val , TIMER_TCFG0);
	val = readl(TIMER_TCFG1);
	val &= ~(0xf<<16);
	val |= (0x0<<16);
	writel(val , TIMER_TCFG1);
	Flag_dat = false;
	return 0;
}

int p_release (struct inode *inode, struct file *file)
{
	printk("release ok\r\n");
	return 0;
}

ssize_t p_read (struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	int temp = 0;
	int val , reg;
	int time;

	writel(40000 , TIMER_TCNTB4);
	reg = readl(TIMER_TCON);
	reg |= (1<<21);
	writel(reg,TIMER_TCON);
	reg = readl(TIMER_TCON);
	reg &= ~(1<<21);
	writel(reg,TIMER_TCON);

	gpio_set_value(Gpio_Trig , 1);
	udelay(15);
	gpio_set_value(Gpio_Trig , 0);

	//wait_event_interruptible(r_wait , Flag_dat );

	do{
		val = gpio_get_value(Gpio_Echo);
		udelay(1);
		time++;
		if(time > 30000)
			return -EAGAIN;
	}while(val==0);
	reg = readl(TIMER_TCON);
	reg |= ((0<<22) | (1<<20));
	writel(reg,TIMER_TCON);
	time = 0;
	do{
		val = gpio_get_value(Gpio_Echo);
		udelay(1);
		time++;
		if(time > 30000)
			return -EAGAIN;
	}while(val==1);

	reg = readl(TIMER_TCON);
	reg &= ~(1<<20);
	writel(reg,TIMER_TCON);
	temp = 40000 - readl(TIMER_TCNTO4);
	writel(0 , TIMER_TCNTB4);
	if( 0 != copy_to_user(buf , &temp , sizeof(int)) ){
		printk("copy_to_user error\r\n");
	}
	return count;
}
ssize_t p_write (struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	// int conver_start = 0;
	// unsigned long irqflag;
	// if( copy_from_user (&conver_start , buf , sizeof(int)) ) {
	// 	printk("copy_from_user error\r\n");
	// }

	// if( conver_start == START_CONV ){
	// 	spin_lock_irqsave(&lock , irqflag);
	// 	//schedule_work(&work);
	// 	spin_unlock_irqrestore(&lock , irqflag);
	// }else if( conver_start == STOP_CONV ){
	// 	//del_timer_sync(&timer);
	// }

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
	int err ,val;
	int i, j;
	struct device *dev;
	struct device_node	*nd;
	dev = &pdev->dev;
	nd = dev->of_node;
	
	printk("led driver probe called pdev %d\r\n" , pdev->num_resources);

	/* 获取gpio */
	Gpio_Echo = of_get_named_gpio(nd, "Hc_Sr04,gpio_echo", 1);
	if (!gpio_is_valid(Gpio_Echo)) {
		dev_err(dev, "cannot get Hc_Sr04,Gpio_Echo %d\n", Gpio_Echo);
		return -ENODEV;
	}

	/* 获取gpio */
	Gpio_Trig = of_get_named_gpio(nd, "Hc_Sr04,gpio_echo", 0);
	if (!gpio_is_valid(Gpio_Trig)) {
		dev_err(dev, "cannot get Hc_Sr04,gpio_trig %d\n", Gpio_Trig);
		return -ENODEV;
	}

	printk("gpiod = %d , gpioc = %d\r\n" , Gpio_Echo , Gpio_Trig);

	/* 请求gpio */
	err = devm_gpio_request(dev, Gpio_Trig, "Hc_Sr04");
	if(err)
		dev_err(dev, "gpio request error  %d\r\n" ,err );

	err = devm_gpio_request(dev, Gpio_Echo, "Hc_Sr04");
	if(err)
		dev_err(dev, "gpio request error  %d\r\n" ,err );

	/* 请求中断 */
	// err = devm_request_irq(dev,gpio_to_irq(Gpio_Echo) ,Hc_Sr04_irq, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "Hc_Sr04", NULL);
	// if (err) {
	// 	dev_err(dev, "IRQ error \r\n");
	// 	return err;
	// }

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

	TIMER_TCFG0 	= ioremap((0xC0017000 + 0x00) , 4);
	TIMER_TCFG1	 	= ioremap((0xC0017000 + 0x04) , 4);
	TIMER_TCON 		= ioremap((0xC0017000 + 0x08) , 4);
	TIMER_TCNTB4 	= ioremap((0xC0017000 + 0x3C), 4);
	TIMER_TCNTO4 	= ioremap((0xC0017000 + 0x40), 4);
	IP_RSTCON1		= ioremap((0xC0010000 + 0x2004), 4);

	gpio_direction_output(Gpio_Trig , 0);

	gpio_set_value(Gpio_Trig , 0);

	gpio_direction_input(Gpio_Echo);

	return 0;
}
int p_remove(struct platform_device *pdev)
{
	printk("p_remove called\n");
	iounmap(TIMER_TCFG0);
	iounmap(TIMER_TCFG1);
	iounmap(TIMER_TCON);
	iounmap(TIMER_TCNTB4);
	iounmap(TIMER_TCNTO4);
	iounmap(IP_RSTCON1);
	devm_gpio_free(&pdev->dev, Gpio_Trig);
	devm_gpio_free(&pdev->dev, Gpio_Echo);
	//devm_free_irq(&pdev->dev,gpio_to_irq(Gpio_Echo), NULL);
	device_destroy(d_class , MKDEV(major , 0));
	class_destroy(d_class);
	cdev_del(&d_cdev);
	unregister_chrdev_region(d_id , 1);
	return 0;
}


static struct of_device_id pp_test[] = {
	{.compatible = "nexell,s5p4418-ppm,Hc_Sr04"},
	{/* sentinel */}
};

static struct platform_driver led = {
	.probe		= p_probe,
	.remove		= p_remove,
	.driver = {
		.name = "nexell,s5p4418-ppm,Hc_Sr04",
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
	d_class = class_create(THIS_MODULE , "Hc_Sr04_CLASS");
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


