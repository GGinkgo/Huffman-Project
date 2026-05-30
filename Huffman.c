#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>

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
//建树
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
//编码
void generateCodes(Node *root,char *path,int depth,char **codes){
    if(root==NULL){
        return;
    }
    if(root->left==NULL&&root->right==NULL){
        path[depth]='\0';
        codes[root->id]=(char*)malloc((depth+1)*sizeof(char));
        strcpy(codes[root->id],path);
        return;
    }
    if(root->left!=NULL){
        path[depth]='0';
        generateCodes(root->left,path,depth+1,codes);
    }
    if(root->right!=NULL){
        path[depth]='1';
        generateCodes(root->right,path,depth+1,codes);
    }
}

void decodeString(Node *root,const char *s){
    if(root==NULL)  return;

    Node *current=root;
    for(int i=0;s[i]!='\0';i++){
        if(s[i]=='0'){
            current=current->left;
        }
        else if(s[i]=='1'){
            current=current->right;
        }

        if(current->left==NULL&&current->right==NULL){
        printf("%c",current->id);
        current=root;
    }
    }
    
}

int compressFile(const char *srcFilename,const char *destFilename){
    long long freq[256]={0};
     if (cntByteFreq(srcFilename, freq) != 0) {
        return 1;
    }

    Node *root = buildHuffmanTree(freq);
    if (root == NULL) {
        printf("The original file is empty, no compression needed.\n");
        return 0;
    }

    char *huffmanCodes[256] = {NULL};
    char path[256];
    generateCodes(root, path, 0, huffmanCodes);

    FILE *src = fopen(srcFilename, "rb");
    if (src == NULL) {
        perror("Failed to open the original file");
        return 1;
    }
    FILE *dest = fopen(destFilename, "wb");
    if (dest == NULL) {
        perror("Failed to create the compressed file");
        fclose(src);
        return 1;
    }

    fwrite(freq, sizeof(long long), 256, dest);

    unsigned char bits_buffer = 0; 
    int bit_count = 0;          
    int ch;

    while ((ch = fgetc(src)) != EOF) {
        char *code = huffmanCodes[ch];
        
        for (int i = 0; code[i] != '\0'; i++) {
            bits_buffer <<= 1;
            if (code[i] == '1') {
                bits_buffer |= 1;
            }
            bit_count++;

            if (bit_count == 8) {
                fputc(bits_buffer, dest);
                bits_buffer = 0;
                bit_count = 0;
            }
        }
    }

    if (bit_count > 0) {
        bits_buffer <<= (8 - bit_count);
        fputc(bits_buffer, dest);
    }

    fclose(src);
    fclose(dest);
    for (int i = 0; i < 256; i++) {
        if (huffmanCodes[i] != NULL) free(huffmanCodes[i]);
    }
    return 0;
}

int decompressFile(const char *srcFilename, const char *destFilename) {
    FILE *src = fopen(srcFilename, "rb");
    if (src == NULL) {
        perror("Error opening compressed file");
        return 1;
    }
    FILE *dest = fopen(destFilename, "wb");
    if (dest == NULL) {
        perror("Error creating decompressed file");
        fclose(src);
        return 1;
    }

    long long freq[256] = {0};
    fread(freq, sizeof(long long), 256, src);

    long long total_bytes = 0;
    for (int i = 0; i < 256; i++) {
        total_bytes += freq[i];
    }

    Node *root = buildHuffmanTree(freq);
    if (root == NULL) {
        fclose(src); fclose(dest);
        return 0;
    }

    Node *current = root;
    long long decoded_bytes = 0;
    int ch;

    while (decoded_bytes < total_bytes && (ch = fgetc(src)) != EOF) {
        for (int i = 7; i >= 0; i--) {
            int bit = (ch >> i) & 1;

            if (bit == 0) {
                current = current->left;
            } else {
                current = current->right;
            }

            if (current->left == NULL && current->right == NULL) {
                fputc(current->id, dest);
                decoded_bytes++;
                current = root;

                if (decoded_bytes == total_bytes) {
                    break;
                }
            }
        }
    }

    fclose(src);
    fclose(dest);
    return 0;
}
void printUsage(const char *progName) {
    printf("Huffman Compression/Decompression Tool\n");
    printf("Usage:\n");
    printf("  Compress: %s -c -o <output.huff> <file1> [file2] [file3]...\n", progName);
    printf("  Extract:  %s -x <input.huff> -d <out_dir>\n", progName);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }

    char *mode = argv[1];

    if (strcmp(mode, "-c") == 0) {
        char *outputFile = NULL;
        int fileStartIndex = 2;

        if (strcmp(argv[2], "-o") == 0) {
            if (argc < 5) {
                printf("Error: Missing output filename!\n");
                return 1;
            }
            outputFile = argv[3];
            fileStartIndex = 4;
        } else {
            outputFile = "output.huff";
        }

        int inputFilesCount = argc - fileStartIndex;
        if (inputFilesCount == 0) {
            printf("Error: No input files specified!\n");
            return 1;
        }

        printf("Starting compression for %d file(s)...\n", inputFilesCount);
        clock_t start = clock();

        if (inputFilesCount == 1) {
            if (compressFile(argv[fileStartIndex], outputFile) == 0) {
                clock_t end = clock();
                double duration = (double)(end - start) / CLOCKS_PER_SEC;
                printf("Compression completed successfully! Time taken: %.3f seconds\n", duration);
            }
        } else {
            printf("Multi-file compression logic pending implementation...\n");
        }

    } else if (strcmp(mode, "-x") == 0) {
        char *inputFile = argv[2];
        char *outputDir = ".";

        if (argc >= 5 && strcmp(argv[3], "-d") == 0) {
            outputDir = argv[4];
        }

        printf("Starting decompression for %s ...\n", inputFile);
        clock_t start = clock();

        char destPath[512];
        sprintf(destPath, "%s/extracted_file", outputDir); 
        
        if (decompressFile(inputFile, destPath) == 0) {
            clock_t end = clock();
            double duration = (double)(end - start) / CLOCKS_PER_SEC;
            printf("Decompression completed successfully! Time taken: %.3f seconds\n", duration);
        }

    } else {
        printf("Error: Unknown mode '%s'\n", mode);
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
