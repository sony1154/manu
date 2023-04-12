/*I2C EEPROM Slave Device Driver*/
#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs.h>
#include<linux/i2c.h>
#include<linux/kdev_t.h>
#include<linux/cdev.h>
#include<linux/kernel.h>
#include<linux/string.h>
#include<linux/delay.h>


/*available i2c bus number*/
#define I2C_BUS (1)
/*Page size to write to eeprom*/
#define SIZE 8
/*eeprom ic address*/
#define SLAVE_ADDRESS (0x50)
/*our slave device name*/
#define SLAVE_DEVICE_NAME "eeprom_slave"
/*Kernel(EEPROM) Buffer Size*/
#define MEM_SIZE 257  
/*Kernel(EEPROM) Buffer*/
char* eeprom_buffer;

/*device number*/
static dev_t eeprom_dev;
/*cdev structure*/
static struct cdev eeprom_cdev;
/*class structure*/
static struct class* eeprom_class;
/*I2C adapter structure*/
static struct i2c_adapter* eeprom_adapter;
/*I2C client structure*/
static struct i2c_client* eeprom_client;

/*write location*/
char wl = '0';


/*eeprom open function*/
static int eeprom_open(struct inode* inode,struct file* file){
  eeprom_buffer = kzalloc(MEM_SIZE,GFP_KERNEL);
  if(eeprom_buffer == NULL){
    pr_err("Failed to allocate memory");
    return -1;
  }
  printk("I2C EEPROM Slave Device Opened...\n");
  return 0;
}

/*eeprom close function*/
static int eeprom_release(struct inode* inode,struct file* file){
  kfree(eeprom_buffer);
  printk("I2C EEPROM Slave Device Closed\n");
  return 0;
}

/*eeprom read function*/
static ssize_t eeprom_read(struct file* file,char* __user buf,size_t len,loff_t* off){
  int ret = 0;
  int pos = 0;
  int read_len = 0;
  char page_buffer[SIZE] ={0};
  int rem_bytes = len;

  memset(eeprom_buffer,0x00,MEM_SIZE);
  //location write here
  page_buffer[0] = wl;
  ret = i2c_master_send(eeprom_client,page_buffer,1);
  if(ret < 0){
    printk("Failed to write in Read\n");
    return -1;
  }
  printk("Byte Write from read: %d\n",ret);
  mdelay(5);
  printk("len: %d\n",len);
  if(wl%SIZE != 0){
    memset(page_buffer,0x00,SIZE+1);
    read_len = ((wl/SIZE) +1)* SIZE - wl;
    ret = i2c_master_recv(eeprom_client,page_buffer,read_len);
    mdelay(5);
    if(ret < 0){
      pr_err("failed to write first chunk\n");
      return -1;
    }
    printk("ret = %d\n",ret);
    memcpy(eeprom_buffer+pos,page_buffer,read_len);
    printk("page_buffer: %s\n",page_buffer);
    
    pos = pos + read_len;
    rem_bytes = rem_bytes - read_len;
    wl = wl + read_len;
  }
  while(rem_bytes >= SIZE){
    memset(page_buffer,0x00,SIZE+1);
    read_len = SIZE;
    ret = i2c_master_recv(eeprom_client,page_buffer,read_len);
    mdelay(5);
    if(ret < 0){
      pr_err("failed to write in while loop\n");
      return -1;
    }
    printk("ret = %d\n",ret);

    memcpy(eeprom_buffer+pos,page_buffer,read_len);
    printk("page_buffer: %s\n",page_buffer);
    pos = pos + read_len;
    rem_bytes = rem_bytes - read_len;
    wl = wl + read_len;
  }

  if(rem_bytes > 0 && rem_bytes < SIZE){
    memset(page_buffer,0x00,SIZE+1);
    read_len = rem_bytes;
    ret = i2c_master_recv(eeprom_client,page_buffer,read_len);
    mdelay(5);
    if(ret < 0){
      pr_err("failed to write in while loop\n");
      return -1;
    }
    printk("ret = %d\n",ret);

    memcpy(eeprom_buffer+pos,page_buffer,read_len);
    printk("page_buffer: %s\n",page_buffer);
    pos = pos + read_len;
    rem_bytes = rem_bytes - read_len;
    wl = wl + read_len;
  }

  printk("Reading: %s\n",eeprom_buffer);
  if(copy_to_user(buf,eeprom_buffer,len) > 0){
    pr_err("Couldn't read all the bytes to user\n");
    return -1;
  }  
  return len;
}

/*eeprom write function*/
static ssize_t eeprom_write(struct file* file,const char* __user buf,size_t len,loff_t* off){
  int ret=0;
  int pos=0;
  int rem_bytes = len;
  int wr_len = 0;

  char page_buffer[SIZE+1] = {0};

  memset(eeprom_buffer,0x00,MEM_SIZE);
  
  printk("Writing to EEPROM Slave..\n");
  
  if(copy_from_user(eeprom_buffer,buf,len) > 0){
    pr_err("failed to write\n");
    return -1;
  }

  printk("Writing: %s\n",eeprom_buffer);
  
  if(wl%SIZE != 0){
    memset(page_buffer,0x00,SIZE+1);
    wr_len = ((wl/SIZE) +1)* SIZE - wl;
    page_buffer[0] = wl;
    memcpy(page_buffer + 1,eeprom_buffer+pos,wr_len);
    printk("page_buffer: %s\n",page_buffer);
    ret = i2c_master_send(eeprom_client,page_buffer,wr_len+1);
    mdelay(5);
    if(ret < 0){
      pr_err("failed to write first chunk\n");
      return -1;
    }
    printk("ret = %d\n",ret);
    pos = pos + wr_len;
    rem_bytes = rem_bytes - wr_len;
    wl = wl + wr_len;
  }
  while(rem_bytes >= SIZE){
    memset(page_buffer,0x00,SIZE+1);
    wr_len = SIZE;
    page_buffer[0] = wl;
    memcpy(page_buffer+1,eeprom_buffer+pos,wr_len);
    printk("page_buffer: %s\n",page_buffer);
    ret = i2c_master_send(eeprom_client,page_buffer,wr_len+1);
    mdelay(5);
    if(ret < 0){
      pr_err("failed to write in while loop\n");
      return -1;
    }
    printk("ret = %d\n",ret);
    pos = pos + wr_len;
    rem_bytes = rem_bytes - wr_len;
    wl = wl + wr_len;
  }

  if(rem_bytes > 0 && rem_bytes < SIZE){
    memset(page_buffer,0x00,SIZE+1);
    wr_len = rem_bytes;
    page_buffer[0] = wl;
    memcpy(page_buffer+1,eeprom_buffer+pos,wr_len);
    printk("page_buffer: %s\n",page_buffer);
    ret = i2c_master_send(eeprom_client,page_buffer,wr_len+1);
    mdelay(5);
    if(ret < 0){
      pr_err("failed to write in last chunk\n");
      return -1;
    }
    printk("ret = %d\n",ret);
    pos = pos + wr_len;
    rem_bytes = rem_bytes - wr_len;
    wl = wl + wr_len;
  }
  return len;
}


loff_t eeprom_llseek(struct file *file, loff_t off, int whence){
  wl = (char)off;
  printk("Given Starting location = %d\n",wl);
  return off;
}

/*eeprom file operations structure*/
static struct file_operations fops = {
  .owner = THIS_MODULE,
  .open = eeprom_open,
  .read = eeprom_read,
  .write = eeprom_write,
  .llseek = eeprom_llseek,
  .release = eeprom_release,
};


/*eeprom driver probe function*/
static int eeprom_probe(struct i2c_client* eeprom_client,const struct i2c_device_id* eeprom_ids){
  int ret = alloc_chrdev_region(&eeprom_dev,0,1,SLAVE_DEVICE_NAME);

  if(ret < 0){
    pr_err("Failed to register i2c eeprom slave device\n");
    return -1;
  }

  cdev_init(&eeprom_cdev,&fops);

  if(cdev_add(&eeprom_cdev,eeprom_dev,1) < 0){
    pr_err("Failed i2c eeprom slave cdev_add\n");
    unregister_chrdev_region(eeprom_dev,1);
    return -1;
  }

  eeprom_class = class_create(THIS_MODULE,"eeprom_slave_class");
  if(eeprom_class == NULL){
    pr_err("Failed to create i2c eeprom slave class\n");
    cdev_del(&eeprom_cdev);
    unregister_chrdev_region(eeprom_dev,1);
    return -1;
  }


  if(device_create(eeprom_class,NULL,eeprom_dev,NULL,SLAVE_DEVICE_NAME) == NULL){
    pr_err("Failed to create i2c eeprom slave device file\n");
    class_destroy(eeprom_class);
    cdev_del(&eeprom_cdev);
    unregister_chrdev_region(eeprom_dev,1);
    return -1;
  }

  printk("I2C EEPROM Salve Driver Probed\n");
  return 0;
}

/*eeprom driver remove function*/
static int eeprom_remove(struct i2c_client* eeprom_client){
  device_destroy(eeprom_class,eeprom_dev);
  class_destroy(eeprom_class);
  cdev_del(&eeprom_cdev);
  unregister_chrdev_region(eeprom_dev,1);
  printk("I2C EEPROM Salve Driver Removed\n");
  return 0;
}

/*i2c slave(eeprom) device id structure*/
static const struct i2c_device_id eeprom_ids[] = {
  {SLAVE_DEVICE_NAME, 0},
  {} 
};

MODULE_DEVICE_TABLE(i2c,eeprom_ids);


/*i2c driver structure*/
static struct i2c_driver eeprom_driver  = {
  .driver = {
    .name = SLAVE_DEVICE_NAME,
    .owner = THIS_MODULE,
  },
  .probe = eeprom_probe,
  .remove = eeprom_remove,
  .id_table = eeprom_ids,
};

/*EEPROM IC information structure*/
static const struct i2c_board_info eeprom_ic_info = {
  I2C_BOARD_INFO(SLAVE_DEVICE_NAME,SLAVE_ADDRESS)  
};


/*module init function*/
static int __init eeprom_init(void){
  eeprom_adapter = i2c_get_adapter(I2C_BUS);

  if(eeprom_adapter == NULL){
    pr_err("I2C adapter failed to allocate\n");
    return -1;
  }

  eeprom_client = i2c_new_client_device(eeprom_adapter,&eeprom_ic_info);

  if(eeprom_client == NULL){
    pr_err("Failed to create new client device for i2c eeprom slave\n");
    return -1;
  } 

  i2c_add_driver(&eeprom_driver);

  printk("I2C EEPROM Salve Driver loaded successsfully\n");
  return 0;
}

/*module exit function*/
static void __exit eeprom_cleanup(void){
  i2c_unregister_device(eeprom_client);
  i2c_del_driver(&eeprom_driver);
  printk("I2C EEPROM Salve Driver unloaded successfully\n");
}



module_init(eeprom_init);
module_exit(eeprom_cleanup);


/*Meta information*/
MODULE_AUTHOR("Ram");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A simple I2C EEPROM Slave Device Driver");
MODULE_VERSION("1.0");