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
Node * buildHuffmanTree(long long *freq){
    Node *head=NULL;

    for(int i=0;i<256;i++){
        if(freq[i]>0){
            Node * newNode=(Node*)malloc(sizeof(Node));
            newNode->id=(unsigned char)i;
            newNode->weight=freq[i];
            newNode->left=newNode->right=NULL;
            newNode->next=NULL;
            insertSorted(&head,newNode);
        }
    }

    while(head!=NULL&&head->next!=NULL){
        Node *min1=head;
        Node *min2=head->next;

        head=min2->next;

        Node * parent=(Node*)malloc(sizeof(Node));
        parent->id=0;
        parent->weight=min1->weight+min2->weight;
        parent->left=min1;
        parent->right=min2;
        parent->next=NULL;

        insertSorted(&head,parent);

    }
    return head;
}
// 递归打印哈夫曼树
// root: 当前节点, level: 当前深度（用于控制缩进）
void printTree(Node *root, int level) {
    if (root == NULL) return;

    // 先打印右子树（这样在控制台横过来斜着看就像一棵树）
    printTree(root->right, level + 1);

    // 打印当前节点
    for (int i = 0; i < level; i++) printf("    "); // 每层缩进4个空格
    
    if (root->left == NULL && root->right == NULL) {
        // 如果是叶子节点，打印出字符和权重
        printf("—[ID:%d/W:%lld]\n", root->id, root->weight);
    } else {
        // 如果是内部节点，只打印权重
        printf("—(W:%lld)\n", root->weight);
    }

    // 再打印左子树
    printTree(root->left, level + 1);
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

    Node *root=buildHuffmanTree(freq);

    if (root != NULL) {
        printf("\n--- Huffman Tree Structure (Right-Root-Left) ---\n");
        printTree(root, 0);
        printf("------------------------------------------------\n");
    } else {
        printf("The tree is empty!\n");
    }

    return 0;
}
