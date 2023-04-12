#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<stdlib.h>


int main(){
    int fd1,fd2,fd3;
    while(1){
    char x;
    int c;
    printf("Enter choice:\n1:Red Light.\n2:Green Light.\n3:Yellow Light\n4:Exit\n");
    scanf("%d",&c);
        switch(c){
            case 1:{
                fd3 = open("/dev/led_device-2",O_RDWR);
                if(fd3 == -1){
                    printf("Failed to open led_device-2\n");
                    return -1;
                }
                x = '1';
                if(write(fd3,&x,sizeof(char)) < 0){
                    printf("failed to write");
                    return -2;
                }
                sleep(2);
                x = '0';
                if(write(fd3,&x,sizeof(char)) < 0){
                    printf("failed to write");
                    return -2;
                }
                close(fd3);
                break;
            }
            case 2:{
                fd2 = open("/dev/led_device-1",O_RDWR);
                if(fd2 == -1){
                printf("Failed to open led_device-1\n");
                return -1;
                }
                x = '1';
                if(write(fd2,&x,sizeof(char)) < 0){
                    printf("failed to write");
                    return -2;
                }
                sleep(2);
                x = '0';
                if(write(fd2,&x,sizeof(char)) < 0){
                    printf("failed to write");
                    return -2;
                }
                close(fd2);
                break;
            }
            case 3:{
                fd1 = open("/dev/led_device-0",O_RDWR);
                if(fd1 == -1){
                    printf("Failed to open led_device-0\n");
                    return -1;
                }
                x = '1';
                if(write(fd1,&x,sizeof(char)) < 0){
                    printf("failed to write");
                    return -2;
                }
                sleep(2);
                x = '0';
                if(write(fd1,&x,sizeof(char)) < 0){
                    printf("failed to write");
                    return -2;
                }
                close(fd1);
                break;
            }
            case 4:{
                printf("Exiting.....\n");
                close(fd1);
                close(fd2);
                close(fd3);
                exit(0);
            }
            default:{
                printf("Invalid choice\n");
                break;
            }
        }
    }
    return 0;
}
