/*****************************************************************//**
 * \file   LinkedList.c
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#include "pch.h"
#include "LinkedList.h"

static PLINKEDLISTNODE
CreateNode()
{
	PLINKEDLISTNODE pLinkedListNode = HeapAlloc(GetProcessHeap(),
		HEAP_ZERO_MEMORY,
		sizeof(LINKEDLISTNODE));

	if (NULL == pLinkedListNode)
	{
		PrintError((PCHAR)__func__, __LINE__);
		return NULL;
	}

	return pLinkedListNode;
}


static WORD
DestroyNode(PLINKEDLISTNODE pLinkedListNode)
{
	if (NULL == pLinkedListNode)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "NULL input");
		return EXIT_FAILURE;
	}

	if (WIN_EXIT_FAILURE == HeapFree(GetProcessHeap(), NO_OPTION,
		pLinkedListNode))
	{
		PrintError((PCHAR)__func__, __LINE__);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

PLINKEDLIST
LinkedListInit()
{
	PLINKEDLIST pLinkedList = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		sizeof(LINKEDLIST));

	if (NULL == pLinkedList)
	{
		PrintError((PCHAR)__func__, __LINE__);
		return NULL;
	}

	pLinkedList->m_pHead = NULL;
	pLinkedList->m_pTail = NULL;
	pLinkedList->m_wSize = 0;

	return pLinkedList;
}

static void
LinkedListInsertFirst(PLINKEDLIST pLinkedList, PLINKEDLISTNODE pLinkedListNode)
{
	pLinkedListNode->m_pNext = pLinkedListNode;
	pLinkedList->m_pHead = pLinkedListNode;
	pLinkedList->m_pTail = pLinkedListNode;
	pLinkedList->m_wSize = 1;
}


static void
LinkedListInsertBegin(PLINKEDLIST pLinkedList, PLINKEDLISTNODE pLinkedListNode)
{
	pLinkedListNode->m_pNext = pLinkedList->m_pHead;
	pLinkedList->m_pHead = pLinkedListNode;
	pLinkedList->m_pTail->m_pNext = pLinkedListNode;
	pLinkedList->m_wSize += 1;
}


static void
LinkedListInsertEnd(PLINKEDLIST pLinkedList, PLINKEDLISTNODE pLinkedListNode)
{
	pLinkedListNode->m_pNext = pLinkedList->m_pHead;
	pLinkedList->m_pTail->m_pNext = pLinkedListNode;
	pLinkedList->m_pTail = pLinkedListNode;
	pLinkedList->m_wSize += 1;
}

static WORD
LinkedListInsertAtIndex(PLINKEDLIST pLinkedList,
	PLINKEDLISTNODE pLinkedListNode, WORD wIndex)
{
	//See explanation below for selection of pTail here
	PLINKEDLISTNODE pTempNode = pLinkedList->m_pTail;
	WORD wTargetIndex = wIndex;
	wIndex = 0;

	do
	{
		pTempNode = pTempNode->m_pNext;
		wIndex += 1;

		if (NULL == pTempNode->m_pNext)
		{
			PrintErrorCustom((PCHAR)__func__, __LINE__, "NULL node");
			return EXIT_FAILURE;
		}
	} while (wIndex != wTargetIndex);

	//Now the node is placed before our target node index: i.e. if we are
	//trying to place a node at index 1, we'll have the list index 0 node.
	pLinkedListNode->m_pNext = pTempNode->m_pNext->m_pNext;
	pTempNode->m_pNext = pLinkedListNode;
	pLinkedList->m_wSize += 1;

	return EXIT_SUCCESS;
}

WORD
LinkedListInsert(PLINKEDLIST pLinkedList, PVOID pData, WORD wIndex)
{
	if (NULL == pLinkedList || NULL == pData)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "NULL input");
		return EXIT_FAILURE;
	}

	if (wIndex > pLinkedList->m_wSize)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "list index out of range");
		return EXIT_FAILURE;
	}

	PLINKEDLISTNODE pLinkedListNode = CreateNode();
	if (NULL == pLinkedListNode)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "CreateNode()");
		return EXIT_FAILURE;
	}

	pLinkedListNode->m_pData = pData;

	//Handle case where the size is 0
	if (NULL == pLinkedList->m_pHead)
	{
		//If the head is NULL but the size isn't 0, let's return error
		if (pLinkedList->m_wSize != 0)
		{
			PrintErrorCustom((PCHAR)__func__, __LINE__, "list head NULL");
			return EXIT_FAILURE;
		}

		LinkedListInsertFirst(pLinkedList, pLinkedListNode);
		return EXIT_SUCCESS;
	}

	//Tail should not be NULL either
	if (NULL == pLinkedList->m_pTail)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "list tail NULL");
		return EXIT_FAILURE;
	}

	if (0 == wIndex)
	{
		LinkedListInsertBegin(pLinkedList, pLinkedListNode);
		return EXIT_SUCCESS;
	}
	else if (wIndex == pLinkedList->m_wSize)
	{
		LinkedListInsertEnd(pLinkedList, pLinkedListNode);
		return EXIT_SUCCESS;
	}
	else
	{
		WORD wResult = LinkedListInsertAtIndex(pLinkedList, pLinkedListNode,
			wIndex);
		return wResult;
	}
}


static PVOID
LinkedListRemoveLast(PLINKEDLIST pLinkedList)
{
	PLINKEDLISTNODE pTempNode = pLinkedList->m_pHead;
	pLinkedList->m_pHead = NULL;
	pLinkedList->m_pTail = NULL;
	pLinkedList->m_wSize = 0;

	PVOID pData = pTempNode->m_pData;

	if (EXIT_FAILURE == DestroyNode(pTempNode))
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "DestroyNode()");
		return NULL;
	}

	return pData;
}


static PVOID
LinkedListRemoveBegin(PLINKEDLIST pLinkedList)
{
	PLINKEDLISTNODE pTempNode = pLinkedList->m_pHead;
	pLinkedList->m_pHead = pTempNode->m_pNext;
	pLinkedList->m_pTail->m_pNext = pLinkedList->m_pHead;
	pLinkedList->m_wSize -= 1;

	PVOID pData = pTempNode->m_pData;

	if (EXIT_FAILURE == DestroyNode(pTempNode))
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "DestroyNode()");
		return NULL;
	}

	return pData;
}

static PVOID
LinkedListRemoveAtIndex(PLINKEDLIST pLinkedList, WORD wIndex)
{
	PLINKEDLISTNODE pTempNode = pLinkedList->m_pTail;

	WORD wTargetIndex = wIndex;
	wIndex = 0;

	do
	{
		pTempNode = pTempNode->m_pNext;
		wIndex += 1;

		if (NULL == pTempNode->m_pNext)
		{
			PrintErrorCustom((PCHAR)__func__, __LINE__, "NULL node");
			return NULL;
		}
	} while (wIndex != wTargetIndex);

	//Now the node is placed before our target node index: i.e. if we are
	//trying to place a node at index 1, we'll have the list index 0 node.
	PLINKEDLISTNODE pDeleteNode = pTempNode->m_pNext;
	pTempNode->m_pNext = pDeleteNode->m_pNext;
	if (wTargetIndex == (pLinkedList->m_wSize - 1))
	{
		pLinkedList->m_pTail = pTempNode;
	}
	pLinkedList->m_wSize -= 1;

	PVOID pData = pDeleteNode->m_pData;

	if (EXIT_FAILURE == DestroyNode(pDeleteNode))
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "DestroyNode()");
		return NULL;
	}

	return pData;
}

WORD
LinkedListRemove(PLINKEDLIST pLinkedList, WORD wIndex,
	VOID (*pfnFreeFunction)(PVOID))
{
	if ((NULL == pLinkedList) || (NULL == pLinkedList->m_pHead) ||
		(NULL == pLinkedList->m_pTail))
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "List/List head/List tail NULL");
		return EXIT_FAILURE;
	}

	if (wIndex > (pLinkedList->m_wSize - 1))
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "list index out of range");
		return EXIT_FAILURE;
	}

	PVOID pData = NULL;

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
		PrintErrorCustom((PCHAR)__func__, __LINE__, "Data was NULL");
		return EXIT_FAILURE;
	}

	if (NULL != pfnFreeFunction)
	{
		pfnFreeFunction(pData);
	}

	return EXIT_SUCCESS;
}


static PVOID
LinkedListReturnLast(PLINKEDLIST pLinkedList, WORD wFlags)
{
	if (DONT_DESTROY_NODE == wFlags)
	{
		return pLinkedList->m_pHead->m_pData;
	}

	PLINKEDLISTNODE pTempNode = pLinkedList->m_pHead;
	pLinkedList->m_pHead = NULL;
	pLinkedList->m_pTail = NULL;
	pLinkedList->m_wSize = 0;

	PVOID pData = pTempNode->m_pData;

	if (EXIT_FAILURE == DestroyNode(pTempNode))
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "DestroyNode()");
		return NULL;
	}

	return pData;
}


static PVOID
LinkedListReturnBegin(PLINKEDLIST pLinkedList, WORD wFlags)
{
	if (DONT_DESTROY_NODE == wFlags)
	{
		return pLinkedList->m_pHead->m_pData;
	}

	PLINKEDLISTNODE pTempNode = pLinkedList->m_pHead;
	pLinkedList->m_pHead = pTempNode->m_pNext;
	pLinkedList->m_pTail->m_pNext = pLinkedList->m_pHead;
	pLinkedList->m_wSize -= 1;

	PVOID pData = pTempNode->m_pData;

	if (EXIT_FAILURE == DestroyNode(pTempNode))
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "DestroyNode()");
		return NULL;
	}

	return pData;
}


static PVOID
LinkedListReturnAtIndex(PLINKEDLIST pLinkedList, WORD wIndex, WORD wFlags)
{
	PLINKEDLISTNODE pTempNode = pLinkedList->m_pTail;

	WORD wTargetIndex = wIndex;
	wIndex = 0;

	do
	{
		pTempNode = pTempNode->m_pNext;
		wIndex += 1;

		if (NULL == pTempNode->m_pNext)
		{
			PrintErrorCustom((PCHAR)__func__, __LINE__, "NULL node");
			return NULL;
		}
	} while (wIndex != wTargetIndex);

	//Now the node is placed before our target node index: i.e. if we are
	//trying to place a node at index 1, we'll have the list index 0 node.

	if (DONT_DESTROY_NODE == wFlags)
	{
		return pTempNode->m_pNext->m_pData;
	}

	PLINKEDLISTNODE pDeleteNode = pTempNode->m_pNext;
	pTempNode->m_pNext = pDeleteNode->m_pNext;
	if (wTargetIndex == (pLinkedList->m_wSize - 1))
	{
		pLinkedList->m_pTail = pTempNode;
	}
	pLinkedList->m_wSize -= 1;

	PVOID pData = pDeleteNode->m_pData;

	if (EXIT_FAILURE == DestroyNode(pDeleteNode))
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "DestroyNode()");
		return NULL;
	}

	return pData;
}


PVOID
LinkedListReturn(PLINKEDLIST pLinkedList, WORD wIndex, WORD wFlags)
{
	//NOTE: This includes case where list length is zero
	if ((NULL == pLinkedList) || (NULL == pLinkedList->m_pHead) ||
		(NULL == pLinkedList->m_pTail))
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "List/List head/List tail NULL");
		return NULL;
	}

	if (wIndex > (pLinkedList->m_wSize - 1))
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "list index out of range");
		return NULL;
	}

	PVOID pData;

	if (1 == pLinkedList->m_wSize)
	{
		pData = LinkedListReturnLast(pLinkedList, wFlags);
		if (NULL == pData)
		{
			PrintErrorCustom((PCHAR)__func__, __LINE__, "LinkedListReturnLast()");
		}
		return pData;
	}

	if (0 == wIndex)
	{
		pData = LinkedListReturnBegin(pLinkedList, wFlags);
		if (NULL == pData)
		{
			PrintErrorCustom((PCHAR)__func__, __LINE__, "LinkedListReturnBegin()");
		}
		return pData;
	}
	else
	{
		pData = LinkedListReturnAtIndex(pLinkedList, wIndex, wFlags);
		if (NULL == pData)
		{
			PrintErrorCustom((PCHAR)__func__, __LINE__, "LinkedListReturnAtIndex()");
		}
		return pData;
	}
}


WORD
LinkedListDestroy(PLINKEDLIST pLinkedList,
	VOID (*pfnFreeFunction)(PVOID))
{
	if (NULL == pLinkedList)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "NULL input");
		return EXIT_FAILURE;
	}

	//WARNING: iSize is assumed to be correct, altering value will result
	//in un-freed heap memory oir double frees.
	PLINKEDLISTNODE pTempNode = pLinkedList->m_pHead;
	PLINKEDLISTNODE pTempNode2;
	WORD wResult = EXIT_SUCCESS;

	while (0 < pLinkedList->m_wSize)
	{
		if ((NULL == pTempNode) || (NULL == pTempNode->m_pNext))
		{
			PrintErrorCustom((PCHAR)__func__, __LINE__, "NULL node");
			wResult = EXIT_FAILURE;
			break;
		}

		pTempNode2 = pTempNode->m_pNext;

		if (NULL != pfnFreeFunction)
		{
			pfnFreeFunction(pTempNode->m_pData);
		}

		if (EXIT_FAILURE == DestroyNode(pTempNode))
		{
			PrintErrorCustom((PCHAR)__func__, __LINE__, "DestroyNode()");
			wResult = EXIT_FAILURE;
			break;
		}

		pLinkedList->m_wSize -= 1;
		pTempNode = pTempNode2;
	}

	if (WIN_EXIT_FAILURE == HeapFree(GetProcessHeap(),
		NO_OPTION, pLinkedList))
	{
		PrintError((PCHAR)__func__, __LINE__);
		return EXIT_FAILURE;
	}

	return wResult;
}

//NOTE: Function only being used for testing purposes.
//		Test works when WORD/int pointers are given to list.
VOID
PrintList(PLINKEDLIST pLinkedList)
{
	if (0 == pLinkedList->m_wSize)
	{
		printf("[]\n");
		return;
	}

	printf("[");
	PLINKEDLISTNODE pTempNode = pLinkedList->m_pHead;

	for (WORD wIndex = 0; wIndex < pLinkedList->m_wSize; wIndex++)
	{
		printf("%u ", *(WORD*)pTempNode->m_pData);
		pTempNode = pTempNode->m_pNext;
	}
	printf("]\n");
}

//TODO: port over sort function if required

//End of file
