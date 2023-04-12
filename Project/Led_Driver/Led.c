#include <linux/kernel.h>
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

/*GPIO pin*/
#define GPIO_OUT (21)

static dev_t led_dev;   /* device number*/
static struct cdev led_cdev;
static struct class *led_class;



static int led_open(struct inode* pinode,struct file* pfile){
  printk("Device has been opened...\n");
  if(gpio_is_valid(LED_GPIO_OUT) == false){
    printk("GPIO - %d is not valid\n",GPIO_21);
    return -1;
  }
  if(gpio_request(LED_GPIO_OUT,"LED_GPIO_OUT") < 0){
    printk("GPIO - %d failed to request\n",GPIO_21);
    return -1;
  }
  if(gpio_direction_output(LED_GPIO_OUT,0) < 0){
    printk("GPIO - %d failed to set direction\n",GPIO_21);
    gpio_free(LED_GPIO_OUT);
    return -1;
  }
  return 0;
}


static ssize_t led_read(struct file* pfile,char* __user buf,size_t len,loff_t *offset){
  uint8_t gpio_state = gpio_get_value(GPIO_21);
  printk("Led Read function has been called\n");
  len = 1;
  if(copy_to_user(buf,&gpio_state,len) > 0){
    printk("Couldn't write all the bytes to user\n");
  }

  printk("From GPIO - %d =  %d\n",LED_GPIO_OUT,gpio_state);

  
  return len;  
}



static ssize_t led_write(struct file* pfile,const char* __user buf,size_t len,loff_t *offset){
  uint8_t dev_buf[10] = {0};
  printk("Led Write function has been called\n");
  if(copy_from_user(dev_buf,buf,len) > 0){
    printk("Failed to write all the bytes\n");
  }
  printk("Wrote : %c\n",dev_buf[0]);
  
  if(dev_buf[0] == '1'){
    gpio_set_value(LED_GPIO_OUT,1);
  }else if(dev_buf[0] == '0'){
    gpio_set_value(LED_GPIO_OUT,0);
  }else{
    printk("Please enter either 1 or 0 to on/off Led\n");
    return -1;
  }
  return len;
}

static int led_release(struct inode* pinod,struct file* pfile){
  printk("Device has been closed\n");
  gpio_free(LED_GPIO_OUT);
  return 0;
}


struct file_operations fops = {
  .owner = THIS_MODULE,
  .open = led_open,
  .read = led_read,
  .write = led_write,
  .release = led_release,
};


static int __init led_init(void){
  int err = alloc_chrdev_region(&led_dev,0,1,"led_device");

  if(err < 0){
    printk("Failed to allocate region for device\n");
    unregister_chrdev_region(led_dev,1);
    return -1;
  }
  printk("Device has been registered successfully with <Major> : <Minor> = %d : %d\n",MAJOR(led_dev),MINOR(led_dev));
  
  cdev_init(&led_cdev,&fops);
  led_cdev.owner = THIS_MODULE;
  
  if(cdev_add(&led_cdev,led_dev,1) < 0){
    printk("Failed at cdev_add\n");
    unregister_chrdev_region(led_dev,1);
    return -1;
  }
  led_class = class_create(THIS_MODULE,"led_class");
  if(IS_ERR(led_class)){
    printk("Failed to create class\n");
    cdev_del(&led_cdev);
    unregister_chrdev_region(led_dev,1);
    return -1;
  }

  if(IS_ERR(device_create(led_class,NULL,led_dev,NULL,"led_device"))){
    printk("Failed to create device file in /dev directory\n");
    cdev_del(&led_cdev);
    class_destroy(led_class);
    unregister_chrdev_region(led_dev,1);
    return -1;
  }
  printk("Led Device Driver is  loaded succesfully\n");
  return 0;
}


static void __exit led_cleanup(void){
  device_destroy(led_class,led_dev);
  class_destroy(led_class);
  cdev_del(&led_cdev);
  unregister_chrdev_region(led_dev,1);
  printk("Device driver has been removed succefully\n");
}


module_init(led_init);
module_exit(led_cleanup);



MODULE_AUTHOR("Maneesha");
MODULE_DESCRIPTION("LED Out Device Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
