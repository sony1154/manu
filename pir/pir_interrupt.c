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
#define RELAY_GPIO_OUT (24)
#define PIR_GPIO_IN  (23)


/*Device number*/
static dev_t pir_dev;
/*pir class structure*/
static struct class *pir_class;
/*pir cdev structure*/
static struct cdev pir_cdev;

/*storing irq number*/
static unsigned int pir_irq_number;


/*switch for interrupt handler*/
static uint8_t pir_state = 0;
	
/*Interrupt handler*/
static irqreturn_t gpio_irq_handler(int irq,void *dev_id) {
	pir_state = gpio_get_value(PIR_GPIO_IN);
        gpio_set_value(RELAY_GPIO_OUT, pir_state); 
	return IRQ_HANDLED;
}


/*open function*/
static int pir_open(struct inode *inode, struct file *file){
	printk("Pir device file Opened...!!!\n");
	if(gpio_is_valid(PIR_GPIO_IN) == false){
		printk("GPIO - %d is not valid\n",PIR_GPIO_IN);
		return -1;
	}
	if(gpio_request(PIR_GPIO_IN,"PIR_GPIO_IN") < 0){
		printk("GPIO -%d failed to request\n",PIR_GPIO_IN);
		return -1;
	}
	if(gpio_direction_input(PIR_GPIO_IN) <0){
		printk("GPIO - %d failed to set direction\n",PIR_GPIO_IN);
		gpio_free(PIR_GPIO_IN);
		return -1;
	}
	radar_irq_number = gpio_to_irq(PIR_GPIO_IN);
	printk("GPIO IRQ Number = %d\n", pir_irq_number);
	if (request_irq(pir_irq_number,(void *)gpio_irq_handler,IRQF_TRIGGER_HIGH | IRQF_TRIGGER_LOW,"pir_device",NULL)){                    
		printk("Pir cannot register IRQ\n");
		gpio_free(PIR_GPIO_IN);
		return -1;
	}
	return 0;
}

/*release function*/
static int pir_release(struct inode *inode, struct file *file){
	free_irq(pir_irq_number,NULL);
	gpio_free(PIR_GPIO_IN);
	printk("Pir device file is Closed...!!!\n");
	return 0;
}

/*read function*/
static ssize_t pir_read(struct file *filp,char __user *buf, size_t len, loff_t *off){
	if(len < 1){
		printk("Buffer size is invalid\n");
		return -1;
	}
	pir_state = gpio_get_value(PIR_GPIO_IN);

	printk("Pir State = %d \n", pir_state);

	if( copy_to_user(buf, &pir_state, 1) > 0) {
		pr_err("ERROR: Counldn't write pir state to user\n");
		return -1;
	}
	return len;
}



/*file operations structure*/
static struct file_operations fops ={
	.owner          = THIS_MODULE,
	.read           = pir_read,
	.open           = pir_open,
	.release        = pir_release,
};

/*module init function*/
static int __init pir_driver_init(void){
	int err = alloc_chrdev_region(&pir_dev, 0, 1, "pir_Dev");
	if( err < 0){
		printk("Failed to allocate character device region for pir\n");
		return -1;
	}
	printk("Pir: Major = %d Minor = %d \nDevice Number = %d\n",MAJOR(pir_dev), MINOR(pir_dev),pir_dev);
	
	cdev_init(&pir_cdev,&fops);
	pir_cdev.owner = THIS_MODULE;

	if(cdev_add(&pir_cdev,pir_dev,1) < 0){
		printk("Failed at cdev_add\n");
		unregister_chrdev_region(pir_dev, 1);
		return -1;
	}
	pir_class = class_create(THIS_MODULE,"pir_class");

	if(IS_ERR(pir_class)){
		printk("Failed to create class\n");
		cdev_del(&pir_cdev);
		unregister_chrdev_region(pir_dev, 1);
		return -1;
	}

	if(IS_ERR(device_create(pir_class,NULL,pir_dev,NULL,"pir_device"))){
		printk("Failed to create device file in /dev directory\n");
		class_destroy(pir_class);
		cdev_del(&pir_cdev);
		unregister_chrdev_region(pir_dev, 1);
		return -1;
	}
	printk("Pir Driver has been Loaded successfully !!!\n");
	return 0;
}

/*module cleanup function*/
static void __exit pir_driver_exit(void){
	device_destroy(pir_class,pir_dev);
	class_destroy(pir_class);
	cdev_del(&pir_cdev);
	unregister_chrdev_region(pir_dev, 1);
	printk("Relay Driver has been removed successfully !!\n");
}

module_init(pir_driver_init);
module_exit(pir_driver_exit);

/*Meta Information*/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kaveri");
MODULE_DESCRIPTION("A Simple Pir Device Driver");
MODULE_VERSION("1.0");
