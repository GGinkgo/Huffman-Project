#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct Node {
    unsigned char id;
    long long weight;
    struct Node *left;
    struct Node *right;
    struct Node *next;
} Node;

int real_file_count = 0;

int cntByteFreq(const char *filename, long long *freq) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open file[%s]: ", filename);
        perror("");
        return 1;
    }
    int ch;
    while ((ch = fgetc(fp)) != EOF) {
        freq[(unsigned char)ch]++;
    }
    fclose(fp);
    return 0;
}

void insertSorted(Node **head, Node *newNode) {
    if ((*head) == NULL || newNode->weight < (*head)->weight) {
        newNode->next = *head;
        *head = newNode;
        return;
    }
    Node *current = *head;
    while (current->next != NULL && current->next->weight < newNode->weight) {
        current = current->next;
    }
    newNode->next = current->next;
    current->next = newNode;
}

Node *buildHuffmanTree(long long *freq) {
    Node *head = NULL;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            Node *newNode = (Node*)malloc(sizeof(Node));
            newNode->id = (unsigned char)i;
            newNode->weight = freq[i];
            newNode->left = newNode->right = NULL;
            newNode->next = NULL;
            insertSorted(&head, newNode);
        }
    }
    while (head != NULL && head->next != NULL) {
        Node *min1 = head;
        Node *min2 = head->next;
        head = min2->next;

        Node *parent = (Node*)malloc(sizeof(Node));
        parent->id = 0;
        parent->weight = min1->weight + min2->weight;
        parent->left = min1;
        parent->right = min2;
        parent->next = NULL;
        insertSorted(&head, parent);
    }
    return head;
}

void generateCodes(Node *root, char *path, int depth, char **codes) {
    if (root == NULL) return;
    if (root->left == NULL && root->right == NULL) {
        path[depth] = '\0';
        codes[root->id] = (char*)malloc((depth + 1) * sizeof(char));
        strcpy(codes[root->id], path);
        return;
    }
    if (root->left != NULL) {
        path[depth] = '0';
        generateCodes(root->left, path, depth + 1, codes);
    }
    if (root->right != NULL) {
        path[depth] = '1';
        generateCodes(root->right, path, depth + 1, codes);
    }
}

void freeTree(Node *root) {
    if (root == NULL) return;
    freeTree(root->left);
    freeTree(root->right);
    free(root);
}

int compressAppendFile(const char *srcFilename, FILE *dest, unsigned char *bits_buffer, int *bit_count) {
    long long freq[256] = {0};
    if (cntByteFreq(srcFilename, freq) != 0) return 1;

    Node *root = buildHuffmanTree(freq);
    int nameLen = strlen(srcFilename);

    if (root == NULL) { 
        fwrite(freq, sizeof(long long), 256, dest);
        fwrite(&nameLen, sizeof(int), 1, dest);
        fwrite(srcFilename, sizeof(char), nameLen, dest);
        real_file_count++;
        return 0;
    }

    char *huffmanCodes[256] = {NULL};
    char path[256];
    generateCodes(root, path, 0, huffmanCodes);

    FILE *src = fopen(srcFilename, "rb");
    if (src == NULL) {
        perror("Failed to open original file");
        freeTree(root);
        return 1;
    }

    fwrite(freq, sizeof(long long), 256, dest);
    fwrite(&nameLen, sizeof(int), 1, dest);
    fwrite(srcFilename, sizeof(char), nameLen, dest);

    int ch;
    while ((ch = fgetc(src)) != EOF) {
        char *code = huffmanCodes[ch];
        for (int i = 0; code[i] != '\0'; i++) {
            *bits_buffer <<= 1;
            if (code[i] == '1') *bits_buffer |= 1;
            (*bit_count)++;

            if (*bit_count == 8) {
                fputc(*bits_buffer, dest);
                *bits_buffer = 0;
                *bit_count = 0;
            }
        }
    }

    if (*bit_count > 0) {
        *bits_buffer <<= (8 - *bit_count);
        fputc(*bits_buffer, dest);
        *bits_buffer = 0;
        *bit_count = 0;
    }

    fclose(src);
    freeTree(root);
    for (int i = 0; i < 256; i++) {
        if (huffmanCodes[i] != NULL) free(huffmanCodes[i]);
    }
    real_file_count++;
    return 0;
}

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
    int bit_count = 0;

    for (int f = 0; f < totalFiles; f++) {
  
        bits_buffer = 0;
        bit_count = 0;

        long long freq[256] = {0};
        if (fread(freq, sizeof(long long), 256, src) != 256) break;

        int nameLen = 0;
        fread(&nameLen, sizeof(int), 1, src);

        char *origName = (char*)malloc((nameLen + 1) * sizeof(char));
        fread(origName, sizeof(char), nameLen, src);
        origName[nameLen] = '\0';

        char *pureName = strrchr(origName, '/');
        if (!pureName) pureName = strrchr(origName, '\\');
        if (pureName) pureName++;
        else pureName = origName;

        char destPath[512];
        sprintf(destPath, "%s/%s", outputDir, pureName);

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

void processPath(const char *path, FILE *dest, unsigned char *bits_buffer, int *bit_count, int *success) {
    if (!*success) return;
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "Path does not exist: %s\n", path);
        return;
    }

    if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
        char searchPath[MAX_PATH];
        sprintf(searchPath, "%s\\*", path);

        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(searchPath, &findData);
        if (hFind == INVALID_HANDLE_VALUE) return;

        do {
            if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0) {
                continue;
            }
            char nextPath[MAX_PATH];
            sprintf(nextPath, "%s\\%s", path, findData.cFileName);
            processPath(nextPath, dest, bits_buffer, bit_count, success);
        } while (FindNextFileA(hFind, &findData));

        FindClose(hFind);
    } else {
        if (compressAppendFile(path, dest, bits_buffer, bit_count) != 0) {
            fprintf(stderr, "Failed to compress file: %s\n", path);
            *success = 0;
        } 
    }
}

void printUsage(const char *progName) {
    printf("Huffman Multi-File Compression/Decompression Tool\n");
    printf("Usage:\n");
    printf("  Compress: %s -c -o <output.huff> <path1> [path2]...\n", progName);
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

        int inputPathsCount = argc - fileStartIndex;
        if (inputPathsCount == 0) {
            printf("Error: No input paths specified!\n");
            return 1;
        }

        printf("Starting compression process...\n");
        clock_t start = clock();

        FILE *dest = fopen(outputFile, "wb");
        if (dest == NULL) {
            perror("Failed to create compressed archive");
            return 1;
        }

        int placeholder = 0;
        fwrite(&placeholder, sizeof(int), 1, dest);

        unsigned char bits_buffer = 0;
        int bit_count = 0;
        int success = 1;

        for (int i = 0; i < inputPathsCount; i++) {
            processPath(argv[fileStartIndex + i], dest, &bits_buffer, &bit_count, &success);
        }

        if (success && bit_count > 0) {
            bits_buffer <<= (8 - bit_count);
            fputc(bits_buffer, dest);
        }

        if (success) {
            fseek(dest, 0, SEEK_SET);
            fwrite(&real_file_count, sizeof(int), 1, dest);
        }
        fclose(dest);

        if (success) {
            clock_t end = clock();
            double duration = (double)(end - start) / CLOCKS_PER_SEC;
            printf("Successfully compressed %d file(s)! Time taken: %.3f seconds\n", real_file_count, duration);
        }

    } else if (strcmp(mode, "-x") == 0) {
        char *inputFile = argv[2];
        char *outputDir = ".";

        if (argc >= 5 && strcmp(argv[3], "-d") == 0) {
            outputDir = argv[4];
        }

        printf("Starting decompression for %s...\n", inputFile);
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