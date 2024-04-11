#ifndef MEMORY_H
#define MEMORY_H

#include <stdlib.h>
#include <stdbool.h>

#include "constants.h"

// Definição da estrutura MemoryBlock, que representa um bloco de memória.
typedef struct {
    int words[WORDS_SIZE];
} MemoryBlock;

// Definição da estrutura RAM, que contém os blocos de memória e o tamanho da RAM.
typedef struct {
    MemoryBlock* blocks;
    int size;
} RAM;

// Definição da estrutura Line, que representa uma linha na cache.
typedef struct {
    MemoryBlock block;
    int tag; /* Endereço do bloco na memória RAM */
    bool updated;

    int cost;
    int cacheHit;
    int timesUsed; // (LFU) - Contador de frequência de uso para política LFU (Least Frequently Used).
    int timeInCache; // (LRU) - Contador de tempo de permanência na cache para política LRU (Least Recently Used).
} Line;

// Definição da estrutura Cache, que contém as linhas da cache e o tamanho da cache.
typedef struct {
    Line* lines;
    int size;
} Cache;

// Protótipos das funções relacionadas à cache
void startCache(Cache*, int);
void stopCache(Cache*);

// Protótipos das funções relacionadas à RAM
void startRAM(RAM*, int);
void stopRAM(RAM*);

#endif // !MEMORY_H
