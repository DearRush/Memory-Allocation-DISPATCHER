/*******************************************************************

  OS Eercises - Project 2 - HOST dispatcher

   mab - memory management functions for HOST dispatcher

   MabPtr memChk (MabPtr arena, int size);
      - check for memory available (any algorithm)
 
    returns address of "First Fit" block or NULL

   int memChkMax (int size);
      - check for over max memory
 
    returns TRUE/FALSE OK/OVERSIZE

   MabPtr memAlloc (MabPtr arena, int size);
      - allocate a memory block
 
    returns address of block or NULL if failure

   MabPtr memFree (MabPtr mab);
      - de-allocate a memory block
 
    returns address of block or merged block

   MabPtr memMerge(Mabptr m);
      - merge m with m->next
 
    returns m

   MabPtr memSplit(Mabptr m, int size);
      - split m into two with first mab having size
  
    returns m or NULL if unable to supply size bytes

   void memPrint(MabPtr arena);
      - print contents of memory arena
   no return

   extern enum memAllocAlg MabAlgorithm; - type of memory algorithm to use
     enum memAllocAlg { FIRST_FIT, NEXT_FIT, BEST_FIT, WORST_FIT};

********************************************************************

  version: 1.0 (exercise 10)
  history:
     v1.0: Original for exercise 10

*******************************************************************/

#include "mab.h"

enum memAllocAlg MabAlgorithm = FIRST_FIT;

static MabPtr next_mab = NULL;    // for NEXT_FIT algorithm

/*******************************************************
 * MabPtr memChk (MabPtr arena, int size);
 *    - check for memory available (any algorithm)
 *
 * returns address of "First Fit" block or NULL
 *******************************************************/
MabPtr memChk(MabPtr arena, int size)
{
    while (arena) {
        if (!arena->allocated && (arena->size >= size)) {
            return arena;
        }
        arena = arena->next;
    }
    return NULL;
}
      
/*******************************************************
 * int memChkMax (int size);
 *    - check for memory available (any algorithm)
 *
 * returns TRUE/FALSE
 *******************************************************/
int memChkMax(int size)
{
    return size > USER_MEMORY_SIZE ? FALSE : TRUE;
}      

/*******************************************************
 * MabPtr memAlloc (MabPtr arena, int size);
 *    - allocate a memory block
 *
 * returns address of block or NULL if failure
 *******************************************************/
MabPtr memAlloc(MabPtr arena, int size)
{
    MabPtr m;

    // First Fit
    if (MabAlgorithm == FIRST_FIT) {
        if ((m = memChk(arena, size)) &&
        (m = memSplit(m, size)))
        m->allocated = TRUE;
        return m;
    }
    // Next Fit
    else if (MabAlgorithm == NEXT_FIT) {

        // TODO

        next_mab = arena;
        if ((m = memChk(next_mab, size)) && (m = memSplit(m, size)))     //从next_mab指向的内存开始查找，其余和FIRST_FIT相似
        {
            m->allocated = TRUE;
            next_mab = m->next;                        //用一个指针指向被分配内存块的下一个内存块
            return m;
        }
        else
        {
            return NULL;
        }

        return NULL;
    }
    // Best Fit
    else if (MabAlgorithm == BEST_FIT) {

        // TODO

        MabPtr a;
        a = arena;                         //用临时变量a记录最初的地址
        int min = 1025;
        while (m = memChk(arena, size))     //遍历链表，找到能够满足内存需求的各个内存块中最小的内存值
        {
            if (min >= m->size)
            {
                min = m->size;              //迭代记录最小值
                arena = arena->next;
            }
            else
            {
                arena = arena->next;
            }
        }
        while (a)
        {
            if (a->size == min)          //找到内存值为最小值的第一个内存块，分配内存
            {
                a = memSplit(a, size);
                a->allocated = TRUE;
                break;
            }
            else
            {
                a = a->next;
            }
        }
        if (min >= 1025)                  //说明没有找到合适的内存块
        {
            return NULL;
        }
        else
        {
            return a;
        }

    }
    // Worst Fit
    else if (MabAlgorithm == WORST_FIT) {

        // TODO
        MabPtr a;
        a = arena;
        int max = 0;
        while (m = memChk(arena, size))     //遍历链表，找到能够满足内存需求的各个内存块中最大的内存值
        {
            if (max <= m->size)
            {
                max = m->size;              //迭代记录最大值
                arena = arena->next;
            }
            else
            {
                arena = arena->next;
            }
        }
        while (a)
        {
            if (a->size == max)          //找到内存值为最大值的第一个内存块，分配内存
            {
                a = memSplit(a, size);
                a->allocated = TRUE;
                break;
            }
            else
            {
                a = a->next;
            }
        }
        if (max <= 0)                  //说明没有找到合适的内存块
        {
            return NULL;
        }
        else
        {
            return a;
        }


        return NULL;
    }
}

/*******************************************************
 * MabPtr memFree (MabPtr mab);
 *    - de-allocate a memory block
 *
 * returns address of block or merged block
 *******************************************************/
MabPtr memFree(MabPtr m)
{
    if (m) {
        m->allocated = FALSE;
        if (m->next && (m->next->allocated == FALSE))
            memMerge(m);
        if (m->prev && (m->prev->allocated == FALSE))
            m = memMerge(m->prev);
    }
    return m;
}
      
/*******************************************************
 * MabPtr memMerge(Mabptr m);
 *    - merge m with m->next
 *
 * returns m
 *******************************************************/
MabPtr memMerge(MabPtr m)
{
    MabPtr n;

    if (m && (n = m->next)) {
        m->next = n->next;
        m->size += n->size;
        
        if (MabAlgorithm == NEXT_FIT &&     
            next_mab == n) next_mab = m;

        free (n);
        if (m->next) (m->next)->prev = m;
    }
    return m;
}

/*******************************************************
 * MabPtr memSplit(MabPtr m, int size);
 *    - split m into two with first mab having size
 *
 * returns m or NULL if unable to supply size bytes
 *******************************************************/
MabPtr memSplit(MabPtr m, int size)
{
    MabPtr n;
    
    if (m) {
        if (m->size > size) {
            n = (MabPtr) malloc( sizeof(Mab) );
            if (!n) {
                fprintf(stderr,"memory allocation error\n");
                exit(127);
            }
            n->offset = m->offset + size;
            n->size = m->size - size;
            m->size = size;
            n->allocated = m->allocated;
            n->next = m->next;
            m->next = n;
            n->prev = m;
            if (n->next) n->next->prev = n;
        }
        if (m->size == size) return m;
    }
    return NULL;    
}

/*******************************************************
 * void memPrint(MabPtr arena);
 *    - print contents of memory arena
 * no return
 *******************************************************/
void memPrint(MabPtr arena)
{
    while(arena) {
        printf("offset%7d: size%7d - ",arena->offset, arena->size);
        if (arena->allocated) printf("allocated\n");
        else printf("free\n");
        arena = arena->next;
    }
}

