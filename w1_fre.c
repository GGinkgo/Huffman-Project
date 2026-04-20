#include<stdio.h>
#include<stdlib.h>

typedef struct Node{
    unsigned char id;
    long long weight;
    struct Node *left;
    struct Node *right;
    struct Node *next;
}Node;

int main(int argc,char* argv[]) {
    if(argc<2){
        printf("Error:Missing Filename!\n");
        printf("Usage:%s <filename>\n",argv[0]);
        return 1;
    }
    FILE *fp =fopen(argv[1],"rb");
    if(fp==NULL){
        fprintf(stderr,"Cannot open file[%s]:",argv[1]);
        perror("");
        return 1;
    }
    long long freq[256]={0};
    int ch;
    while((ch=fgetc(fp))!=EOF){
        freq[(unsigned char)ch]++;
    }
    fclose(fp);
    
    int unique_chars=0;
    for(int i=0;i<256;i++){
        if(freq[i]>0){
            unique_chars++;
            if(i>32&&i<127){
                printf("Char [%c](ASCII %3d):%lld times\n",i,i,freq[i]);
            }else {
                printf("Byte [0x%02X](ASCII %3d):%lld times\n",i,i,freq[i]);
            }
        }
    }
    printf("Total unique bytes found:%d\n",unique_chars);
    return 0;
}