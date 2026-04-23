#include<stdio.h>
#include<stdlib.h>
//创建节点
typedef struct Node{
    unsigned char id;
    long long weight;
    struct Node *left;
    struct Node *right;
    struct Node *next;
}Node;
//统计每个字节的频率
int cntByteFreq(const char *filename,long long *freq){
    FILE *fp =fopen(filename,"rb");
    if(fp==NULL){
        fprintf(stderr,"Cannot open file[%s]:",filename);
        perror("");
        return 1;
    }
    
    int ch;
    while((ch=fgetc(fp))!=EOF){
        freq[(unsigned char)ch]++;
    }
    fclose(fp);
    return 0;
}
//建树辅助函数：插入排序
void insertSorted(Node **head,Node *newNode){
    if((*head)==NULL||newNode->weight<(*head)->weight){
        newNode->next=*head;
        *head=newNode;
        return;
    }
    Node *current=*head;
    while(current->next!=NULL&&current->next->weight<newNode->weight){
        current=current->next;
    }
    newNode->next=current->next;
    current->next=newNode;

}
int main(int argc,char *argv[]){
    if(argc<2){
        printf("Error:Missing Filename!\n");
        printf("Usage:%s <filename>\n",argv[0]);
        return 1;
    }

    long long freq[256]={0};
    if(cntByteFreq(argv[1],freq)!=0){
        return 1;
    }


    return 0;
}
