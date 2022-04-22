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
#include <asm/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>

//#include <linux/pinctrl/consumer.h>

/* 
	leds: gpio-leds {
		compatible = "gpio-leds";
		pinctrl-names = "default";
		pinctrl-0 =<&led1_gpio>;

		led@1 {
			gpios = <&gpio_b 12 GPIO_ACTIVE_LOW>;
			label = "status_led";
			linux,default-trigger = "heartbeat";
			linux,default-trigger-delay-ms = <0>;
		};
	};
 */

// struct node {
// 	const char *name;
// 	const char *type;
// 	phandle phandle;
// 	const char *full_name;
// 	struct fwnode_handle fwnode;

// 	struct	property *properties;
// 	struct	property *deadprops;	/* removed properties */
// 	struct	node *parent;
// 	struct	node *child;
// 	struct	node *sibling;
// 	struct	kobject kobj;
// 	unsigned long _flags;
// 	void	*data;
// #if defined(CONFIG_SPARC)
// 	const char *path_component_name;
// 	unsigned int unique_id;
// 	struct of_irq_controller *irq_trans;
// #endif
// };

#define CONTAINER_OF(ptr, type, member) ({(type *)( (char *)(ptr) - offsetof(type,member) );})

int id = -1;
/* 模块入口 */
static int __init dtsof_init(void)
{
	struct property *property;
	struct device_node *node;
	struct device *dev;
	const char * string;
	const char * string2;
	const char * gname;
	unsigned int out_value;
	int err;
	unsigned int *bvalue;
	uint elemsize;
	int i;
	int sda_io , scl_io , irq;

	struct pinctrl *pctrl;
	struct pinctrl_state	*pins_sda_dft;
	
	//int ret;

	node = of_find_node_by_name(NULL, "onewire_pwm");
	if(node == NULL){
		printk(KERN_WARNING "of_find_node_by_name get node failed\r\n");
	}else {
		printk("name:%s type:%s phandle:%d full_name:%s\r\n", 
		node->name, 
		node->type, 
		node->phandle,
		node->full_name);
	}

	property = of_find_property(node,"compatible",NULL);
	if(!property){
		printk(KERN_WARNING "of_find_property get property failed\r\n");
	}else {
		printk("compatible:%s name:%s length:%d\r\n", 
		(char*)property->value, 
		property->name, 
		property->length);
	}

	err = of_property_read_string(node,"pinctrl-names",&string);
	if(err){
		printk(KERN_WARNING "of_property_read_string get property failed\r\n");
	}else {
		printk("pinctrl-names:%s\r\n", string);
	}

	err = of_property_read_u32(node, "interrupts",&out_value);
	if(err){
		printk(KERN_WARNING "of_property_read_u32 get property failed\r\n");
	}else {
		printk("interrupts:%d\r\n", out_value);
	}

	elemsize = of_property_count_elems_of_size(node,"pwms", sizeof(unsigned int));
    if(elemsize < 0){
        printk(KERN_WARNING "of_property_count_elems_of_size get property failed\r\n");
    }
    else
        printk("pwms elem count = %d\r\n", elemsize);
  
    bvalue = (u32*)kmalloc(sizeof(unsigned int )*elemsize, GFP_KERNEL);
    if(!bvalue)
    {
       printk("kmalloc failed\r\n");
    }

	err = of_property_read_u32_array(node,"pwms",bvalue,elemsize);
    if(err < 0)
    	kfree(bvalue);
    else
    	for(i = 0; i<elemsize; i++)
            printk("pwms[%d]=%d\r\n",i, *bvalue++);
    kfree(bvalue);








	node = of_find_node_by_path("/onewire_pwm");
	if(node == NULL){
		printk(KERN_WARNING "of_find_node_by_path get node failed\r\n");
	}else {
		printk("name:%s type:%s phandle:%d full_name:%s\r\n", 
		node->name, 
		node->type, 
		node->phandle,
		node->full_name);
	}

	property = of_find_property(node,"compatible",NULL);
	if(!property){
		printk(KERN_WARNING "of_find_property get property failed\r\n");
	}else {
		printk("compatible:%s name:%s length:%d\r\n", 
		(char*)property->value, 
		property->name, 
		property->length);
	}




	node = of_find_compatible_node(NULL,NULL, "gpio-keys");
	if(!node){
		printk(KERN_WARNING "of_find_compatible_node get node failed\r\n");
	}else {
		printk("name:%s type:%s phandle:%d full_name:%s\r\n", 
		node->name, 
		node->type, 
		node->phandle,
		node->full_name);
	}

	property = of_find_property(node,"compatible",NULL);
	if(!property){
		printk(KERN_WARNING "of_find_property get property failed\r\n");
	}else {
		printk("compatible:%s name:%s length:%d\r\n", 
		(char*)property->value,
		property->name, 
		property->length);
	}

	node = of_get_next_child(node,NULL);
	if(node == NULL){
		printk(KERN_WARNING "of_get_next_child get node failed\r\n");
	}else {
		printk("name:%s type:%s phandle:%d full_name:%s\r\n", 
		node->name, 
		node->type, 
		node->phandle,
		node->full_name);
	}

	err = of_property_read_string(node,"label",&string2);
	if(err){
		printk(KERN_WARNING "of_property_read_string get property failed\r\n");
	}else {
		printk("label:%s\r\n", string2);
	}


	node = of_find_node_by_path("/soc/pinctrl@C0010000");
	if(node == NULL){
		printk(KERN_WARNING "of_find_node_by_path get node failed\r\n");
	}else {
		printk("name:%s type:%s phandle:%d full_name:%s\r\n", 
		node->name, 
		node->type, 
		node->phandle,
		node->full_name);
	}

	id = of_alias_get_id(node, "pinctrl");
	if (id < 0) {
		printk(KERN_WARNING "failed to get alias id\r\n");
	}else {
		printk("id:%d\r\n", id);
	}


	node = of_find_node_by_path("/soc/pinctrl@C0010000/gmac_pins/gmac-txd");
	if(node == NULL){
		printk(KERN_WARNING "of_find_node_by_path get node failed\r\n");
	}else {
		printk("name:%s type:%s phandle:%d full_name:%s\r\n", 
		node->name, 
		node->type, 
		node->phandle,
		node->full_name);
	}

	id = of_property_count_strings(node, "nexell,pins");
	printk("npins:%d\r\n", id);

	of_property_read_string_index(node, "nexell,pins", 0,
						    &gname);
	printk("gname:%s\r\n", gname);

	gname = NULL;
	of_property_read_string_index(node, "nexell,pins", 1,
						    &gname);
	printk("gname:%s\r\n", gname);

	node = of_find_node_by_path("/key");
	if(node == NULL){
		printk(KERN_WARNING "of_find_node_by_path get node failed\r\n");
	}else {
		printk("name:%s type:%s phandle:%d full_name:%s\r\n", 
		node->name, 
		node->type, 
		node->phandle,
		node->full_name);
	}

	irq = irq_of_parse_and_map(node, 0);
	printk("irq_of_parse_and_map irq : %d\r\n" , irq);


	node = of_find_node_by_path("/soc/i2c@c00a4000");
	if(node == NULL){
		printk(KERN_WARNING "of_find_node_by_path get node failed\r\n");
	}else {
		printk("name:%s type:%s phandle:%d full_name:%s\r\n", 
		node->name, 
		node->type, 
		node->phandle,
		node->full_name);
	}

	sda_io = of_get_gpio(node, 0);
	scl_io = of_get_gpio(node, 1);
	printk("sda_io:%d, scl_io:%d\r\n" , sda_io ,scl_io);
	dev = container_of(node , struct device , of_node);
	// pctrl = devm_pinctrl_get(dev);
	// pins_sda_dft = pinctrl_lookup_state(pctrl, "sda_dft");
	printk("node:0x%p , dev->of_node:0x%p\r\n",node , &dev->of_node);

	   /* 获取设备树节点 */
    // node = of_find_node_by_path("/gpio-leds/led@2");
	// if(node == NULL)
	// {
	// 	printk(KERN_WARNING "of_find_node_by_path get node failed\r\n");
	// }    

    // /* 获取LED所对应的GPIO */
    // id = of_get_named_gpio(node, "gpios", 0);
    // if(id < 0)
    // {
    //     printk("can not find led gpio\r\n");
    // }
    // printk("led_gpio num=%d\r\n", id);

	//    /* 申请gpio */
    // ret = gpio_request(id, "mygpio");
    // if(ret)
    // {
    //     printk("Faile to request the led gpio\r\n");
    // }

    // /* 使用IO，设置为输入或输出 */
    // ret = gpio_direction_output(id, 1);
    // if(ret)
    // {
    //      printk("Faile to gpio_direction_output the led gpio\r\n");
    // }

    // /* 输出低电平，点亮led灯 */
    // gpio_set_value(id, 1);

	return 0;
}
	

/* 模块出口 */
static void __exit dtsof_exit(void)
{
	gpio_set_value(id, 0);
	gpio_free(id);
	printk(KERN_INFO"dtsof_exit\r\n");
}


/* 注册模块入口和出口 */
module_init(dtsof_init);
module_exit(dtsof_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huangzhuyu");

