#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <dt-bindings/soc/s5p4418-base.h>

#define LED_MAJOR 200			//主设备号200
#define LED_NAME "LED"	//设备名字

#define  GpioB_Out		   (PHYS_BASE_GPIOB)
#define  GpioB_OutEnb	   (PHYS_BASE_GPIOB+0x04)
#define  GpioB_ALTFN0	   (PHYS_BASE_GPIOB+0X20)
#define  GpioB_ALTFN1	   (PHYS_BASE_GPIOB+0X24)

//物理地址映射后得到的虚拟地址指针
static void __iomem *GPIOB_Out;
static void __iomem *GPIOB_OutEnb;
static void __iomem *GPIOB_ALTFN0;
static void __iomem *GPIOB_ALTFN1;

/* Opens the device. */
static int led_open(struct inode * inode, struct file * file)
{
	printk("chrdevbase_open\r\n");
	return 0;

}

/* Closes the device. */
static int led_close(struct inode * inode, struct file * file)
{
	printk("chrdevbase_release\r\n");
  /* do nothing for now */
  return 0;
}

/* Writes data to eeprom. */
static ssize_t led_write(struct file * file, __user char * buf, size_t count, loff_t *ppos)
{
	printk("chrdevbase_write\r\n");

	
 	return 0;
}

/* 
 * 字符设备操作集合
 */
static const struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.write = led_write,
	.release = led_close
};

/* 
 * 入口
 */
static int __init led_init(void)
{
	printk("led init\n");
	unsigned int val = 0;
	/* GPIO初始化
	 * PHYS_BASE_GPIOB		(0xC001B000)
	 * #define  GpioB_Out		   (PHYS_BASE_GPIOB)
	 * #define  GpioB_OutEnb	   (PHYS_BASE_GPIOB+0x04))
     * #define  GpioB_ALTFN0	   (PHYS_BASE_GPIOB+0X20))
     * #define  GpioB_ALTFN1	   (PHYS_BASE_GPIOB+0X24))
	 */
	GPIOB_Out = ioremap(GpioB_Out, 4);
	GPIOB_OutEnb = ioremap(GpioB_OutEnb, 4);
	GPIOB_ALTFN0 = ioremap(GpioB_ALTFN0, 4);
	GPIOB_ALTFN1 = ioremap(GpioB_ALTFN1, 4);

	//初始化
	val = readl(GPIOB_ALTFN0);	//读寄存器
	val&=~(0X3<<24);			//清寄存器
	val|=(0x2<<24);				//设置寄存器值
	writel(val, GPIOB_ALTFN0);	//写寄存器

	val = readl(GPIOB_OutEnb);
	val|=(1<<12);
	writel(val, GPIOB_OutEnb);
	unsigned long s = 1000000;
	 
	val = readl(GPIOB_Out);
	val&=~(1<<12);
	writel(val, GPIOB_Out);

	while (s > 1)
	{
		s--;
	}

	int ret =0;
	//注册字符设备
	ret = register_chrdev(LED_MAJOR, LED_NAME, &led_fops);
	if(ret < 0)
	{
		printk("register fail!\n");
		return -EIO;
	}
	printk("led inited\n");
	return 0;
}

/* 
 * 出口
 */
static void __exit led_exit(void)
{
	unsigned int val = 0;
	unsigned long s = 10000000;
	val = readl(GPIOB_Out);
	val|=(1<<12);
	writel(val, GPIOB_Out);

	//取消地址映射
	iounmap(GPIOB_Out);
	iounmap(GPIOB_OutEnb);
	iounmap(GPIOB_ALTFN0);
	iounmap(GPIOB_ALTFN1);

	unregister_chrdev(LED_MAJOR, LED_NAME);
	printk("led exit\n");
	return;
}

/* 
 * 注册驱动加载和卸载
 */

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("HUANGZHUYU");

