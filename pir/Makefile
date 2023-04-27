obj-m := relay_pir.o
obj-m += pir.o
  
 
host:
	make -C /lib/modules/$(shell uname -r)/build  M=$(shell pwd) modules
 


clean:
	 make -C /lib/modules/$(shell uname -r)/build  M=$(shell pwd) clean
