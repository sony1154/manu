#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include<linux/version.h>
#include<linux/uaccess.h>
#include<linux/gpio.h>

#define MAX 3

/*GPIO pins*/

#define GPIO_21 (21)
#define GPIO_20 (20)
#define GPIO_16 (16)


static struct gpio leds_gpio[] = {
    {GPIO_21,GPIOF_OUT_INIT_LOW,"LED-1"},
    {GPIO_20,GPIOF_OUT_INIT_LOW,"LED-2"},
    {GPIO_16,GPIOF_OUT_INIT_LOW,"LED-3"}
};

static dev_t major_number;
static struct class* led_class;
static struct cdev led_cdev[MAX];




static int led_open(struct inode *pinode,struct file *pfile) {
    printk("Device file opened...\n");
    int minor = iminor(file_inode(pfile));
    if( minor >=MAX ){
	    printk("Minor number exceeded max number of device\n");
	    return -1;
    }
      if(gpio_is_valid(leds_gpio[minor].gpio) == false){
            printk("GPIO - %d is not valid\n",leds_gpio[minor].gpio);
   
        }
        if(gpio_request(leds_gpio[minor].gpio,"GPIO_21") < 0){
            printk("GPIO - %d failed to request\n",leds_gpio[minor].gpio);
        }
        if(gpio_direction_output(leds_gpio[minor].gpio,0) < 0){
            printk("GPIO -%d failed to set direction\n",leds_gpio[minor].gpio);
            return -1;
        }
    return 0;
}


static int led_close(struct inode *pinode,struct file *pfile){
	int minor = iminor(file_inode(pfile));
    if(minor >= MAX){
        printk("Device file closed!\n");
    }
    gpio_free(leds_gpio[minor].gpio);
         return 0;
}

static ssize_t led_read(struct file* pfile,char* __user buf,size_t len,loff_t *loff){
    printk("Device file opened to read %ld\n",len);
    return len;
}

static ssize_t led_write(struct file* pfile,const char* __user buf,size_t len,loff_t *loff){
    uint8_t dev_buf[10] = {0};
    printk("Changing led state\n");
    int minor = iminor(file_inode(pfile));
    if(copy_from_user(dev_buf,buf,len) > 0){
        printk("Couldn't write all the given bytes\n");
        return -1;
    }
    if(minor >= MAX){
	   printk("Minor number has exceed MAX number of devices\n");
	    return -1;
    }
        if(dev_buf[0] == '0'){
            gpio_set_value(leds_gpio[minor].gpio,0);
        }else if(dev_buf[0] == '1'){
            gpio_set_value(leds_gpio[minor].gpio,1);
        }else{
            printk("Please write either 1 or 0 to on/off Led\n");
            return -1;
        }
   }
    return len;
}

struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_close,
};

static int __init led_init(void){
    int err = alloc_chrdev_region(&major_number,0,MAX,"led_devices");
    dev_t dev_num;
    int major,i;
    if(err < 0){
        printk("failed to allocate major number\n");
        return -1;
    }
    major = MAJOR(major_number);
    printk("Device register with <MAJOR> = <%d>\n",MAJOR(major_number));
    led_class = class_create(THIS_MODULE,"led_class");
    
    if(IS_ERR(led_class)){
        printk("failed to create class\n");
        unregister_chrdev_region(major_number,1);
        return -2;
    }
    for(i=0;i<MAX;i++){
        dev_num = MKDEV(major,i);
        printk(" minor number = %d\n",MINOR(dev_num));
        cdev_init(&led_cdev[i],&fops);
        if(cdev_add(&led_cdev[i],dev_num,1) < 0){
            printk("Failed at cdev_add\n");
	    class_destroy(led_class);
	    unregister_chrdev_region(major_number,1);
            return -3;
        }
        if(IS_ERR(device_create(led_class,NULL,dev_num,NULL,"led_device-%d",i))){
            printk("Failed to create device file in /dev directory\n");
	    cdev_del(&led_cdev[i]);
            class_destroy(led_class);
	    unregister_chrdev_region(major_number,1);
            return -4;
        }
    }
    printk("Multiple Leds Device Drivee loaded succesfully\n");
    return 0;
}



static void __exit led_cleanup(void){
    dev_t dev_num;
    int i,major;
    major = MAJOR(major_number);
    for(i=0;i<MAX;i++){
        cdev_del(&led_cdev[i]);
        dev_num = MKDEV(major ,i);
        device_destroy(led_class,dev_num);
    }
    class_destroy(led_class);
    unregister_chrdev_region(major_number,MAX);
    printk("Device driver has been removed succefully\n");
}


module_init(led_init);
module_exit(led_cleanup);

MODULE_AUTHOR("Maneesha");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Multiple Leds Device Driver");
MODULE_VERSION("1.0");
