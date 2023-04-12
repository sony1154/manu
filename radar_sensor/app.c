/*application to read*/
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<string.h>
#include<errno.h>


int main(void){
	int fd;
	fd = open("/dev/radar_device",O_RDWR);

	if(fd < 0){
		perror("Failed Open");
		return -1;
	}

	int buf;
	while(1){
		if(read(fd, &buf, sizeof(int)) < 0){
			printf("Failed to Read");
			return -1;
		}
		if(buf == 0) 	printf("Sensor Output:%d Motion is detected\n", buf);	
		else 	printf("Sensor Output:%d Motion is not detected\n", buf);	
		sleep(1);
	}
	close(fd);

	return 0;
}
