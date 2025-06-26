#include <Windows.h>
#include <stdio.h>
#include <time.h>

#include "../linkedlist/linkedlist.h"
#include "hashtable.h"

static DWORD ModularExponentiation(DWORD dwBase,
                                   DWORD dwExponent,
                                   DWORD dwModulus)
{
    DWORD dwResult = 1;
    dwBase         = dwBase % dwModulus;

    while (dwExponent > 0)
    {
        if (dwExponent % 2 == 1)
        {
            dwResult = (dwResult * dwBase) % dwModulus;
        }

        dwBase     = (dwBase * dwBase) % dwModulus;
        dwExponent = dwExponent >> 1;
    }

    return dwResult;
}

BOOL IsPrime(WORD wValue)
{
    // NOTE: Factoring out powers of two.
    DWORD dwOddInt     = wValue - 1;
    DWORD dwPowerOfTwo = 0;
    DWORD dwMaxNum     = wValue - 2;
    DWORD dwMinNum     = 2;
    DWORD dwRoundCount = 5;
    DWORD dwCounterOne = 0;
    DWORD dwPrimeBase  = 0;
    DWORD dwCalcOne    = 0;
    DWORD dwCalcTwo    = 0;
    DWORD dwCounterTwo = 0;
    BOOL  bReturn      = FALSE;

    if (wValue <= 1)
    {
        return FALSE;
    }
    else if (wValue <= 3)
    {
        return TRUE;
    }

    while (dwOddInt % 2 == 0)
    {
        dwPowerOfTwo += 1;
        dwOddInt = dwOddInt / 2;
    }

    // NOTE: Seeding inside the loop would generate the same number each time
    // due to the computational speed.
    srand((DWORD)time(NULL));

    for (dwCounterOne = 0; dwCounterOne < dwRoundCount; dwCounterOne++)
    {
        // NOTE: Getting the number within range will decrease the randomness
        // slightly our base for prime calculations will be determined.
        dwPrimeBase = rand() % (dwMaxNum + 1 - dwMinNum) + dwMinNum;
        dwCalcOne   = ModularExponentiation(dwPrimeBase, dwOddInt, wValue);
        dwCalcTwo   = 0;

        // NOTE: Test to see if strong base, will skip to next loop if not.
#pragma warning(push)
#pragma warning(disable : 4389)
        // NOTE: wValue can never be below four at this point in the code:
        // compiler warnings can safely be ignored here.
        if ((1 == dwCalcOne) || ((wValue - 1) == dwCalcOne))
#pragma warning(pop)

        {
            continue;
        }

        for (dwCounterTwo = 0; dwCounterTwo < dwPowerOfTwo; dwCounterTwo++)
        {
            dwCalcTwo = ModularExponentiation(dwCalcOne, 2, wValue);
#pragma warning(push)
#pragma warning(disable : 4389)
            // NOTE: wValue can never be below four at this point in the code:
            // compiler warnings can safely be ignored here.
            if ((1 == dwCalcTwo) && (1 != dwCalcOne) &&
                (dwCalcOne != (wValue - 1)))
#pragma warning(pop)
            {
                goto EXIT;
            }

            dwCalcOne = dwCalcTwo;
        }

        if (1 != dwCalcTwo)
        {
            goto EXIT;
        }
    }

    bReturn = TRUE;
EXIT:
    return bReturn;
}

WORD NextPrime(WORD wValue)
{
    WORD wReturn      = 0;
    WORD wDoubleValue = 0;
    WORD wLoopValue   = 0;

    if (wValue <= 1)
    {
        return 2;
    }

    if (wValue == 2)
    {
        return 3;
    }

    // NOTE: The search will start at the next odd number.
    if (wValue % 2 == 0)
    {
        wValue += 1;
    }
    else
    {
        wValue += 2;
    }

    wDoubleValue = wValue * 2;

    if (wValue > 32767)
    {
        wDoubleValue = 65535;
    }

    // NOTE: Bertrand's postulate: for any int  > 3 there exists at least one
    // prime number, p, such that n < p < 2n.
    for (wLoopValue = wValue; wLoopValue < wDoubleValue; wLoopValue += 2)
    {
        if (IsPrime(wLoopValue))
        {
            wReturn = wLoopValue;
            goto EXIT;
        }
    }

EXIT:
    return wReturn;
}

// NOTE: Return EXIT_SUCCESS if equal, EXIT_FAILURE if not.
// WARNING: pszKEy2 is allocated by user. If pszKeyLen is longer, accessing
// un-allocated memory.
static RETURNTYPE CompareMemory(PCHAR pszKey1, PCHAR pszKey2, WORD wKeyLen)
{
    RETURNTYPE Return = SUCCESS;

    for (INT iCounter = 0; iCounter < wKeyLen; iCounter++)
    {
        if (pszKey1[iCounter] != pszKey2[iCounter])
        {
            Return = ERR_GENERIC;
            goto EXIT;
        }
    }

EXIT:
    return Return;
}

static DWORD DefaultHashFunction(PVOID pKey)
{
    PWCHAR      pcaKey      = (PWCHAR)pKey;
    const DWORD dwFNVOffset = 2166136261;
    const DWORD dwFNVPrime  = 16777619;
    DWORD       dwHash      = dwFNVOffset;
    WORD        wCounter    = 0;

    for (wCounter = 0; wCounter <= KEY_LENGTH; wCounter++)
    {
        if (pcaKey[wCounter] == 0)
        {
            break;
        }
        dwHash = dwHash * dwFNVPrime;
        dwHash ^= pcaKey[wCounter];
    }

    return dwHash;
}

RETURNTYPE
HashTableInit(PPHASHTABLE ppHashTable,
              WORD        wCapacity,
              DWORD (*pfnHashFunction)(PVOID))
{
    RETURNTYPE Return = ERR_GENERIC;
    PHASHTABLE pHashTable =
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HASHTABLE));

    if (NULL == pHashTable)
    {
        DEBUG_ERROR("Failed to allocate hash table");
        goto EXIT;
    }

    if (NULL != pfnHashFunction)
    {
        pHashTable->m_pfnHashFunction = pfnHashFunction;
    }
    else
    {
        pHashTable->m_pfnHashFunction = DefaultHashFunction;
    }

    if (MIN_CAPACITY > wCapacity)
    {
        pHashTable->m_wCapacity = MIN_CAPACITY;
    }
    else
    {
        pHashTable->m_wCapacity = wCapacity;
    }

    pHashTable->m_ppTable =
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                  pHashTable->m_wCapacity * sizeof(PLINKEDLIST));
    if (NULL == pHashTable->m_ppTable)
    {
        DEBUG_ERROR("Failed to allocate hash table array");
        goto CLEAN;
    }

    Return       = SUCCESS;
    *ppHashTable = pHashTable;
    goto EXIT;
CLEAN:
    ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pHashTable,
                             sizeof(HASHTABLE));
EXIT:
    return Return;
}

static RETURNTYPE DuplicateDataCheck(PLINKEDLIST pLinkedList, PCHAR pszKey)
{
    RETURNTYPE      Return     = ERR_GENERIC;
    WORD            wCounter   = 0;
    PHASHTABLEENTRY pTempEntry = NULL;

    if ((NULL == pLinkedList) || (NULL == pszKey))
    {
        DEBUG_PRINT("Input NULL");
        goto EXIT;
    }

    for (wCounter = 0; wCounter < pLinkedList->m_wSize; wCounter++)
    {
        pTempEntry = LinkedListReturn(pLinkedList, wCounter);
        if (NULL == pTempEntry)
        {
            DEBUG_PRINT("LinkedListReturn failed");
            goto EXIT;
        }

        if (EXIT_SUCCESS ==
            CompareMemory(pTempEntry->m_caKey, pszKey, pTempEntry->m_wKeyLen))
        {
            DEBUG_PRINT("Duplicate key found");
            Return = ERR_INVALID_PARAM;
            goto EXIT;
        }
    }

    Return = SUCCESS;
EXIT:
    return Return;
}

static RETURNTYPE HashTableNewEntryCalc(PHASHTABLE      pHashTable,
                                        PHASHTABLEENTRY pNewEntry)
{
    RETURNTYPE  Return       = ERR_GENERIC;
    DWORD       dwHash       = 0;
    DWORD       dwEntryIndex = 0;
    PLINKEDLIST pTempList    = NULL;

    if (((NULL == pHashTable) || (NULL == pHashTable->m_pfnHashFunction) ||
         (NULL == pNewEntry)))
    {
        DEBUG_PRINT("Input NULL");
        return EXIT_FAILURE;
    }

    dwHash = pHashTable->m_pfnHashFunction(pNewEntry->m_caKey);

    dwEntryIndex = dwHash % (DWORD)pHashTable->m_wCapacity;

    if (NULL == pHashTable->m_ppTable[dwEntryIndex])
    {
        LinkedListInit(&pTempList);
        if (NULL == pTempList)
        {
            DEBUG_PRINT("LinkedListInit failed");
            goto EXIT;
        }

        if (EXIT_FAILURE == LinkedListInsert(pTempList, pNewEntry, 0))
        {
            DEBUG_PRINT("LinkedListInsert failed");
            goto EXIT;
        }

        pHashTable->m_ppTable[dwEntryIndex] = pTempList;
    }
    else
    {
        pTempList = pHashTable->m_ppTable[dwEntryIndex];

        Return = DuplicateDataCheck(pTempList, pNewEntry->m_caKey);
        if (SUCCESS != Return)
        {
            if (ERR_GENERIC == Return)
            {
                DEBUG_PRINT("DuplicateDataCheck failed");
            }
            goto EXIT;
        }
    }

    pHashTable->m_wSize++;
    Return = SUCCESS;
EXIT:
    return Return;
}

// NOTE: On error, dLoadFactor will be less than zero.
static double LoadFactor(PHASHTABLE pHashTable)
{
    double dLoadFactor = -1;

    if (NULL == pHashTable)
    {
        DEBUG_PRINT("Input NULL");
        goto EXIT;
    }

    if (0.0 == (double)pHashTable->m_wCapacity)
    {
        DEBUG_PRINT("Stop trying to divide by zero");
        goto EXIT;
    }

    dLoadFactor =
        ((double)pHashTable->m_wSize) / ((double)pHashTable->m_wCapacity);

EXIT:
    return dLoadFactor;
}

static PLINKEDLIST HashTabletoList(PHASHTABLE pHashTable)
{
    PLINKEDLIST     pHashTableList = NULL;
    WORD            wCounter       = 0;
    PLINKEDLIST     pTempList      = NULL;
    WORD            iTempSize      = 0;
    WORD            wCounter2      = 0;
    PHASHTABLEENTRY pTempEntry     = NULL;

    if (NULL == pHashTable)
    {
        DEBUG_PRINT("Input NULL");
        return NULL;
    }

    LinkedListInit(&pHashTableList);
    if (NULL == pHashTableList)
    {
        DEBUG_PRINT("LinkedListInit failed");
        return NULL;
    }

    for (wCounter = 0; wCounter < pHashTable->m_wCapacity; wCounter++)
    {
        if (NULL != pHashTable->m_ppTable[wCounter])
        {
            pTempList = pHashTable->m_ppTable[wCounter];
            iTempSize = pTempList->m_wSize;

            for (wCounter2 = 0; wCounter2 < iTempSize; wCounter2++)
            {
                pTempEntry = LinkedListReturn(pTempList, wCounter2);
                if (NULL == pTempEntry)
                {
                    DEBUG_PRINT("LinkedListReturn()");
                    goto CLEAN;
                }

                if (SUCCESS != LinkedListInsert(pHashTableList, pTempEntry,
                                                pHashTableList->m_wSize))
                {
                    DEBUG_PRINT("LinkedListInsert()");
                    goto CLEAN;
                }
            }

            if (SUCCESS != LinkedListDestroy(pTempList, NULL))
            {
                DEBUG_PRINT("LinkedListDestroy failed");
                goto CLEAN;
            }

            pHashTable->m_ppTable[wCounter] = NULL;
        }
    }

    goto EXIT;
CLEAN:
    LinkedListDestroy(pHashTableList, NULL);
EXIT:
    return pHashTableList;
}

static RETURNTYPE HashTableListToTable(PHASHTABLE  pHashTable,
                                       PLINKEDLIST pLinkedList)
{
    RETURNTYPE      Return     = ERR_GENERIC;
    WORD            iTempSize  = 0;
    WORD            wCounter   = 0;
    PHASHTABLEENTRY pTempEntry = NULL;

    if ((NULL == pHashTable) || (NULL == pLinkedList))
    {
        DEBUG_PRINT("Input NULL");
        goto EXIT;
    }

    iTempSize = pLinkedList->m_wSize;

    for (wCounter = 0; wCounter < iTempSize; wCounter++)
    {
        pTempEntry = LinkedListReturn(pLinkedList, wCounter);

        if (NULL == pTempEntry)
        {
            DEBUG_PRINT("LinkedListReturn failed");
            Return = ERR_GENERIC;
            goto EXIT;
        }

        Return = HashTableNewEntryCalc(pHashTable, pTempEntry);
        if (EXIT_SUCCESS != Return)
        {
            if (EXIT_FAILURE != Return)
            {
                DEBUG_PRINT("HashTableNewEntryCalc failed");
            }
            goto EXIT;
        }
    }

EXIT:
    return Return;
}

static RETURNTYPE HashTableReHash(PHASHTABLE pHashTable)
{
    RETURNTYPE  Return          = SUCCESS;
    PLINKEDLIST pLinkedList     = NULL;
    WORD        wCapacityHolder = pHashTable->m_wCapacity;

    if (NULL == pHashTable)
    {
        DEBUG_PRINT("Input NULL");
        goto EXIT;
    }

    if (pHashTable->m_wCapacity >= MAX_PRIME_IN_RANGE)
    {
        DEBUG_PRINT("Re-hashing not possible. Size limitation, load factor may "
                    "start to be non-ideal.\n");
        Return = SUCCESS;
        goto EXIT;
    }

    double dwLoadFactor = LoadFactor(pHashTable);
    if (0 > dwLoadFactor)
    {
        DEBUG_PRINT("LoadFactor()");
        goto EXIT;
    }

    if (dwLoadFactor < LOAD_FACTOR_UPDATE)
    {
        Return = SUCCESS;
        goto EXIT;
    }

    pLinkedList = HashTabletoList(pHashTable);
    if (NULL == pLinkedList)
    {
        DEBUG_PRINT("HashTabletoList failed");
        return EXIT_FAILURE;
    }

    pHashTable->m_wSize = 0;

    // A hash table's size greatly impacts how often clusters form.
    // When the size is prime, clustering happens less.
    pHashTable->m_wCapacity = NextPrime(wCapacityHolder);
    if (0 == pHashTable->m_wCapacity)
    {
        DEBUG_PRINT("NextPrime()");
        goto CLEAN;
    }

    ZeroingHeapFree(GetProcessHeap(), NO_OPTION, (PVOID)&pHashTable->m_ppTable,
                             wCapacityHolder * sizeof(PLINKEDLIST));

    pHashTable->m_ppTable =
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                  pHashTable->m_wCapacity * sizeof(PLINKEDLIST));
    if (NULL == pHashTable->m_ppTable)
    {
        DEBUG_ERROR("HeapAlloc()");
        goto CLEAN;
    }

    for (WORD wCounter = 0; wCounter < pHashTable->m_wCapacity; wCounter++)
    {
        pHashTable->m_ppTable[wCounter] = NULL;
    }

    if (EXIT_FAILURE == HashTableListToTable(pHashTable, pLinkedList))
    {
        DEBUG_PRINT("HashTabletoList()");
        goto CLEAN2;
    }

    Return = SUCCESS;
    goto CLEAN;
CLEAN2:
    ZeroingHeapFree(GetProcessHeap(), NO_OPTION, (PVOID)&pHashTable->m_ppTable,
                             pHashTable->m_wCapacity * sizeof(PLINKEDLIST));
CLEAN:
    LinkedListDestroy(pLinkedList, NULL);
EXIT:
    return Return;
}

RETURNTYPE
HashTableNewEntry(PHASHTABLE pHashTable,
                  PVOID      pData,
                  PCHAR      pszKey,
                  WORD       wKeyLen)
{
    RETURNTYPE      Return    = SUCCESS;
    PHASHTABLEENTRY pNewEntry = NULL;

    if ((NULL == pHashTable) || (NULL == pData) || (NULL == pszKey))
    {
        DEBUG_PRINT("Input NULL");
        goto EXIT;
    }

    if (KEY_LENGTH < wKeyLen)
    {
        DEBUG_PRINT("wKeyLen too long");
        goto EXIT;
    }

    if (EXIT_FAILURE == HashTableReHash(pHashTable))
    {
        DEBUG_PRINT("HashTableReHash failed");
        goto EXIT;
    }

    pNewEntry =
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HASHTABLEENTRY));

    if (NULL == pNewEntry)
    {
        DEBUG_ERROR("HeapAlloc failed");
        goto EXIT;
    }

    if (FAILED(memcpy_s(pNewEntry->m_caKey, (KEY_LENGTH + 1), pszKey, wKeyLen)))
    {
        DEBUG_ERROR("wmemcpy_s failed");
        goto CLEAN;
    }
    pNewEntry->m_pfnFreeFunction = NULL;
    pNewEntry->m_wKeyLen         = wKeyLen;
    pNewEntry->m_pData           = pData;

    Return = HashTableNewEntryCalc(pHashTable, pNewEntry);
    if (EXIT_SUCCESS != Return)
    {
        if (EXIT_FAILURE == Return)
        {
            DEBUG_PRINT("HashTableNewEntryCalc()");
        }
        goto CLEAN;
    }

    Return = SUCCESS;
    goto EXIT;
CLEAN:
    ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pNewEntry,
                             sizeof(HASHTABLEENTRY));
EXIT:
    return Return;
}

PVOID
HashTableReturnEntry(PHASHTABLE pHashTable, PCHAR pszKey, WORD wKeyLen)
{
    DWORD           dwHash       = 0;
    DWORD           dwEntryIndex = 0;
    PLINKEDLIST     pTempList    = NULL;
    WORD            iTempSize    = 0;
    WORD            wCounter     = 0;
    PHASHTABLEENTRY pTempEntry   = NULL;
    PVOID           pData        = NULL;

    if ((NULL == pHashTable) || (NULL == pszKey) ||
        (NULL == pHashTable->m_pfnHashFunction))
    {
        DEBUG_PRINT("Input NULL");
        goto EXIT;
    }

    if (KEY_LENGTH < wKeyLen)
    {
        DEBUG_PRINT("wKeyLen too long");
        goto EXIT;
    }

    dwHash = pHashTable->m_pfnHashFunction(pszKey);

    dwEntryIndex = dwHash % (DWORD)pHashTable->m_wCapacity;

    if (NULL == pHashTable->m_ppTable[dwEntryIndex])
    {
        DEBUG_PRINT("entry doesn't exist");
        goto EXIT;
    }
    else
    {
        pTempList = pHashTable->m_ppTable[dwEntryIndex];

        iTempSize = pTempList->m_wSize;

        for (wCounter = 0; wCounter < iTempSize; wCounter++)
        {
            pTempEntry = LinkedListReturn(pTempList, wCounter);
            if (EXIT_SUCCESS ==
                CompareMemory(pTempEntry->m_caKey, pszKey, wKeyLen))
            {
                pData = pTempEntry->m_pData;
                goto EXIT;
            }
        }
    }

    DEBUG_PRINT("entry doesn't exist");
EXIT:
    return pData;
}

PVOID
HashTableDestroyEntry(PHASHTABLE pHashTable, PCHAR pszKey, WORD wKeyLen)
{
    DWORD           dwHash       = 0;
    DWORD           dwEntryIndex = 0;
    PLINKEDLIST     pTempList    = NULL;
    WORD            iTempSize    = 0;
    WORD            wCounter     = 0;
    PHASHTABLEENTRY pTempEntry   = NULL;
    PVOID           pData        = NULL;

    if ((NULL == pHashTable) || (NULL == pszKey) ||
        (NULL == pHashTable->m_pfnHashFunction))
    {
        DEBUG_PRINT("Input NULL");
        goto EXIT;
    }

    if (KEY_LENGTH < wKeyLen)
    {
        DEBUG_PRINT("wKeyLen too long");
        goto EXIT;
    }

    dwHash = pHashTable->m_pfnHashFunction(pszKey);

    dwEntryIndex = dwHash % (DWORD)pHashTable->m_wCapacity;

    if (NULL == pHashTable->m_ppTable[dwEntryIndex])
    {
        DEBUG_PRINT("entry not found, key invalid");
        goto EXIT;
    }
    else
    {
        pTempList = pHashTable->m_ppTable[dwEntryIndex];

        iTempSize = pTempList->m_wSize;

        for (wCounter = 0; wCounter < iTempSize; wCounter++)
        {
            pTempEntry = LinkedListReturn(pTempList, wCounter);

            if (SUCCESS == CompareMemory(pTempEntry->m_caKey, pszKey, wKeyLen))
            {
                if (NULL == LinkedListRemove(pTempList, wCounter, NULL))
                {
                    DEBUG_PRINT("LinkedListRemove()");
                    goto EXIT;
                }

                if (0 == pTempList->m_wSize)
                {
                    if (EXIT_FAILURE == LinkedListDestroy(pTempList, NULL))
                    {
                        DEBUG_PRINT("LinkedListDestroy failed");
                        goto EXIT;
                    }

                    pHashTable->m_ppTable[dwEntryIndex] = NULL;
                }

                pData = pTempEntry->m_pData;
                ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pTempEntry,
                                sizeof(HASHTABLEENTRY));

                pHashTable->m_wSize -= 1;
                goto EXIT;
            }
        }
    }

EXIT:
    return pData;
}

static void HashTableFreeHelper(PVOID pData)
{
    PHASHTABLEENTRY pTempEntry = (PHASHTABLEENTRY)pData;
    if (NULL == pTempEntry)
    {
        DEBUG_PRINT("HashTableFreeHelper failed");
        return;
    }

    if (NULL == pTempEntry->m_pfnFreeFunction)
    {
        ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pTempEntry,
                        sizeof(HASHTABLEENTRY));
    }
    else
    {
        pTempEntry->m_pfnFreeFunction(pTempEntry->m_pData);
        ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pTempEntry,
                        sizeof(HASHTABLEENTRY));
    }
}

RETURNTYPE
HashTableDestroy(PHASHTABLE pHashTable, VOID (*pfnFreeFunction)(PVOID))
{
    RETURNTYPE      Return     = ERR_GENERIC;
    WORD            wCounter   = 0;
    WORD            wCounter2  = 0;
    PHASHTABLEENTRY pTempEntry = NULL;

    if ((NULL == pHashTable) || (NULL == pHashTable->m_ppTable))
    {
        DEBUG_PRINT("Input NULL");
        goto EXIT;
    }

    for (wCounter = 0; wCounter < pHashTable->m_wCapacity; wCounter++)
    {
        if (NULL != pHashTable->m_ppTable[wCounter])
        {
            for (wCounter2 = 0;
                 wCounter2 < pHashTable->m_ppTable[wCounter]->m_wSize;
                 wCounter2++)
            {
                pTempEntry = LinkedListReturn(pHashTable->m_ppTable[wCounter],
                                              wCounter2);
                pTempEntry->m_pfnFreeFunction = pfnFreeFunction;
            }

            if (SUCCESS != LinkedListDestroy(pHashTable->m_ppTable[wCounter],
                                             &HashTableFreeHelper))
            {
                DEBUG_PRINT("LinkedListDestroy()");
                goto CLEAN;
            }
        }
    }

    Return = SUCCESS;
CLEAN:
    ZeroingHeapFree(GetProcessHeap(), NO_OPTION, (PVOID)&pHashTable->m_ppTable,
                             pHashTable->m_wCapacity * sizeof(PLINKEDLIST));

    ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pHashTable,
                             sizeof(HASHTABLE));
EXIT:
    return Return;
}

// End of file
