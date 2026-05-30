#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>

// 创建节点
typedef struct Node{
    unsigned char id;
    long long weight;
    struct Node *left;
    struct Node *right;
    struct Node *next;
}Node;

// 统计每个字节的频率
int cntByteFreq(const char *filename, long long *freq){
    FILE *fp = fopen(filename, "rb");
    if(fp == NULL){
        fprintf(stderr, "Cannot open file[%s]: ", filename);
        perror("");
        return 1;
    }
    
    int ch;
    while((ch = fgetc(fp)) != EOF){
        freq[(unsigned char)ch]++;
    }
    fclose(fp);
    return 0;
}

// 建树辅助函数：插入排序
void insertSorted(Node **head, Node *newNode){
    if((*head) == NULL || newNode->weight < (*head)->weight){
        newNode->next = *head;
        *head = newNode;
        return;
    }
    Node *current = *head;
    while(current->next != NULL && current->next->weight < newNode->weight){
        current = current->next;
    }
    newNode->next = current->next;
    current->next = newNode;
}

// 建树
Node * buildHuffmanTree(long long *freq){
    Node *head = NULL;

    for(int i = 0; i < 256; i++){
        if(freq[i] > 0){
            Node * newNode = (Node*)malloc(sizeof(Node));
            newNode->id = (unsigned char)i;
            newNode->weight = freq[i];
            newNode->left = newNode->right = NULL;
            newNode->next = NULL;
            insertSorted(&head, newNode);
        }
    }

    while(head != NULL && head->next != NULL){
        Node *min1 = head;
        Node *min2 = head->next;

        head = min2->next;

        Node * parent = (Node*)malloc(sizeof(Node));
        parent->id = 0;
        parent->weight = min1->weight + min2->weight;
        parent->left = min1;
        parent->right = min2;
        parent->next = NULL;

        insertSorted(&head, parent);
    }
    return head;
}

// 编码
void generateCodes(Node *root, char *path, int depth, char **codes){
    if(root == NULL){
        return;
    }
    if(root->left == NULL && root->right == NULL){
        path[depth] = '\0';
        codes[root->id] = (char*)malloc((depth + 1) * sizeof(char));
        strcpy(codes[root->id], path);
        return;
    }
    if(root->left != NULL){
        path[depth] = '0';
        generateCodes(root->left, path, depth + 1, codes);
    }
    if(root->right != NULL){
        path[depth] = '1';
        generateCodes(root->right, path, depth + 1, codes);
    }
}

// 释放树内存
void freeTree(Node *root) {
    if (root == NULL) return;
    freeTree(root->left);
    freeTree(root->right);
    free(root);
}

// 核心修改 1：支持多文件顺序写入同一个目标压缩包
int compressAppendFile(const char *srcFilename, FILE *dest, unsigned char *bits_buffer, int *bit_count) {
    long long freq[256] = {0};
    if (cntByteFreq(srcFilename, freq) != 0) {
        return 1;
    }

    Node *root = buildHuffmanTree(freq);
    if (root == NULL) {
        // 空文件单独处理：只写元数据，不写数据流
        int nameLen = strlen(srcFilename);
        fwrite(freq, sizeof(long long), 256, dest);
        fwrite(&nameLen, sizeof(int), 1, dest);
        fwrite(srcFilename, sizeof(char), nameLen, dest);
        return 0;
    }

    char *huffmanCodes[256] = {NULL};
    char path[256];
    generateCodes(root, path, 0, huffmanCodes);

    FILE *src = fopen(srcFilename, "rb");
    if (src == NULL) {
        perror("Failed to open the original file");
        freeTree(root);
        return 1;
    }

    // 1. 写入当前文件的文件头（元数据）
    int nameLen = strlen(srcFilename);
    fwrite(freq, sizeof(long long), 256, dest);  // 频数表
    fwrite(&nameLen, sizeof(int), 1, dest);       // 文件名长度
    fwrite(srcFilename, sizeof(char), nameLen, dest); // 文件名字符串

    // 2. 读取源文件并进行位编码写入
    int ch;
    while ((ch = fgetc(src)) != EOF) {
        char *code = huffmanCodes[ch];
        for (int i = 0; code[i] != '\0'; i++) {
            *bits_buffer <<= 1;
            if (code[i] == '1') {
                *bits_buffer |= 1;
            }
            (*bit_count)++;

            if (*bit_count == 8) {
                fputc(*bits_buffer, dest);
                *bits_buffer = 0;
                *bit_count = 0;
            }
        }
    }

    fclose(src);
    freeTree(root);
    for (int i = 0; i < 256; i++) {
        if (huffmanCodes[i] != NULL) free(huffmanCodes[i]);
    }
    return 0;
}

// 核心修改 2：多文件解压逻辑
int decompressMultiFile(const char *srcFilename, const char *outputDir) {
    FILE *src = fopen(srcFilename, "rb");
    if (src == NULL) {
        perror("Error opening compressed file");
        return 1;
    }

    int totalFiles = 0;
    if (fread(&totalFiles, sizeof(int), 1, src) != 1) {
        printf("Error reading file count.\n");
        fclose(src);
        return 1;
    }

    unsigned char bits_buffer = 0;
    int bit_count = 0; // 当前缓存里剩余未读的有效位数量

    for (int f = 0; f < totalFiles; f++) {
        long long freq[256] = {0};
        if (fread(freq, sizeof(long long), 256, src) != 256) break;

        int nameLen = 0;
        fread(&nameLen, sizeof(int), 1, src);

        char *origName = (char*)malloc((nameLen + 1) * sizeof(char));
        fread(origName, sizeof(char), nameLen, src);
        origName[nameLen] = '\0';

        // 去掉原本路径里的相对前缀，防止解压错地方，只取纯文件名
        char *pureName = strrchr(origName, '/');
        if (!pureName) pureName = strrchr(origName, '\\');
        if (pureName) pureName++;
        else pureName = origName;

        char destPath[512];
        sprintf(destPath, "%s/%s", outputDir, pureName);
        printf(" -> Extracting: %s\n", destPath);

        FILE *dest = fopen(destPath, "wb");
        if (dest == NULL) {
            perror("Error creating decompressed file");
            free(origName);
            fclose(src);
            return 1;
        }

        long long total_bytes = 0;
        for (int i = 0; i < 256; i++) total_bytes += freq[i];

        Node *root = buildHuffmanTree(freq);
        if (root == NULL) {
            // 说明是空文件
            fclose(dest);
            free(origName);
            continue;
        }

        Node *current = root;
        long long decoded_bytes = 0;

        while (decoded_bytes < total_bytes) {
            if (bit_count == 0) {
                int ch = fgetc(src);
                if (ch == EOF) {
                    printf("Unexpected end of compressed file.\n");
                    break;
                }
                bits_buffer = (unsigned char)ch;
                bit_count = 8;
            }

            // 从高位到低位解析 Bit
            int bit = (bits_buffer >> (bit_count - 1)) & 1;
            bit_count--;

            if (bit == 0) current = current->left;
            else current = current->right;

            if (current->left == NULL && current->right == NULL) {
                fputc(current->id, dest);
                decoded_bytes++;
                current = root;
            }
        }

        fclose(dest);
        freeTree(root);
        free(origName);
    }

    fclose(src);
    return 0;
}

void printUsage(const char *progName) {
    printf("Huffman Multi-File Compression/Decompression Tool\n");
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

        printf("Starting multi-file compression for %d file(s)...\n", inputFilesCount);
        clock_t start = clock();

        FILE *dest = fopen(outputFile, "wb");
        if (dest == NULL) {
            perror("Failed to create the compressed archive");
            return 1;
        }

        // 写入总文件数
        fwrite(&inputFilesCount, sizeof(int), 1, dest);

        unsigned char bits_buffer = 0;
        int bit_count = 0;
        int success = 1;

        // 循环压缩每一个文件
        for (int i = 0; i < inputFilesCount; i++) {
            printf(" -> Compressing: %s\n", argv[fileStartIndex + i]);
            if (compressAppendFile(argv[fileStartIndex + i], dest, &bits_buffer, &bit_count) != 0) {
                success = 0;
                break;
            }
        }

        // 最后一个字节如果没凑满 8 位，强行刷入缓冲区
        if (success && bit_count > 0) {
            bits_buffer <<= (8 - bit_count);
            fputc(bits_buffer, dest);
        }

        fclose(dest);

        if (success) {
            clock_t end = clock();
            double duration = (double)(end - start) / CLOCKS_PER_SEC;
            printf("All files compressed successfully! Time taken: %.3f seconds\n", duration);
        }

    } else if (strcmp(mode, "-x") == 0) {
        char *inputFile = argv[2];
        char *outputDir = ".";

        if (argc >= 5 && strcmp(argv[3], "-d") == 0) {
            outputDir = argv[4];
        }

        printf("Starting multi-file decompression for %s ...\n", inputFile);
        clock_t start = clock();

        if (decompressMultiFile(inputFile, outputDir) == 0) {
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