/*****************************************************************//**
 * \file   HashTable.c
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#include "pch.h"
#include "HashTable.h"

static DWORD
ModularExponentiation(DWORD dwBase, DWORD dwExponent, DWORD dwModulus)
{
    DWORD dwResult = 1;
    dwBase = dwBase % dwModulus;

    while (dwExponent > 0)
    {
        if (dwExponent % 2 == 1)
        {
            dwResult = (dwResult * dwBase) % dwModulus;
        }

        dwBase = (dwBase * dwBase) % dwModulus;
        dwExponent = dwExponent >> 1;
    }

    return dwResult;
}

BOOL
IsPrime(WORD wValue)
{
    if (wValue <= 1)
    {
        return FALSE;
    }
    else if (wValue <= 3)
    {
        return TRUE;
    }

    //NOTE: Factoring out powers of two.
    DWORD dwOddInt = wValue - 1;
    DWORD dwPowerOfTwo = 0;

    while (dwOddInt % 2 == 0)
    {
        dwPowerOfTwo += 1;
        dwOddInt = dwOddInt / 2;
    }

    //NOTE: Seeding inside the loop would generate the same number each time
    //due to the computational speed.
    srand((DWORD)time(NULL));

    DWORD dwMaxNum = wValue - 2;
    DWORD dwMinNum = 2;
    DWORD dwRoundCount = 5;

    for (DWORD dwCounterOne = 0; dwCounterOne < dwRoundCount; dwCounterOne++)
    {
        //NOTE: Getting the number within range will decrease the randomness
        //slightly our base for prime calculations will be determined.
        DWORD dwPrimeBase = rand() % (dwMaxNum + 1 - dwMinNum) + dwMinNum;
        DWORD dwCalcOne = ModularExponentiation(dwPrimeBase, dwOddInt, wValue);
        DWORD dwCalcTwo = 0;

        //NOTE: Test to see if strong base, will skip to next loop if not.
#pragma warning(push)
#pragma warning(disable : 4389)
        //NOTE: wValue can never be below four at this point in the code:
        //compiler warnings can safely be ignored here.
        if ((1 == dwCalcOne) || ((wValue - 1) == dwCalcOne))
#pragma warning(pop)
            
        {
            continue;
        }

        for (DWORD dwCounterTwo = 0; dwCounterTwo < dwPowerOfTwo; dwCounterTwo++)
        {
            dwCalcTwo = ModularExponentiation(dwCalcOne, 2, wValue);
#pragma warning(push)
#pragma warning(disable : 4389)
            //NOTE: wValue can never be below four at this point in the code:
            //compiler warnings can safely be ignored here.
            if ((1 == dwCalcTwo) && (1 != dwCalcOne) &&
                (dwCalcOne != (wValue - 1)))
#pragma warning(pop)
            {
                return FALSE;
            }

            dwCalcOne = dwCalcTwo;
        }

        if (1 != dwCalcTwo)
        {
            return FALSE;
        }
    }

    return TRUE;
}

WORD
NextPrime(WORD wValue)
{
    if (wValue <= 1)
    {
        return 2;
    }

    if (wValue == 2)
    {
        return 3;
    }

    //NOTE: The search will start at the next odd number.
    if (wValue % 2 == 0)
    {
        wValue += 1;
    }
    else
    {
        wValue += 2;
    }

    WORD wDoubleValue = wValue * 2;

    if (wValue > 32767)
    {
        wDoubleValue = 65535;
    }

    //NOTE: Bertrand's postulate: for any int  > 3 there exists at least one
    //prime number, p, such that n < p < 2n.
    for (WORD wLoopValue = wValue; wLoopValue < wDoubleValue; wLoopValue += 2)
    {
        if (IsPrime(wLoopValue))
        {
            return wLoopValue;
        }
    }

    return WIN_EXIT_FAILURE;
}

//NOTE: Return EXIT_SUCCESS if equal, EXIT_FAILURE if not.
//WARNING: pszKEy2 is allocated by user. If pszKeyLen is longer, accessing 
//un-allocated memory.
static INT
CompareMemory(PWCHAR pszKey1, PWCHAR pszKey2, WORD wKeyLen)
{
    for (INT iCounter = 0; iCounter < wKeyLen; iCounter++)
    {
        if (pszKey1[iCounter] != pszKey2[iCounter])
        {
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

static INT64
DefaultHashFunction(PVOID pKey)
{
    if (NULL == pKey)
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "NULL input");
        return EXIT_FAILURE_NEGATIVE;
    }

    PWCHAR pcaKey = (PWCHAR)pKey;

    const DWORD dwFNVOffset = 2166136261;
    const DWORD dwFNVPrime = 16777619;
    DWORD dwHash = dwFNVOffset;

    for (WORD wCounter = 0; wCounter <= KEY_LENGTH; wCounter++)
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

PHASHTABLE
HashTableInit(WORD wCapacity, INT64(*pfnHashFunction)(PVOID))
{
    PHASHTABLE pHashTable = HeapAlloc(GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        sizeof(HASHTABLE));

    if (NULL == pHashTable)
    {
        PrintError((PCHAR)__func__, __LINE__);
        return NULL;
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

    pHashTable->m_ppTable = HeapAlloc(GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        pHashTable->m_wCapacity * sizeof(LINKEDLIST));

    if (NULL == pHashTable->m_ppTable)
    {
        PrintError((PCHAR)__func__, __LINE__);
        return NULL;
    }

    return pHashTable;
}

static WORD
DuplicateDataCheck(PLINKEDLIST pLinkedList, PWCHAR pszKey)
{
    if ((NULL == pLinkedList) || (NULL == pszKey))
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "input NULL");
        return EXIT_FAILURE;
    }

    for (WORD wCounter = 0; wCounter < pLinkedList->m_wSize; wCounter++)
    {
        PHASHTABLEENTRY pTempEntry = LinkedListReturn(pLinkedList, wCounter,
            DONT_DESTROY_NODE);
        if (NULL == pTempEntry)
        {
            PrintErrorCustom((PCHAR)__func__, __LINE__, "LinkedListReturn()");
            return EXIT_FAILURE;
        }

        if (EXIT_SUCCESS == CompareMemory(pTempEntry->m_caKey, pszKey,
            pTempEntry->m_wKeyLen))
        {
            return DUPLICATE_KEY;
        }
    }

    return EXIT_SUCCESS;
}

static WORD
HashTableNewEntryCalc(PHASHTABLE pHashTable, PHASHTABLEENTRY pNewEntry)
{
    if (((NULL == pHashTable) || (NULL == pHashTable->m_pfnHashFunction)
        || (NULL == pNewEntry)))
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "input NULL");
        return EXIT_FAILURE;
    }

    INT64 lHash = pHashTable->m_pfnHashFunction(pNewEntry->m_caKey);

    if (EXIT_FAILURE_NEGATIVE == lHash)
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "m_pfnHashFunction()");
        return EXIT_FAILURE;
    }

    DWORD dwHash = (DWORD)lHash; //See warning in hash_table.h

    DWORD dwEntryIndex = dwHash % (DWORD)pHashTable->m_wCapacity;

    if (NULL == pHashTable->m_ppTable[dwEntryIndex])
    {
        PLINKEDLIST pTempList = LinkedListInit();

        if (NULL == pTempList)
        {
            PrintErrorCustom((PCHAR)__func__, __LINE__, "LinkedListInit()");
            return EXIT_FAILURE;
        }

        if (EXIT_FAILURE == LinkedListInsert(pTempList, pNewEntry, 0))
        {
            PrintErrorCustom((PCHAR)__func__, __LINE__, "LinkedListInsert()");
            return EXIT_FAILURE;
        }

        pHashTable->m_ppTable[dwEntryIndex] = pTempList;
    }
    else
    {
        PLINKEDLIST pTempList = pHashTable->m_ppTable[dwEntryIndex];

        WORD wResult = DuplicateDataCheck(pTempList, pNewEntry->m_caKey);
        if (EXIT_SUCCESS != wResult)
        {
            if (EXIT_FAILURE == wResult)
            {
                PrintErrorCustom((PCHAR)__func__, __LINE__,
                    "DuplicateDataCheck()");
            }
            return wResult;
        }
    }

    pHashTable->m_wSize++;

    return EXIT_SUCCESS;
}

static double
LoadFactor(PHASHTABLE pHashTable)
{
    if (NULL == pHashTable)
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "input NULL");
        return EXIT_FAILURE_NEGATIVE;
    }

    if (0.0 == (double)pHashTable->m_wCapacity)
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__,
            "Stop trying to divide by zero");
        return EXIT_FAILURE_NEGATIVE;
    }

    double dwLoadFactor = ((double)pHashTable->m_wSize) /
        ((double)pHashTable->m_wCapacity);

    if (dwLoadFactor < 0)
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__,
            "load factor is out of bounds");
        return EXIT_FAILURE_NEGATIVE;
    }

    return dwLoadFactor;
}

static PLINKEDLIST
HashTabletoList(PHASHTABLE pHashTable)
{
    if (NULL == pHashTable)
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "input NULL");
        return NULL;
    }

    PLINKEDLIST pHashTableList = LinkedListInit();

    if (NULL == pHashTableList)
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "LinkedListInit()");
        return NULL;
    }

    for (WORD wCounter = 0; wCounter < pHashTable->m_wCapacity; wCounter++)
    {
        if (NULL != pHashTable->m_ppTable[wCounter])
        {
            PLINKEDLIST pTempList = pHashTable->m_ppTable[wCounter];
            WORD iTempSize = pTempList->m_wSize;

            for (WORD wCounter2 = 0; wCounter2 < iTempSize; wCounter2++)
            {
                PHASHTABLEENTRY pTempEntry = LinkedListReturn(pTempList,
                    wCounter2, DONT_DESTROY_NODE);

                if (NULL == pTempEntry)
                {
                    PrintErrorCustom((PCHAR)__func__, __LINE__,
                        "LinkedListReturn()");
                    LinkedListDestroy(pHashTableList, NULL);
                    return NULL;
                }

                if (EXIT_FAILURE == LinkedListInsert(pHashTableList,
                    pTempEntry, pHashTableList->m_wSize))
                {
                    PrintErrorCustom((PCHAR)__func__, __LINE__,
                        "LinkedListInsert()");
                    LinkedListDestroy(pHashTableList, NULL);
                    return NULL;
                }
            }

            if (EXIT_FAILURE == LinkedListDestroy(pTempList, NULL))
            {
                PrintErrorCustom((PCHAR)__func__, __LINE__,
                    "LinkedListDestroy()");
                LinkedListDestroy(pHashTableList, NULL);
                return NULL;
            }

            pHashTable->m_ppTable[wCounter] = NULL;
        }
    }

    return pHashTableList;
}

static WORD
HashTableListToTable(PHASHTABLE pHashTable, PLINKEDLIST pLinkedList)
{
    if ((NULL == pHashTable) || (NULL == pLinkedList))
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "input NULL");
        return EXIT_FAILURE;
    }

    WORD iTempSize = pLinkedList->m_wSize;

    for (WORD wCounter = 0; wCounter < iTempSize; wCounter++)
    {
        PHASHTABLEENTRY pTempEntry = LinkedListReturn(pLinkedList, wCounter,
            DONT_DESTROY_NODE);

        if (NULL == pTempEntry)
        {
            PrintErrorCustom((PCHAR)__func__, __LINE__, "LinkedListReturn()");
            return EXIT_FAILURE;
        }

        WORD wResult = HashTableNewEntryCalc(pHashTable, pTempEntry);
        if (EXIT_SUCCESS != wResult)
        {
            if (EXIT_FAILURE != wResult)
            {
                PrintErrorCustom((PCHAR)__func__, __LINE__,
                    "HashTableNewEntryCalc()");
            }
            return wResult;
        }
    }

    return EXIT_SUCCESS;
}

static WORD
HashTableReHash(PHASHTABLE pHashTable)
{
    if (NULL == pHashTable)
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "input NULL");
        return EXIT_FAILURE;
    }

    if (pHashTable->m_wCapacity >= MAX_PRIME_IN_RANGE)
    {
        printf("Re-hashing not possible. Size limitation, load factor may "
            "start to be non-ideal.\n");
        return EXIT_SUCCESS;
    }

    double dwLoadFactor = LoadFactor(pHashTable);

    if (EXIT_FAILURE_NEGATIVE == dwLoadFactor)
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "LoadFactor");
        return EXIT_FAILURE;
    }

    if (dwLoadFactor < LOAD_FACTOR_UPDATE)
    {
        return EXIT_SUCCESS;
    }

    PLINKEDLIST pLinkedList = HashTabletoList(pHashTable);

    if (NULL == pLinkedList)
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "HashTabletoList");
        return EXIT_FAILURE;
    }

    pHashTable->m_wSize = 0;
    WORD wCapacityHolder = pHashTable->m_wCapacity;

    //A hash table's size greatly impacts how often clusters form.
    //When the size is prime, clustering happens less.
    pHashTable->m_wCapacity = NextPrime(wCapacityHolder);

    if (WIN_EXIT_FAILURE == pHashTable->m_wCapacity)
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "NextPrime");
        LinkedListDestroy(pLinkedList, NULL);
        return EXIT_FAILURE;
    }

    if (WIN_EXIT_FAILURE == HeapFree(GetProcessHeap(),
        NO_OPTION, pHashTable->m_ppTable))
    {
        PrintError((PCHAR)__func__, __LINE__);
        LinkedListDestroy(pLinkedList, NULL);
        return EXIT_FAILURE;
    }

    pHashTable->m_ppTable = HeapAlloc(GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        pHashTable->m_wCapacity * sizeof(LINKEDLIST));

    if (NULL == pHashTable->m_ppTable)
    {
        PrintError((PCHAR)__func__, __LINE__);
        LinkedListDestroy(pLinkedList, NULL);
        return EXIT_FAILURE;
    }

    for (WORD wCounter = 0; wCounter < pHashTable->m_wCapacity; wCounter++)
    {
        pHashTable->m_ppTable[wCounter] = NULL;
    }

    if (EXIT_FAILURE == HashTableListToTable(pHashTable, pLinkedList))
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "HashTabletoList");
        LinkedListDestroy(pLinkedList, NULL);
        return EXIT_FAILURE;
    }

    if (EXIT_FAILURE == LinkedListDestroy(pLinkedList, NULL))
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "LinkedListDestroy");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

WORD
HashTableNewEntry(PHASHTABLE pHashTable, PVOID pData, PWCHAR pszKey,
    WORD wKeyLen)
{
    if ((NULL == pHashTable) || (NULL == pData) || (NULL == pszKey))
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "input NULL");
        return EXIT_FAILURE;
    }

    if (KEY_LENGTH < wKeyLen)
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "wKeyLen too long");
        return EXIT_FAILURE;
    }

    if (EXIT_FAILURE == HashTableReHash(pHashTable))
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "HashTableReHash");
        return EXIT_FAILURE;
    }

    PHASHTABLEENTRY pNewEntry = HeapAlloc(GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        sizeof(HASHTABLEENTRY));

    if (NULL == pNewEntry)
    {
        PrintError((PCHAR)__func__, __LINE__);
        return EXIT_FAILURE;
    }

    if (FAILED(wmemcpy_s(pNewEntry->m_caKey, KEY_LENGTH, pszKey, wKeyLen)))
    {
        PrintError((PCHAR)__func__, __LINE__);
        HeapFree(GetProcessHeap(), NO_OPTION, pNewEntry);
        return EXIT_FAILURE;
    }
    pNewEntry->m_pfnFreeFunction = NULL;
    pNewEntry->m_wKeyLen = wKeyLen;
    pNewEntry->m_pData = pData;

    WORD wResult = HashTableNewEntryCalc(pHashTable, pNewEntry);
    if (EXIT_SUCCESS != wResult)
    {
        if (EXIT_FAILURE == wResult)
        {
            PrintErrorCustom((PCHAR)__func__, __LINE__,
                "HashTableNewEntryCalc()");
        }
        HeapFree(GetProcessHeap(), NO_OPTION, pNewEntry);
        return wResult;
    }

    return EXIT_SUCCESS;
}

PVOID
HashTableReturnEntry(PHASHTABLE pHashTable, PWCHAR pszKey, WORD wKeyLen)
{
    if ((NULL == pHashTable) || (NULL == pszKey) ||
        (NULL == pHashTable->m_pfnHashFunction))
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "input NULL");
        return NULL;
    }

    if (KEY_LENGTH < wKeyLen)
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "wKeyLen too long");
        return NULL;
    }

    INT64 lHash = pHashTable->m_pfnHashFunction(pszKey);

    if (EXIT_FAILURE_NEGATIVE == lHash)
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "m_pfnHashFunction()");
        return NULL;
    }

    DWORD dwHash = (DWORD)lHash; //See warning in hash_table.h

    DWORD dwEntryIndex = dwHash % (DWORD)pHashTable->m_wCapacity;

    if (NULL == pHashTable->m_ppTable[dwEntryIndex])
    {
        return NULL;
    }
    else
    {
        PLINKEDLIST pTempList = pHashTable->m_ppTable[dwEntryIndex];

        WORD iTempSize = pTempList->m_wSize;

        for (WORD wCounter = 0; wCounter < iTempSize; wCounter++)
        {
            PHASHTABLEENTRY pTempEntry = LinkedListReturn(pTempList, wCounter,
                DONT_DESTROY_NODE);
             //TODO: Ensure data is NULL terminated to compare, use memcmp()
            if (EXIT_SUCCESS == CompareMemory(pTempEntry->m_caKey, pszKey,
                wKeyLen))
            {
                return pTempEntry->m_pData;
            }
        }
    }

    pHashTable->m_wSize++;

    return NULL;
}

PVOID
HashTableDestroyEntry(PHASHTABLE pHashTable, PWCHAR pszKey, WORD wKeyLen)
{
    if ((NULL == pHashTable) || (NULL == pszKey) ||
        (NULL == pHashTable->m_pfnHashFunction))
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "input NULL");
        return NULL;
    }

    if (KEY_LENGTH < wKeyLen)
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "wKeyLen too long");
        return NULL;
    }

    INT64 lHash = pHashTable->m_pfnHashFunction(pszKey);

    if (EXIT_FAILURE_NEGATIVE == lHash)
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "m_pfnHashFunction()");
        return NULL;
    }

    DWORD dwHash = (DWORD)lHash; //See warning in hash_table.h

    DWORD dwEntryIndex = dwHash % (DWORD)pHashTable->m_wCapacity;

    if (NULL == pHashTable->m_ppTable[dwEntryIndex])
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__,
            "entry not found, key invalid");
        return NULL;
    }
    else
    {
        PLINKEDLIST pTempList = pHashTable->m_ppTable[dwEntryIndex];

        WORD iTempSize = pTempList->m_wSize;

        for (WORD wCounter = 0; wCounter < iTempSize; wCounter++)
        {
            PHASHTABLEENTRY pTempEntry = LinkedListReturn(pTempList, wCounter,
                DONT_DESTROY_NODE);

            if (EXIT_SUCCESS == CompareMemory(pTempEntry->m_caKey, pszKey,
                wKeyLen))
            {
                if (EXIT_FAILURE == LinkedListRemove(pTempList, wCounter,
                    NULL))
                {
                    PrintErrorCustom((PCHAR)__func__, __LINE__,
                        "LinkedListRemove");
                    return NULL;
                }

                if (0 == pTempList->m_wSize)
                {
                    if (EXIT_FAILURE == LinkedListDestroy(pTempList, NULL))
                    {
                        PrintErrorCustom((PCHAR)__func__, __LINE__,
                            "LinkedListDestroy");
                        return NULL;
                    }

                    pHashTable->m_ppTable[dwEntryIndex] = NULL;
                }

                PVOID pData = pTempEntry->m_pData;

                if (WIN_EXIT_FAILURE == HeapFree(GetProcessHeap(),
                    NO_OPTION, pTempEntry))
                {
                    return NULL;
                }

                pHashTable->m_wSize -= 1;

                return pData;
            }
        }
    }

    return NULL;
}

static void
HashTableFreeHelper(PVOID pData)
{
    PHASHTABLEENTRY pTempEntry = (PHASHTABLEENTRY)pData;

    if (NULL == pTempEntry)
    {
        PrintErrorCustom((PCHAR)__func__, __LINE__, "HashTableFreeHelper()");
        return;
    }

    if (NULL == pTempEntry->m_pfnFreeFunction)
    {
        HeapFree(GetProcessHeap(), NO_OPTION, pTempEntry);
    }
    else
    {
        pTempEntry->m_pfnFreeFunction(pTempEntry->m_pData);
        HeapFree(GetProcessHeap(), NO_OPTION, pTempEntry);
    }
}

WORD
HashTableDestroy(PHASHTABLE pHashTable, VOID (*pfnFreeFunction)(PVOID))
{
    if (NULL == pHashTable)
    {
        PrintError((PCHAR)__func__, __LINE__);
        return EXIT_FAILURE;
    }

    if (NULL == pHashTable->m_ppTable)
    {
        PrintError((PCHAR)__func__, __LINE__);
        return EXIT_FAILURE;
    }

    for (WORD wCounter = 0; wCounter < pHashTable->m_wCapacity; wCounter++)
    {
        if (NULL != pHashTable->m_ppTable[wCounter])
        {
            for (WORD wCounter2 = 0; wCounter2 <
                pHashTable->m_ppTable[wCounter]->m_wSize; wCounter2++)
            {
                PHASHTABLEENTRY pTempEntry = LinkedListReturn(
                    pHashTable->m_ppTable[wCounter], wCounter2,
                    DONT_DESTROY_NODE);
                pTempEntry->m_pfnFreeFunction = pfnFreeFunction;
            }

            if (EXIT_FAILURE == LinkedListDestroy(
                pHashTable->m_ppTable[wCounter], &HashTableFreeHelper))
            {
                PrintErrorCustom((PCHAR)__func__, __LINE__,
                    "LinkedListDestroy()");
                return EXIT_FAILURE;
            }
        }
    }

    if (WIN_EXIT_FAILURE == HeapFree(GetProcessHeap(),
        NO_OPTION, pHashTable->m_ppTable))
    {
        PrintError((PCHAR)__func__, __LINE__);
        HeapFree(GetProcessHeap(), NO_OPTION, pHashTable->m_ppTable);
        return EXIT_FAILURE;
    }

    if (WIN_EXIT_FAILURE == HeapFree(GetProcessHeap(),
        NO_OPTION, pHashTable))
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}



 //End of file
