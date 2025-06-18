#include "pch.h"
#include "Queue.h"

static PQUEUENODE
CreateQueueNode()
{
	PQUEUENODE pQueueNode = HeapAlloc(GetProcessHeap(),
		HEAP_ZERO_MEMORY,
		sizeof(QUEUENODE));

	if (NULL == pQueueNode)
	{
		PrintError((PCHAR)__func__, __LINE__);
		return NULL;
	}

	return pQueueNode;
}


static WORD
DestroyQueueNode(PQUEUENODE pQueueNode)
{
	if (NULL == pQueueNode)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "NULL input");
		return EXIT_FAILURE;
	}

	if (WIN_EXIT_FAILURE == HeapFree(GetProcessHeap(),
		NO_OPTION, pQueueNode))
	{
		PrintError((PCHAR)__func__, __LINE__);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

PQUEUE
QueueInit()
{
	PQUEUE pQueue = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		sizeof(QUEUE));
	if (NULL == pQueue)
	{
		PrintError((PCHAR)__func__, __LINE__);
		return NULL;
	}

	pQueue->m_pHead = NULL;
	pQueue->m_pTail = NULL;
	pQueue->m_iSize = 0;

	return pQueue;
}

WORD
QueuePush(PQUEUE pQueue, PVOID pData)
{
	if (NULL == pQueue || NULL == pData)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "NULL input");
		return EXIT_FAILURE;
	}

	PQUEUENODE pQueueNode = CreateQueueNode();
	if (NULL == pQueueNode)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "CreateNode()");
		return EXIT_FAILURE;
	}

	pQueueNode->m_pData = pData;

	if (0 == pQueue->m_iSize)
	{
		pQueue->m_pTail = pQueueNode;
		pQueue->m_pHead = pQueueNode;
		pQueueNode->m_pNext = NULL;
	}
	else
	{
		if (NULL == pQueue->m_pHead)
		{
			PrintErrorCustom((PCHAR)__func__, __LINE__, "NULL head");
			return EXIT_FAILURE;
		}

		pQueue->m_pHead->m_pPrev = pQueueNode;
		pQueueNode->m_pNext = pQueue->m_pHead;
		pQueue->m_pHead = pQueueNode;
	}

	pQueueNode->m_pPrev = NULL;
	pQueue->m_iSize += 1;

	return EXIT_SUCCESS;
}

//TODO: update with free fn
PVOID
QueuePop(PQUEUE pQueue)
{
	if (NULL == pQueue || NULL == pQueue->m_pTail)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "NULL input");
		return NULL;
	}

	PQUEUENODE pTempNode;

	if (0 == pQueue->m_iSize)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__,
			"Can't return from an empty list");
		return NULL;
	}
	else if (1 == pQueue->m_iSize)
	{
		pTempNode = pQueue->m_pHead;
		pQueue->m_pHead = NULL;
		pQueue->m_pTail = NULL;
	}
	else
	{
		pTempNode = pQueue->m_pTail;
		pQueue->m_pTail = pTempNode->m_pPrev;
		pQueue->m_pTail->m_pNext = NULL;
	}

	pQueue->m_iSize -= 1;
	PVOID pData = pTempNode->m_pData;

	if (EXIT_FAILURE == DestroyQueueNode(pTempNode))
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "DestroyQueueNode()");
		return NULL;
	}

	return pData;
}

PVOID
QueuePeek(PQUEUE pQueue)
{
	return pQueue->m_pTail->m_pData;
}

WORD
QueuePopRemove(PQUEUE pQueue, VOID(*pfnFreeFunction)(PVOID))
{
	if (NULL == pQueue || NULL == pQueue->m_pTail || NULL == pfnFreeFunction)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "NULL input");
		return EXIT_FAILURE;
	}

	PQUEUENODE pTempNode;

	if (0 == pQueue->m_iSize)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__,
			"Can't return from an empty list");
		return EXIT_SUCCESS;
	}
	else if (1 == pQueue->m_iSize)
	{
		pTempNode = pQueue->m_pHead;
		pQueue->m_pHead = NULL;
		pQueue->m_pTail = NULL;
	}
	else
	{
		pTempNode = pQueue->m_pTail;
		pQueue->m_pTail = pTempNode->m_pPrev;
		pQueue->m_pTail->m_pNext = NULL;
	}

	pQueue->m_iSize -= 1;
	PVOID pData = pTempNode->m_pData;

	if (EXIT_FAILURE == DestroyQueueNode(pTempNode))
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "DestroyQueueNode()");
		return EXIT_FAILURE;
	}

	pfnFreeFunction(pData);
	return EXIT_SUCCESS;
}

WORD
QueueDestroy(PQUEUE pQueue, VOID (*pfnFreeFunction)(PVOID))
{
	if (NULL == pQueue)
	{
		PrintErrorCustom((PCHAR)__func__, __LINE__, "NULL input");
		return EXIT_FAILURE;
	}

	//WARNING: iSize is assumed to be correct, altering value will result
	//in un-freed heap memory oir double frees.
	PQUEUENODE pTempNode = pQueue->m_pHead;
	PQUEUENODE pTempNode2;
	WORD iResult = EXIT_SUCCESS;

	while (0 < pQueue->m_iSize)
	{
		if (NULL == pTempNode)
		{
			PrintErrorCustom((PCHAR)__func__, __LINE__, "NULL node");
			iResult = EXIT_FAILURE;
			break;
		}

		pTempNode2 = pTempNode->m_pNext;

		if (NULL != pfnFreeFunction)
		{
			pfnFreeFunction(pTempNode->m_pData);
		}

		if (EXIT_FAILURE == DestroyQueueNode(pTempNode))
		{
			PrintErrorCustom((PCHAR)__func__, __LINE__, "DestroyQueueNode()");
			iResult = EXIT_FAILURE;
			break;
		}

		pQueue->m_iSize -= 1;
		pTempNode = pTempNode2;
	}

	if (WIN_EXIT_FAILURE == HeapFree(GetProcessHeap(),
		NO_OPTION, pQueue))
	{
		PrintError((PCHAR)__func__, __LINE__);
		return EXIT_FAILURE;
	}

	return iResult;
}

//End of file
