/*****************************************************************//**
 * \file   Queue.h
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#pragma once
#include "pch.h"

/// <summary>
/// Node for linked list structure.
/// Has void pointer for random that will be held in linked list.
/// Contains pointer to the next node within the linked list.
/// </summary>
typedef struct QUEUENODE QUEUENODE; //NOTE: Enables inner pointer.

typedef struct QUEUENODE {
	PVOID	   m_pData;
	QUEUENODE* m_pNext;
	QUEUENODE* m_pPrev;
} QUEUENODE, *PQUEUENODE;

/// <summary>
/// Linked List structure
/// Contains pointer to head and tail of linked list.
/// Maintains linked list size in WORD value.
/// </summary>
typedef struct QUEUE {
	PQUEUENODE m_pHead;
	PQUEUENODE m_pTail;
	WORD	   m_iSize; //Max size is 65535.
} QUEUE, *PQUEUE;

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: CreateQueueNode

  Summary:  Creates an empty node to be assigned a void pointer and
			implemented in a queue.

  Args:     None

  Returns:  static PQUEUENODE
			  Empty Node for assignment and addition into queue.
-----------------------------------------------------------------F-F*/
static PQUEUENODE
CreateQueueNode();

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: DestroyQueueNode

  Summary:  Destroys a queue node.

  Args:     PQUEUENODE pQueueNode
			  Node for destruction.

  Returns:  static WORD
			  EXIT_SUCCESS (0) or EXIT_FAILURE (1) returned.
-----------------------------------------------------------------F-F*/
static WORD
DestroyQueueNode(PQUEUENODE pQueueNode);

/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
  Function: QueueInit

  Summary:  Allocates space for and creates empty queue structure.

  Args:     None

  Returns:  Queue *
			  queue custom data structure.
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
PQUEUE
QueueInit();

/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
  Function: QueuePush

  Summary:  Allocates space for and creates empty linked list structure.

  Args:     PQUEUE pQueue
			  queue custom data structure.
			PVOID pData
			  pointer to data to be entered into queue.

  Returns:  WORD
			  EXIT_SUCCESS (0) or EXIT_FAILURE (1) returned.
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
WORD
QueuePush(PQUEUE pQueue, PVOID pData);

/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
  Function: QueuePop

  Summary:  pops off the item at the end of the queue and returns it to
			calling function.

  Args:     PQUEUE pQueue

  Returns:  PVOID
			  Pointer to the data at specified position. If failure, NULL.
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
PVOID
QueuePop(PQUEUE pQueue);

PVOID
QueuePeek(PQUEUE pQueue);

WORD
QueuePopRemove(PQUEUE pQueue, VOID(*pfnFreeFunction)(PVOID));

/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
  Function: QueueDestroy

  Summary:  Removes all nodes from the queue and destroys it.

  Args:     PQUEUE pQueue
			  Queue struct that will be destroyed.

  Returns:  WORD
			  EXIT_SUCCESS (0) or EXIT_FAILURE (1) returned.
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
WORD
QueueDestroy(PQUEUE pQueue, VOID (*pfnFreeFunction)(PVOID));

// End of file
