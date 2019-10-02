#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

int main(void){
	char a[32][100];
	int id = 0;
	while(1){
		char c[100];
		scanf("%s",c);
		int flag = 0;
		for(int i=0;i<id;i++){
			if(strcmp(c,a[i])==0){
				flag = 1;
				printf("%d\n",i);
			}
		}
		if(flag==0){
			printf("%d\n",id);
			strcpy(a[id++],c);
		}
	}
	return 0;
}