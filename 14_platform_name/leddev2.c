#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <dt-bindings/soc/s5p4418-base.h>
#include <linux/platform_device.h>

#define  GpioB_Out		   (PHYS_BASE_GPIOB)
#define  GpioB_OutEnb	   (PHYS_BASE_GPIOB+0x04)
#define  GpioB_ALTFN1	   (PHYS_BASE_GPIOB+0X24)


void	led_release(struct device *dev)
{
    printk("platform test release\r\n");
    return ;
}

static struct resource led_res[] = {
    [0] = {
        .name   = "led_mem_resource",
        .start  = GpioB_Out,
        .end    = GpioB_Out + 0x4 - 0x1,
        .flags  = IORESOURCE_MEM,
    },
    [1] = {
        .name   = "led_mem_resource",
        .start  = GpioB_OutEnb,
        .end    = GpioB_OutEnb + 0x4 - 0x1,
        .flags  = IORESOURCE_MEM,
    },
    [2] = {
        .name   = "led_mem_resource",
        .start  = GpioB_ALTFN1,
        .end    = GpioB_ALTFN1 + 0x4 - 0x1,
        .flags  = IORESOURCE_MEM,
    },

};


static struct platform_device led = {
    .name = "Nanopi-led",
    .id = 1,
    .resource = led_res,
    .num_resources = ARRAY_SIZE(led_res),
    .dev = {
        .release = led_release,
    },
};

static int __init platform_test_init(void)
{
    printk("platform_test_init\r\n");
    
    return platform_device_register(&led);
}

static void __exit platform_test_exit(void)
{
    printk("platform_test_exit\r\n");
    platform_device_unregister(&led);
    return;
} 

module_init(platform_test_init);
module_exit(platform_test_exit);

MODULE_AUTHOR("hzy");
MODULE_DESCRIPTION("this is a platform test");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

