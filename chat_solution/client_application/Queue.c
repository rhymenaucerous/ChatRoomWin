#include <Windows.h>
#include <stdio.h>

#include "Queue.h"
#include "c_shared.h"

static PQUEUENODE
CreateQueueNode()
{
	PQUEUENODE pQueueNode = HeapAlloc(GetProcessHeap(),
		HEAP_ZERO_MEMORY,
		sizeof(QUEUENODE));

	if (NULL == pQueueNode)
	{
		DEBUG_ERROR("HeapAlloc failed");
		return NULL;
	}

	return pQueueNode;
}


static WORD
DestroyQueueNode(PQUEUENODE pQueueNode)
{
	if (NULL == pQueueNode)
	{
		DEBUG_PRINT("NULL input");
		return ERR_INVALID_PARAM;
	}

	ZeroingHeapFree(GetProcessHeap(),
		NO_OPTION, &pQueueNode, sizeof(QUEUENODE));

	return SUCCESS;
}

PQUEUE
QueueInit()
{
	PQUEUE pQueue = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		sizeof(QUEUE));
	if (NULL == pQueue)
	{
		DEBUG_ERROR("HeapAlloc failed");
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
		DEBUG_PRINT("NULL input");
		return ERR_INVALID_PARAM;
	}

	PQUEUENODE pQueueNode = CreateQueueNode();
	if (NULL == pQueueNode)
	{
		DEBUG_PRINT("CreateNode()");
		return ERR_GENERIC;
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
			DEBUG_PRINT("NULL head");
			return ERR_GENERIC;
		}

		pQueue->m_pHead->m_pPrev = pQueueNode;
		pQueueNode->m_pNext = pQueue->m_pHead;
		pQueue->m_pHead = pQueueNode;
	}

	pQueueNode->m_pPrev = NULL;
	pQueue->m_iSize += 1;

	return SUCCESS;
}

//TODO: update with free fn
PVOID
QueuePop(PQUEUE pQueue)
{
	if (NULL == pQueue || NULL == pQueue->m_pTail)
	{
		DEBUG_PRINT("NULL input");
		return NULL;
	}

	PQUEUENODE pTempNode;
	if (0 == pQueue->m_iSize)
	{
		DEBUG_PRINT("Can't return from an empty list");
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

	if (SUCCESS != DestroyQueueNode(pTempNode))
	{
		DEBUG_PRINT("DestroyQueueNode()");
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
		DEBUG_PRINT("NULL input");
		return ERR_INVALID_PARAM;
	}

	PQUEUENODE pTempNode;

	if (0 == pQueue->m_iSize)
	{
		DEBUG_PRINT("Can't return from an empty list");
		return SUCCESS;
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

	if (SUCCESS != DestroyQueueNode(pTempNode))
	{
		DEBUG_PRINT("DestroyQueueNode()");
		return ERR_GENERIC;
	}

	pfnFreeFunction(pData);
	return SUCCESS;
}

WORD
QueueDestroy(PQUEUE pQueue, VOID (*pfnFreeFunction)(PVOID))
{
	if (NULL == pQueue)
	{
		DEBUG_PRINT("NULL input");
		return ERR_INVALID_PARAM;
	}

	//WARNING: iSize is assumed to be correct, altering value will result
	//in un-freed heap memory oir double frees.
	PQUEUENODE pTempNode = pQueue->m_pHead;
	PQUEUENODE pTempNode2;
	WORD iResult = SUCCESS;

	while (0 < pQueue->m_iSize)
	{
		if (NULL == pTempNode)
		{
			DEBUG_PRINT("NULL node");
			iResult = ERR_GENERIC;
			break;
		}

		pTempNode2 = pTempNode->m_pNext;

		if (NULL != pfnFreeFunction)
		{
			pfnFreeFunction(pTempNode->m_pData);
		}

		if (SUCCESS != DestroyQueueNode(pTempNode))
		{
			DEBUG_PRINT("DestroyQueueNode()");
			iResult = ERR_GENERIC;
			break;
		}

		pQueue->m_iSize -= 1;
		pTempNode = pTempNode2;
	}

	ZeroingHeapFree(GetProcessHeap(),
		NO_OPTION, &pQueue, sizeof(QUEUE));

	return iResult;
}

//End of file
