#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <dt-bindings/soc/s5p4418-base.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_irq.h>
#include <asm-generic/gpio.h>
#include <asm/gpio.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>

#define  GpioB_Out		   (PHYS_BASE_GPIOB)
#define  GpioB_OutEnb	   (PHYS_BASE_GPIOB+0x04)
#define  GpioB_ALTFN1	   (PHYS_BASE_GPIOB+0X24)

void leddev_release(struct device *dev)
{
    printk("leddev relaese\r\n");
}

static struct resource leddev_resources[]= 
{
    [0] = {
        .start = GpioB_OutEnb,
        .end   = GpioB_OutEnb + 0x03,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = GpioB_Out,
        .end   = GpioB_Out + 0x03,
        .flags = IORESOURCE_MEM,
    },
    [2] = {
        .start = GpioB_ALTFN1,
        .end   = GpioB_ALTFN1 + 0x03,
        .flags = IORESOURCE_MEM,
    }
};

static struct platform_device leddev = 
{
    .name = "Nanopi2-led",
    .id   = -1,
    .dev   = {
        .release = leddev_release,
    },
    .num_resources = ARRAY_SIZE(leddev_resources),
    .resource = leddev_resources,
};

static int __init leddev_init(void)
{
	return platform_device_register(&leddev);
}

static void __exit leddev_exit(void)
{

	return platform_device_unregister(&leddev);
}

module_init(leddev_init);
module_exit(leddev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("huangzhuyu");