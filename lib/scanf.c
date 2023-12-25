#include "stdio.h"

char getchar()
{
	char ch;
	return read(STD_IN,&ch,1)==1 ? ch : EOF;
}

char* gets(char *str)
{
	char c;
	char *cs;
	cs= str;
	while( (c=getchar()) != EOF ){
		if( (*cs=c)=='\n' ){
			*cs = '\0';
			break;
		}
		cs++;
	}
	return str;
}

//  int scanf(char *str, ...)
// {
// 	va_list vl;
// 	int i = 0, j = 0, ret = 0;
// 	char buff[100] = {0};
// 	char *out_loc;
// 	gets(buff);
// 	va_start(vl, str);
// 	i = 0;
// 	while (str && str[i])
// 	{
// 		if (str[i] == '%')
// 		{
// 			i++;
// 			switch (str[i])
// 			{
// 				case 'c':
// 				{
// 					*(char *)va_arg(vl, char *) = buff[j];
// 					j++;
// 					ret++;
// 					break;
// 				}
// 				case 'd':
// 				{
// 					*(int *)va_arg(vl, int *) = strtol(&buff[j], &out_loc, 10);
// 					j += out_loc - &buff[j];
// 					ret++;
// 					break;
// 				}
// 				case 'x':
// 				{
// 					*(int *)va_arg(vl, int *) = strtol(&buff[j], &out_loc, 16);
// 					j += out_loc - &buff[j];
// 					ret++;
// 					break;
// 				}
// 			}
// 		}
// 		else
// 		{
// 			buff[j] = str[i];
// 			j++;
// 		}
// 		i++;
// 	}
// 	va_end(vl);
// 	return ret;
// }