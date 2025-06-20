#include <Windows.h>
#include <stdio.h>

#include "linkedlist.h"

static PLINKEDLISTNODE CreateNode()
{
    PLINKEDLISTNODE pLinkedListNode =
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LINKEDLISTNODE));

    if (NULL == pLinkedListNode)
    {
        DEBUG_ERROR("Failed to create node");
    }

    return pLinkedListNode;
}

static RETURNTYPE DestroyNode(PLINKEDLISTNODE pLinkedListNode)
{
    RETURNTYPE Return = ERR_GENERIC;

    if (NULL == pLinkedListNode)
    {
        DEBUG_PRINT("NULL input");
        goto EXIT;
    }

    ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pLinkedListNode,
                    sizeof(LINKEDLISTNODE));

    Return = SUCCESS;
EXIT:
    return Return;
}

RETURNTYPE
LinkedListInit(PPLINKEDLIST ppLinkedList)
{
    RETURNTYPE  Return = ERR_GENERIC;
    PLINKEDLIST pLinkedList =
        HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LINKEDLIST));

    if (NULL == pLinkedList)
    {
        DEBUG_ERROR("Failed to initialize list");
        Return = ERR_GENERIC;
        goto EXIT;
    }

    pLinkedList->m_pHead = NULL;
    pLinkedList->m_pTail = NULL;
    pLinkedList->m_wSize = 0;
    *ppLinkedList        = pLinkedList;
    Return               = SUCCESS;
EXIT:
    return Return;
}

static RETURNTYPE LinkedListInsertFirst(PLINKEDLIST     pLinkedList,
                                        PLINKEDLISTNODE pLinkedListNode)
{
    pLinkedListNode->m_pNext = pLinkedListNode;
    pLinkedList->m_pHead     = pLinkedListNode;
    pLinkedList->m_pTail     = pLinkedListNode;
    pLinkedList->m_wSize     = 1;

    return SUCCESS;
}

static RETURNTYPE LinkedListInsertBegin(PLINKEDLIST     pLinkedList,
                                        PLINKEDLISTNODE pLinkedListNode)
{
    pLinkedListNode->m_pNext      = pLinkedList->m_pHead;
    pLinkedList->m_pHead          = pLinkedListNode;
    pLinkedList->m_pTail->m_pNext = pLinkedListNode;
    pLinkedList->m_wSize += 1;

    return SUCCESS;
}

static RETURNTYPE LinkedListInsertEnd(PLINKEDLIST     pLinkedList,
                                      PLINKEDLISTNODE pLinkedListNode)
{
    pLinkedListNode->m_pNext      = pLinkedList->m_pHead;
    pLinkedList->m_pTail->m_pNext = pLinkedListNode;
    pLinkedList->m_pTail          = pLinkedListNode;
    pLinkedList->m_wSize += 1;

    return SUCCESS;
}

static RETURNTYPE LinkedListInsertAtIndex(PLINKEDLIST     pLinkedList,
                                          PLINKEDLISTNODE pLinkedListNode,
                                          WORD            wIndex)
{
    RETURNTYPE Return = ERR_GENERIC;
    // See explanation below for selection of pTail herev
    PLINKEDLISTNODE pTempNode    = pLinkedList->m_pTail;
    WORD            wTargetIndex = wIndex;
    wIndex                       = 0;

    do
    {
        pTempNode = pTempNode->m_pNext;
        wIndex += 1;

        if (NULL == pTempNode->m_pNext)
        {
            DEBUG_PRINT("NULL node");
            goto EXIT;
        }
    } while (wIndex != wTargetIndex);

    // Now the node is placed before our target node index: i.e. if we are
    // trying to place a node at index 1, we'll have the list index 0 node.
    pLinkedListNode->m_pNext = pTempNode->m_pNext->m_pNext;
    pTempNode->m_pNext       = pLinkedListNode;
    pLinkedList->m_wSize += 1;

    Return = SUCCESS;
EXIT:
    return Return;
}

RETURNTYPE
LinkedListInsert(PLINKEDLIST pLinkedList, PVOID pData, WORD wIndex)
{
    RETURNTYPE      Return          = ERR_GENERIC;
    PLINKEDLISTNODE pLinkedListNode = NULL;

    if (NULL == pLinkedList || NULL == pData)
    {
        DEBUG_PRINT("NULL input");
        goto EXIT;
    }

    if (wIndex > pLinkedList->m_wSize)
    {
        DEBUG_PRINT("List index out of range");
        goto EXIT;
    }

    pLinkedListNode = CreateNode();
    if (NULL == pLinkedListNode)
    {
        DEBUG_PRINT("CreateNode failed");
        goto EXIT;
    }

    pLinkedListNode->m_pData = pData;

    // Handle case where the size is 0
    if (NULL == pLinkedList->m_pHead)
    {
        // If the head is NULL but the size isn't 0, let's return error
        if (pLinkedList->m_wSize != 0)
        {
            DEBUG_PRINT("List head NULL");
            goto EXIT;
        }

        Return = LinkedListInsertFirst(pLinkedList, pLinkedListNode);
        goto EXIT;
    }

    // Tail should not be NULL either
    if (NULL == pLinkedList->m_pTail)
    {
        DEBUG_PRINT("List tail NULL");
        goto EXIT;
    }

    if (0 == wIndex)
    {
        Return = LinkedListInsertBegin(pLinkedList, pLinkedListNode);
        goto EXIT;
    }
    else if (wIndex == pLinkedList->m_wSize)
    {
        Return = LinkedListInsertEnd(pLinkedList, pLinkedListNode);
        goto EXIT;
    }
    else
    {
        Return = LinkedListInsertAtIndex(pLinkedList, pLinkedListNode, wIndex);
        goto EXIT;
    }

    Return = SUCCESS;
EXIT:
    return Return;
}

static PVOID LinkedListRemoveLast(PLINKEDLIST pLinkedList)
{
    PLINKEDLISTNODE pTempNode = pLinkedList->m_pHead;
    pLinkedList->m_pHead      = NULL;
    pLinkedList->m_pTail      = NULL;
    pLinkedList->m_wSize      = 0;

    PVOID pData = pTempNode->m_pData;

    if (EXIT_FAILURE == DestroyNode(pTempNode))
    {
        DEBUG_PRINT("DestroyNode failed");
        return NULL;
    }

    return pData;
}

static PVOID LinkedListRemoveBegin(PLINKEDLIST pLinkedList)
{
    PLINKEDLISTNODE pTempNode     = pLinkedList->m_pHead;
    pLinkedList->m_pHead          = pTempNode->m_pNext;
    pLinkedList->m_pTail->m_pNext = pLinkedList->m_pHead;
    pLinkedList->m_wSize -= 1;

    PVOID pData = pTempNode->m_pData;

    if (EXIT_FAILURE == DestroyNode(pTempNode))
    {
        DEBUG_PRINT("DestroyNode failed");
        return NULL;
    }

    return pData;
}

static PVOID LinkedListRemoveAtIndex(PLINKEDLIST pLinkedList, WORD wIndex)
{
    PLINKEDLISTNODE pTempNode   = pLinkedList->m_pTail;
    PLINKEDLISTNODE pDeleteNode = NULL;
    PVOID           pData       = NULL;

    WORD wTargetIndex = wIndex;
    wIndex            = 0;

    do
    {
        pTempNode = pTempNode->m_pNext;
        wIndex += 1;

        if (NULL == pTempNode->m_pNext)
        {
            DEBUG_PRINT("NULL node");
            goto EXIT;
        }
    } while (wIndex != wTargetIndex);

    // Now the node is placed before our target node index: i.e. if we are
    // trying to place a node at index 1, we'll have the list index 0 node.
    pDeleteNode        = pTempNode->m_pNext;
    pTempNode->m_pNext = pDeleteNode->m_pNext;
    if (wTargetIndex == (pLinkedList->m_wSize - 1))
    {
        pLinkedList->m_pTail = pTempNode;
    }
    pLinkedList->m_wSize -= 1;

    pData = pDeleteNode->m_pData;

    if (SUCCESS != DestroyNode(pDeleteNode))
    {
        DEBUG_PRINT("DestroyNode failed");
        pData = NULL;
        goto EXIT;
    }

EXIT:
    return pData;
}

PVOID
LinkedListRemove(PLINKEDLIST pLinkedList,
                 WORD        wIndex,
                 VOID (*pfnFreeFunction)(PVOID))
{
    DEBUG_PRINT("remove called!");
    PVOID pData = NULL;
    if ((NULL == pLinkedList) || (NULL == pLinkedList->m_pHead) ||
        (NULL == pLinkedList->m_pTail))
    {
        DEBUG_PRINT("List/List head/List tail NULL");
        goto EXIT;
    }

    if (wIndex > (pLinkedList->m_wSize - 1))
    {
        DEBUG_PRINT("List index out of range");
        goto EXIT;
    }

    if (1 == pLinkedList->m_wSize)
    {
        pData = LinkedListRemoveLast(pLinkedList);
    }
    else if (0 == wIndex)
    {
        pData = LinkedListRemoveBegin(pLinkedList);
    }
    else
    {
        pData = LinkedListRemoveAtIndex(pLinkedList, wIndex);
    }

    if (NULL == pData)
    {
        DEBUG_PRINT("Data was NULL");
        goto EXIT;
    }

    if (NULL != pfnFreeFunction)
    {
        pfnFreeFunction(pData);
    }

EXIT:
    DEBUG_PRINT("remove finished!");
    return pData;
}

static PVOID LinkedListReturnLast(PLINKEDLIST pLinkedList)
{
    return pLinkedList->m_pHead->m_pData;
}

static PVOID LinkedListReturnBegin(PLINKEDLIST pLinkedList)
{
    return pLinkedList->m_pHead->m_pData;
}

static PVOID LinkedListReturnAtIndex(PLINKEDLIST pLinkedList, WORD wIndex)
{
    PLINKEDLISTNODE pTempNode = pLinkedList->m_pTail;

    WORD wTargetIndex = wIndex;
    wIndex            = 0;

    do
    {
        pTempNode = pTempNode->m_pNext;
        wIndex += 1;

        if (NULL == pTempNode->m_pNext)
        {
            DEBUG_PRINT("NULL node");
            return NULL;
        }
    } while (wIndex != wTargetIndex);

    // Now the node is placed before our target node index: i.e. if we are
    // trying to place a node at index 1, we'll have the list index 0 node.

    return pTempNode->m_pNext->m_pData;
}

PVOID
LinkedListReturn(PLINKEDLIST pLinkedList, WORD wIndex)
{
    PVOID pData = NULL;

    // NOTE: This includes case where list length is zero
    if ((NULL == pLinkedList) || (NULL == pLinkedList->m_pHead) ||
        (NULL == pLinkedList->m_pTail))
    {
        DEBUG_PRINT("List/List head/List tail NULL");
        goto EXIT;
    }

    if (wIndex > (pLinkedList->m_wSize - 1))
    {
        DEBUG_PRINT("List index out of range");
        goto EXIT;
    }

    if (1 == pLinkedList->m_wSize)
    {
        pData = LinkedListReturnLast(pLinkedList);
        if (NULL == pData)
        {
            DEBUG_PRINT("LinkedListReturnLast failed");
        }
        goto EXIT;
    }

    if (0 == wIndex)
    {
        pData = LinkedListReturnBegin(pLinkedList);
        if (NULL == pData)
        {
            DEBUG_PRINT("LinkedListReturnBegin failed");
        }
        goto EXIT;
    }
    else
    {
        pData = LinkedListReturnAtIndex(pLinkedList, wIndex);
        if (NULL == pData)
        {
            DEBUG_PRINT("LinkedListReturnAtIndex failed");
        }
        goto EXIT;
    }

EXIT:
    return pData;
}

RETURNTYPE
LinkedListDestroy(PLINKEDLIST pLinkedList, VOID (*pfnFreeFunction)(PVOID))
{
    RETURNTYPE Return = ERR_GENERIC;

    // WARNING: iSize is assumed to be correct, altering value will result
    // in un-freed heap memory oir double frees.
    PLINKEDLISTNODE pTempNode  = pLinkedList->m_pHead;
    PLINKEDLISTNODE pTempNode2 = NULL;

    if (NULL == pLinkedList)
    {
        DEBUG_PRINT("NULL input");
        goto EXIT;
    }

    while (0 < pLinkedList->m_wSize)
    {
        if (NULL == pTempNode)
        {
            DEBUG_PRINT("NULL node");
            goto EXIT;
        }

        pTempNode2 = pTempNode->m_pNext;

        if (NULL != pfnFreeFunction)
        {
            pfnFreeFunction(pTempNode->m_pData);
        }

        if (SUCCESS != DestroyNode(pTempNode))
        {
            DEBUG_PRINT("DestroyNode failed");
            goto EXIT;
        }

        pLinkedList->m_wSize -= 1;
        pTempNode = pTempNode2;
    }

    ZeroingHeapFree(GetProcessHeap(), NO_OPTION, &pLinkedList,
                    sizeof(LINKEDLIST));

    Return = SUCCESS;
EXIT:
    return Return;
}

// NOTE: Function only being used for testing purposes.
//		Test works when WORD/int pointers are given to list.
VOID PrintList(PLINKEDLIST pLinkedList)
{
    PLINKEDLISTNODE pTempNode = NULL;
    WORD            wIndex    = 0;

    if (0 == pLinkedList->m_wSize)
    {
        printf("[]\n");
        return;
    }

    printf("[");
    pTempNode = pLinkedList->m_pHead;

    for (wIndex = 0; wIndex < pLinkedList->m_wSize; wIndex++)
    {
        printf("%u ", *(WORD *)pTempNode->m_pData);
        pTempNode = pTempNode->m_pNext;
    }
    printf("]\n");
}

// End of file
