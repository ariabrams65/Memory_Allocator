#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

#define MAXLINE 127
#define MAXARGS 4
#define HEAPSIZE 127
#define MAXBLOCKS 127 / 3

int tokenize(char* line, char** vector)
{
    const char delim[3] =  " \t\n";

    char* word = strtok(line, delim);
    int i;
    for (i = 0; word != NULL; ++i)
    {
        vector[i] = word;
        word = strtok(NULL, delim);
    }
    vector[i] = NULL;
    return i;
}

void init_heap_to_unalloc(unsigned char* heap, char size)
{
    heap[0] = size;
    heap[0] = heap[0] << 1;
    heap[size - 1] = heap[0];
    memset(heap + 1, 0, size - 2);
}

int next_block(int block, unsigned char* heap)
{
    int next = (heap[block] >> 1) + block;
    if (next >= HEAPSIZE)
    {
        return -1;
    }
    return next;
}

int prev_block(int block, unsigned char* heap)
{
    if (block == 0)
    {
        return -1;
    }
    return block - (heap[block - 1] >> 1);
}

int _malloc(unsigned char* heap, int size, int(*find_unalloc_block) (unsigned char*, int))
{
    int block = find_unalloc_block(heap, size); 
    int block_size = heap[block] >> 1;

    if (block_size - size >= 3)
    {
        int first_meta = ((size + 2) << 1) | 1;  
        heap[block] = first_meta;
        heap[block + size + 1] = first_meta;

        int second_meta = (block_size - (size + 2)) << 1;
        heap[block + size + 2] = second_meta;
        heap[block + block_size - 1] = second_meta;
    } else
    {
        heap[block] |= 1;
        heap[block + block_size - 1] |= 1;
    }
    return block + 1;
}

void _free(int index, unsigned char* heap)
{
    int block = index - 1;
    int footer_index = block + (heap[block] >> 1) - 1;
    heap[block] &= -2; 
    heap[footer_index] &= -2;
    memset(heap + block + 1, 0, (heap[block] >> 1) - 2);

    int next = next_block(block, heap);
    if (next != -1 && ((heap[next] & 1) == 0))
    {
        heap[block] += heap[next]; 
        footer_index = next + (heap[next] >> 1) - 1;
        heap[footer_index] = heap[block];
    }
    int prev = prev_block(block, heap);
    if (prev != -1 && ((heap[prev] & 1) == 0))
    {
        heap[prev] += heap[block];
        heap[footer_index] = heap[prev];
    }
}

int first_fit(unsigned char* heap, int size)
{
    int block = 0;
    int alloc_bit = heap[block] & 1;
    int block_size = heap[block] >> 1;
    while (alloc_bit != 0 || block_size - 2 < size)
    {
       block += block_size; 
       alloc_bit = heap[block] & 1;
       block_size = heap[block] >> 1;
    }
    return block;
}

int best_fit(unsigned char* heap, int size)
{
    int block = 0;
    int best_fit_block;
    int best_fit_payload_size = INT_MAX;
    while (block != -1)
    {
        int payload_size = (heap[block] >> 1) - 2;
        if ((heap[block] & 1) == 0 && (payload_size >= size) &&
            (payload_size < best_fit_payload_size)) 
        {
            best_fit_block = block;
            best_fit_payload_size = payload_size;
        }
        block = next_block(block, heap);
    }
    return best_fit_block; 
}

struct Block
{
    int payload_size;
    int index;
    bool allocated;
};

int init_blocklist(struct Block* blocklist, unsigned char* heap)
{
    int i = 0;
    int block = 0;
    while (block != -1)
    {
        blocklist[i].payload_size = (heap[block] >> 1) - 2; 
        blocklist[i].index = block + 1;
        blocklist[i].allocated = (heap[block] & 1) == 1;
        ++i;
        block = next_block(block, heap);
    }
    return i;
}

int payload_size_cmp(const void* a, const void* b)
{
    return ((struct Block*)b)->payload_size - ((struct Block*)a)->payload_size;
}

void print_blocklist(unsigned char* heap)
{
    struct Block blocklist[MAXBLOCKS]; 
    int num_blocks = init_blocklist(blocklist, heap);
    qsort(blocklist, num_blocks, sizeof(struct Block), payload_size_cmp);
    int i;
    for (i = 0; i < num_blocks; ++i)
    {
        printf("%i-%i-%s\n", blocklist[i].payload_size, blocklist[i].index,
            blocklist[i].allocated ? "allocated" : "free");
    }
}

void writemem(int index, char* chars, unsigned char* heap)
{
    while (*chars != '\0')
    {
        heap[index] = *chars;
        ++index;
        ++chars;
    }
}

void printmem(int index, int num, unsigned char* heap)
{
    int i;
    for (i = 0; i < num - 1; ++i)
    {
        printf("%i-", heap[index + i]);
    }
    printf("%i\n", heap[index + i]);
}

int main(int argc, char** argv)
{
    int(*find_unalloc_block) (unsigned char*, int) = first_fit;
    if (argc > 1 && strcmp(argv[1], "BestFit") == 0)
    {
        find_unalloc_block = best_fit;
    }
    unsigned char heap[HEAPSIZE]; 
    init_heap_to_unalloc(heap, HEAPSIZE);

    printf("> ");
    char cmdline[MAXLINE];
    char* vector[MAXARGS];
    fgets(cmdline, MAXLINE, stdin);
    tokenize(cmdline, vector);

    while (strcmp(vector[0], "quit") != 0)
    {
        if (strcmp(vector[0], "malloc") == 0)
        {
            printf("%i\n", _malloc(heap, atoi(vector[1]), find_unalloc_block));
        } else if (strcmp(vector[0], "free") == 0)
        {
            _free(atoi(vector[1]), heap); 
        } else if (strcmp(vector[0], "blocklist") == 0)
        {
            print_blocklist(heap);
        } else if (strcmp(vector[0], "writemem") == 0)
        {
            writemem(atoi(vector[1]), vector[2], heap);
        } else if (strcmp(vector[0], "printmem") == 0)
        {
            printmem(atoi(vector[1]), atoi(vector[2]), heap);
        }
        printf("> ");
        fgets(cmdline, MAXLINE, stdin);
        tokenize(cmdline, vector);
    }
}
