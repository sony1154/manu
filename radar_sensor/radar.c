:wq#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>  
#include <linux/gpio.h>     
#include <linux/err.h>
#include <linux/jiffies.h>
#include<linux/timer.h>


#define GPIO_21 (21)

static dev_t radar_dev;
struct cdev radar_cdev;
struct class *radar_class;
static int irq;
static uint8_t gpio_state = 0; 

/*Interrupt handler*/
static irqreturn_t irq_handler(unsigned int irq,void *dev_id) {
	gpio_state = gpio_get_value(GPIO_21);
	printk("IRQ Handler called=%d\n",gpio_state);
	return IRQ_HANDLED;
}

/*open function*/
static int radar_open(struct inode* pinode,struct file* pfile){
	printk("Device has been opened...\n");
	if(gpio_is_valid(GPIO_21) == false){
		printk("GPIO - %d is not valid\n",GPIO_21);
		return -1;
	}
	if(gpio_request(GPIO_23,"GPIO_21") < 0){
		printk("GPIO -%d failed to request\n",GPIO_21);
		return -1;
	}
	if(gpio_direction_input(GPIO_21) <0){
		printk("GPIO - %d failed to set direction\n",GPIO_21);
		gpio_free(GPIO_21);
		return -1;
	}
	irq = gpio_to_irq(GPIO_21);
	printk("RADAR GPIO: %d, IRQ:%d\n",GPIO_21,irq);
	if (request_irq(irq,(void *)irq_handler,IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,"radar_device",NULL)){                    
		printk("Microwave Radar cannot register IRQ\n");
		gpio_free(GPIO_21);
		return -1;
	}

	return 0;
}

/*read function*/
static ssize_t radar_read(struct file* pfile,char* __user buf,size_t len,loff_t *offset){
	int radar_out = 0;
	printk("Read function has been called\n");

	radar_out = gpio_get_value(GPIO_21);

	if(copy_to_user(buf, &radar_out, 1) > 0){
		printk("Couldn't copy from user\n");
	}

	if(radar_out){
		printk("Motion is detected\n");
	}else{
		printk("Motion is not detected\n");
	}

	return len;  
}

/*release function*/

static int radar_release(struct inode* pinode,struct file* pfile){
	printk("Device has been closed\n");
	free irq();
	gpio_free(GPIO_21);
	return 0;
}


struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = radar_open,
	.read = radar_read,
	.release = radar_release,
};


/*module init function*/
static int __init radar_init(void){
	int err = alloc_chrdev_region(&radar_dev, 0, 1, "Radar_Dev");
	if( err < 0){
		printk("Failed to allocate character device region for Radar\n");
		return -1;
	}
	printk("Radar: Major = %d Minor = %d \nDevice Number = %d\n",MAJOR(radar_dev), MINOR(radar_dev),radar_dev);
	
	cdev_init(&radar_cdev,&fops);
	radar_cdev.owner = THIS_MODULE;

	if(cdev_add(&radar_cdev,radar_dev,1) < 0){
		printk("Failed at cdev_add\n");
		unregister_chrdev_region(radar_dev, 1);
		return -1;
	}
	radar_class = class_create(THIS_MODULE,"radar_class");

	if(IS_ERR(radar_class)){
		printk("Failed to create class\n");
		cdev_del(&radar_cdev);
		unregister_chrdev_region(radar_dev, 1);
		return -1;
	}

	if(IS_ERR(device_create(radar_class,NULL,radar_dev,NULL,"radar_device"))){
		printk("Failed to create device file in /dev directory\n");
		class_destroy(radar_class);
		cdev_del(&radar_cdev);
		unregister_chrdev_region(radar_dev, 1);
		return -1;
	}
	printk("Radar Sensor Driver has been Loaded successfully!!!\n");
	return 0;
}

/*module cleanup function*/
static void __exit radar_cleanup(void){
	device_destroy(radar_class,radar_dev);
	class_destroy(radar_class);
	cdev_del(&radar_cdev);
	unregister_chrdev_region(radar_dev, 1);
	printk("Radar Sensor Driver has been removed succefully!!\n");
}

module_init(radar_init);
module_exit(radar_cleanup);



MODULE_AUTHOR("Maneesha");
MODULE_DESCRIPTION("GPIO Out Device Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");



