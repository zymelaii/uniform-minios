#include "stdio.h"

int global=0;

char *str2,*str3;


void pthread_test1()
{
	int i;
	//pthread(pthread_test2);
	while(1)
	{
		printf("pth1");
		printf("%d",++global);
		printf(" ");
		i=10000000;
		while(--i){}
	}
}

/*======================================================================*
                          Syscall Pthread Test
added by xw, 18/4/27
 *======================================================================*/

int main(int arg,char *argv[])
{
	int i=0;
	
	pthread(pthread_test1);
	while(1)
	{
		printf("init");
		printf("%d",++global);
		printf(" ");
		i=10000000;
		while(--i){}
	}
	return 0;
}
