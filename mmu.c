#include <stdio.h>
#include "mmu.h"

// Função para converter o tipo WhereWasHit em uma string correspondente
char *convertToString(WhereWasHit whereWasHit)
{
    switch (whereWasHit)
    {
    case L1Hit:
        return "CL1";
    case L2Hit:
        return "CL2";
    case L3Hit:
        return "CL3";
    case RAMHit:
        return "RAM";
    default:
        return "";
    }
}

// Verifica se o bloco pode ser substituído na linha da cache
bool canOnlyReplaceBlock(Line line)
{
    // Retorna verdadeiro se o bloco for inválido ou se não tiver sido atualizado
    if (line.tag == INVALID_ADD || (line.tag != INVALID_ADD && !line.updated))
        return true;
    return false;
}

// Mapeamento direto
int directMapping(int address, Cache *cache)
{
    // Calcula a posição da linha na cache usando o resto da divisão do endereço pelo tamanho da cache
    return address % cache->size;
}

// Least Recently Used (LRU)
int lru(int address, Cache *cache)
{
    int leastRecentlyUsed = 0;

    for (int i = 0; i < cache->size; i++)
    {
        // Verifica se o bloco está na linha atual da cache
        if (cache->lines[i].tag == address)
            return i;

        // Encontra a linha menos recentemente usada na cache
        if (cache->lines[i].timeInCache > cache->lines[leastRecentlyUsed].timeInCache)
            leastRecentlyUsed = i;
    }

    return leastRecentlyUsed;
}

// Least Frequently Used (LFU)
int lfu(int address, Cache *cache)
{
    int leastFrequentlyUsed = 0;

    for (int i = 0; i < cache->size; i++)
    {
        // Verifica se o bloco está na linha atual da cache
        if (cache->lines[i].tag == address)
            return i;

        // Encontra a linha menos frequentemente usada na cache
        if (cache->lines[i].timesUsed < cache->lines[leastFrequentlyUsed].timesUsed)
            leastFrequentlyUsed = i;
    }

    return leastFrequentlyUsed;
}

// First In First Out (FIFO)
int fifo(int address, Cache *cache)
{
    int oldestLine = 0;

    // Procura por um espaço vazio na cache ou pelo endereço que está sendo procurado
    for (int i = 0; i < cache->size; i++)
    {
        if (!cache->lines[i].updated || cache->lines[i].tag == address)
            return i;

        // Encontra a linha mais antiga na cache
        if (cache->lines[i].timeInCache < cache->lines[oldestLine].timeInCache)
            oldestLine = i;
    }

    // Nenhum espaço vazio e endereço não está na cache, substitui a linha mais antiga
    return oldestLine;
}

// Seleciona o tipo de mapeamento de memória
int memoryCacheMapping(int address, Cache *cache)
{
    int mappPolicyType = 1;

    switch (mappPolicyType)
    {
    // Mapeamento direto
    case 1:
        return directMapping(address, cache);

    // Least Recently Used (LRU)
    case 2:
        return lru(address, cache);

    // Least Frequently Used (LFU)
    case 3:
        return lfu(address, cache);

    // First In First Out (FIFO)
    case 4:
        return fifo(address, cache);

    default:
        return -1;
    }
}

// Atualiza as informações de hits e misses de cada nível de memória
void updateMachineInfos(Machine *machine, Line *line, WhereWasHit *whereWasHit, int cost)
{
    switch (*whereWasHit)
    {
    case L1Hit:
        machine->hitL1 += 1;
        break;

    case L2Hit:
        machine->hitL2 += 1;
        machine->missL1 += 1;
        break;

    case L3Hit:
        machine->hitL3 += 1;
        machine->missL1 += 1;
        machine->missL2 += 1;
        break;

    case RAMHit:
        machine->hitRAM += 1;
        machine->missL1 += 1;
        machine->missL2 += 1;
        machine->missL3 += 1;
        break;
    }

    line->timeInCache = 0;
    line->timesUsed += 1;
    machine->totalCost += cost;
}

// Busca um endereço nas memórias (caches e RAM)
Line *MMUSearchOnMemorys(Address add, Machine *machine, WhereWasHit *whereWasHit)
{
    // Strategy => write back

    // Procura nas caches
    int l1pos = memoryCacheMapping(add.block, &machine->l1);
    int l2pos = memoryCacheMapping(add.block, &machine->l2);
    int l3pos = memoryCacheMapping(add.block, &machine->l3);

    Line *cache1 = machine->l1.lines;
    Line *cache2 = machine->l2.lines;
    Line *cache3 = machine->l3.lines;
    MemoryBlock *RAM = machine->ram.blocks;

    // Custo do acesso
    int cost = 0;

    // Incrementa o tempo de permanência de todas as linhas na cache L1
    for (int i = 0; i < machine->l1.size; i++)
    {
        machine->l1.lines[i].timeInCache += 1;
    }

    // Incrementa o tempo de permanência de todas as linhas na cache L2
    for (int i = 0; i < machine->l2.size; i++)
    {
        machine->l2.lines[i].timeInCache += 1;
    }

    // Incrementa o tempo de permanência de todas as linhas na cache L3
    for (int i = 0; i < machine->l3.size; i++)
    {
        machine->l3.lines[i].timeInCache += 1;
    }

    // Cache L1
    if (cache1[l1pos].tag == add.block)
    {
        // Se o bloco estiver na cache L1
        cost = COST_ACCESS_L1;
        cache1[l1pos].cacheHit = 1;
        *whereWasHit = L1Hit;
    }

    // Cache L2
    else if (cache2[l2pos].tag == add.block)
    {
        // Se o bloco estiver na cache L2
        cost = COST_ACCESS_L1 + COST_ACCESS_L2;
        cache2[l2pos].tag = add.block;
        cache2[l2pos].updated = false;
        cache2[l2pos].cacheHit = 2;

        *whereWasHit = L2Hit;
        updateMachineInfos(machine, &cache2[l2pos], whereWasHit, cost);
        return &(cache2[l2pos]);
    }

    // Cache L3
    else if (cache3[l3pos].tag == add.block)
    {
        // Se o bloco estiver na cache L3
        cost = COST_ACCESS_L1 + COST_ACCESS_L2 + COST_ACCESS_L3;
        cache3[l3pos].tag = add.block;
        cache3[l3pos].updated = false;
        cache3[l3pos].cacheHit = 3;

        *whereWasHit = L3Hit;
        updateMachineInfos(machine, &cache3[l3pos], whereWasHit, cost);
        return &(cache3[l3pos]);
    }

    // RAM
    else
    {
        // Se o bloco não estiver em nenhuma das caches
        if (!canOnlyReplaceBlock(cache1[l1pos]))
        {
            if (!canOnlyReplaceBlock(cache2[l2pos]))
            {
                if (!canOnlyReplaceBlock(cache3[l3pos]))
                    RAM[cache3[l3pos].tag] = cache3[l3pos].block;

                cache3[l3pos] = cache2[l2pos];
                machine->l3.lines[l3pos].timeInCache = 0;
            }

            cache2[l2pos] = cache1[l1pos];
            machine->l2.lines[l2pos].timeInCache = 0;
        }

        cache1[l1pos].block = RAM[add.block];

        machine->l1.lines[l1pos].timeInCache = 0;

        cache1[l1pos].tag = add.block;
        cache1[l1pos].updated = false;
        cache1[l1pos].cacheHit = 4;

        cost = COST_ACCESS_L1 + COST_ACCESS_L2 + COST_ACCESS_L3 + COST_ACCESS_RAM;

        *whereWasHit = RAMHit;
    }

    // Atualiza as informações e retorna a linha da cache
    updateMachineInfos(machine, &cache1[l1pos], whereWasHit, cost);

    return &(cache1[l1pos]);
}
