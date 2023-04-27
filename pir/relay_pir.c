#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/uaccess.h>  
#include <linux/gpio.h>     
#include <linux/err.h>


#define RELAY_GPIO_OUT (24)




static int __init relay_init(void){
	if(gpio_is_valid(RELAY_GPIO_OUT) == false){
		printk("GPIO- %d is not valid\n",RELAY_GPIO_OUT);
		return -1;
	}
	if(gpio_request(RELAY_GPIO_OUT,"RELAY_GPIO_OUT") <0){
		printk("GPIO - %d failed to request\n",RELAY_GPIO_OUT);
		return -1;
	}	       
	if(gpio_direction_output(RELAY_GPIO_OUT,0) < 0){
		printk("failed to set direction\n");
		gpio_free(RELAY_GPIO_OUT);
		return -1;
	}
	printk("Relay INIT Module has been loaded\n");
	return 0;
}

static void __exit relay_cleanup(void){
	gpio_free(RELAY_GPIO_OUT);
	printk("Relay INIT has been removed successfully\n");
}


module_init(relay_init);
module_exit(relay_cleanup);



MODULE_AUTHOR("Kaveri");
MODULE_DESCRIPTION("Simple Relay intialising  Device Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
