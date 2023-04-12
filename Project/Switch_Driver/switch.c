#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include<linux/wait.h>
#include <linux/uaccess.h>  
#include <linux/gpio.h>     
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/jiffies.h>
#include<linux/timer.h>

/*Gpio pins*/
#define LED_GPIO_OUT (21)
#define SWITCH_GPIO_IN  (20)


/*Device number*/
static dev_t switch_dev;
/*switch class structure*/
static struct class *switch_class;
/*switch cdev structure*/
static struct cdev switch_cdev;

/*storing irq number*/
static unsigned int switch_irq_number;

/*timer variable*/
static struct timer_list switch_timer;

/*switch for interrupt handler*/
static uint8_t switch_state = 0;
	
/*timer callback*/
void switch_callback(struct timer_list *data){
	gpio_set_value(LED_GPIO_OUT, switch_state); //based on switch value led state is changed                      
	printk("Interrupt Occurred : Toggling LED to %d\n",switch_state);
}

/*Interrupt handler*/
static irqreturn_t gpio_irq_handler(int irq,void *dev_id) {
	switch_state = gpio_get_value(SWITCH_GPIO_IN);
	mod_timer(&switch_timer,jiffies + msecs_to_jiffies(200));
	return IRQ_HANDLED;
}


/*open function*/
static int switch_open(struct inode *inode, struct file *file){
	printk("Switch device file Opened...!!!\n");
	if(gpio_is_valid(SWITCH_GPIO_IN) == false){
		printk("GPIO - %d is not valid\n",SWITCH_GPIO_IN);
		return -1;
	}
	if(gpio_request(SWITCH_GPIO_IN,"SWITCH_GPIO_IN") < 0){
		printk("GPIO -%d failed to request\n",SWITCH_GPIO_IN);
		return -1;
	}
	if(gpio_direction_input(SWITCH_GPIO_IN) <0){
		printk("GPIO - %d failed to set direction\n",SWITCH_GPIO_IN);
		gpio_free(SWITCH_GPIO_IN);
		return -1;
	}
	switch_irq_number = gpio_to_irq(SWITCH_GPIO_IN);
	printk("GPIO IRQ Number = %d\n", switch_irq_number);
	if (request_irq(switch_irq_number,(void *)gpio_irq_handler,IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,"switch_device",NULL)){                    
		printk("Switch cannot register IRQ\n");
		gpio_free(SWITCH_GPIO_IN);
		return -1;
	}
	timer_setup(&switch_timer,switch_callback,0);
	return 0;
}

/*release function*/
static int switch_release(struct inode *inode, struct file *file){
	del_timer(&switch_timer);
	free_irq(switch_irq_number,NULL);
	gpio_free(SWITCH_GPIO_IN);
	printk("Switch device file is Closed...!!!\n");
	return 0;
}

/*read function*/
static ssize_t switch_read(struct file *filp,char __user *buf, size_t len, loff_t *off){
	if(len < 1){
		printk("Buffer size is invalid\n");
		return -1;
	}
	switch_state = gpio_get_value(SWITCH_GPIO_IN);

	printk("Switch State = %d \n", switch_state);

	if( copy_to_user(buf, &switch_state, 1) > 0) {
		pr_err("ERROR: Counldn't write switch state to user\n");
		return -1;
	}
	return len;
}


/*file operations structure*/
static struct file_operations fops ={
	.owner          = THIS_MODULE,
	.read           = switch_read,
	.open           = switch_open,
	.release        = switch_release,
};

/*module init function*/
static int __init switch_driver_init(void){
	int err = alloc_chrdev_region(&switch_dev, 0, 1, "switch_Dev");
	if( err < 0){
		printk("Failed to allocate character device region for switch\n");
		return -1;
	}
	printk("Switch: Major = %d Minor = %d \nDevice Number = %d\n",MAJOR(switch_dev), MINOR(switch_dev),switch_dev);
	
	cdev_init(&switch_cdev,&fops);
	switch_cdev.owner = THIS_MODULE;

	if(cdev_add(&switch_cdev,switch_dev,1) < 0){
		printk("Failed at cdev_add\n");
		unregister_chrdev_region(switch_dev, 1);
		return -1;
	}
	switch_class = class_create(THIS_MODULE,"switch_class");

	if(IS_ERR(switch_class)){
		printk("Failed to create class\n");
		cdev_del(&switch_cdev);
		unregister_chrdev_region(switch_dev, 1);
		return -1;
	}

	if(IS_ERR(device_create(switch_class,NULL,switch_dev,NULL,"switch_device"))){
		printk("Failed to create device file in /dev directory\n");
		class_destroy(switch_class);
		cdev_del(&switch_cdev);
		unregister_chrdev_region(switch_dev, 1);
		return -1;
	}
	printk("Switch Driver has been Loaded successfully!!!\n");
	return 0;
}

/*module cleanup function*/
static void __exit switch_driver_exit(void){
	device_destroy(switch_class,switch_dev);
	class_destroy(switch_class);
	cdev_del(&switch_cdev);
	unregister_chrdev_region(switch_dev, 1);
	printk("Switch Driver has been removed succefully!!\n");
}

module_init(switch_driver_init);
module_exit(switch_driver_exit);

/*Meta Information*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ram");
MODULE_DESCRIPTION("A Switch (interrupt) Device Driver");
MODULE_VERSION("1.0");
