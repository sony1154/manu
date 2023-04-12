#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/uaccess.h>  
#include <linux/gpio.h>     
#include <linux/err.h>


#define LES_OUT (21)




static int __init led_init(void){
	if(gpio_is_valid(LED_OUT) == false){
		printk("GPIO- %d is not valid\n",GPIO_21);
		gpio_free(GPIO_21);
		return -1;
	}
	if(gpio_request(GPIO_21,"LED_OUT") <0){
		printk("GPIO - %d failed to request\n",GPIO_21);
		gpio_free(GPIO_21);
		return -1;
	}	       
	if(gpio_direction_output(LED_OUT,0) < 0){
		printk("failed to set direction\n");
		gpio_free(GPIO_21);
		return -1;
	}
	printk("Led INIT Module has been loaded\n");
	return 0;
}

static void __exit led_cleanup(void){
	gpio_free(LED_OUT);
	printk("Led INIT has been removed succefully\n");
}


module_init(led_init);
module_exit(led_cleanup);



MODULE_AUTHOR("Maneesha");
MODULE_DESCRIPTION("Simple Led intialising  Device Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
