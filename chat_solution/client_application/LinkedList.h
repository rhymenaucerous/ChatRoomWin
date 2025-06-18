/*****************************************************************//**
 * \file   LinkedList.h
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#pragma once
#include "pch.h"

#define DONT_DESTROY_NODE 1
#define DESTROY_NODE 0

 /// <summary>
 /// Node for linked list structure.
 /// Has VOID pointer for random that will be held in linked list.
 /// Contains pointer to the next node within the linked list.
 /// </summary>
typedef struct LINKEDLISTNODE {
	PVOID				   m_pData;
	struct LINKEDLISTNODE* m_pNext;
} LINKEDLISTNODE, *PLINKEDLISTNODE;

/// <summary>
/// Linked List structure
/// Contains pointer to head and tail of linked list.
/// Maintains linked list size in WORD value.
/// </summary>
typedef struct LINKEDLIST {
	struct LINKEDLISTNODE* m_pHead;
	struct LINKEDLISTNODE* m_pTail;
	WORD				   m_wSize; //Max size is 65535.
} LINKEDLIST, *PLINKEDLIST, ** PPLINKEDLIST;

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: CreateNode

  Summary:  Creates an empty node to be assigned a VOID pointer and
			implemented into a linked list.

  Args:     None

  Returns:  static LinkedListNode*
			  Empty Node for assignment and addition into linked list.
-----------------------------------------------------------------F-F*/
static PLINKEDLISTNODE
CreateNode();

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: DestroyNode

  Summary:  Destroys a node.

  Args:     LinkedListNode* pLinkedListNode
			  Node for destruction.

  Returns:  static WORD
			  EXIT_SUCCESS (0) or EXIT_FAILURE (1) returned.
-----------------------------------------------------------------F-F*/
static WORD
DestroyNode(PLINKEDLISTNODE pLinkedListNode);

/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
  Function: LinkedListInit

  Summary:  Allocates space for and creates empty linked list structure.

  Args:     None

  Returns:  PLINKEDLIST
			  LinkedList custom data structure.
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
PLINKEDLIST
LinkedListInit();

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: LinkedListInsertFirst

  Summary:  static function that is called by LinkedListInsert when an
			insertion is being made into an empty list.

  Args:     PLINKEDLIST pLinkedList
			  Linked List struct that will have node added.
			PLINKEDLISTNODE pLinkedListNode
			  Node struct to be added to linked list.

  Returns:  VOID
			  No return.
-----------------------------------------------------------------F-F*/
static VOID
LinkedListInsertFirst(PLINKEDLIST pLinkedList,
	PLINKEDLISTNODE pLinkedListNode);

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: LinkedListInsertBegin

  Summary:  static function that is called by LinkedListInsert when an
			insertion is being made to the first index of a linked list.

  Args:     PLINKEDLIST pLinkedList
			  Linked List struct that will have node added.
			PLINKEDLISTNODE pLinkedListNode
			  Node struct to be added to linked list.

  Returns:  VOID
			  No return.
-----------------------------------------------------------------F-F*/
static VOID
LinkedListInsertBegin(PLINKEDLIST pLinkedList,
	PLINKEDLISTNODE pLinkedListNode);

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: LinkedListInsertEnd

  Summary:  static function that is called by LinkedListInsert when an
			insertion is being made to the last index of a linked list.

  Args:     PLINKEDLIST pLinkedList
			  Linked List struct that will have node added.
			PLINKEDLISTNODE pLinkedListNode
			  Node struct to be added to linked list.

  Returns:  VOID
			  No return.
-----------------------------------------------------------------F-F*/
static VOID
LinkedListInsertEnd(PLINKEDLIST pLinkedList,
	PLINKEDLISTNODE pLinkedListNode);

/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
  Function: LinkedListInsert

  Summary:  Creates a new node, adds the input VOID pointer to it, and
			places that node at the index provided within the linked
			list.

  Args:     PLINKEDLIST pLinkedList
			  Linked List struct that will have node added.
			PVOID pData
			  pointer to data that will be placed in the list.
			WORD wIndex
			  specified index for node insertion.


  Returns:  WORD
			  EXIT_SUCCESS (0) or EXIT_FAILURE (1) returned.
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
WORD
LinkedListInsert(PLINKEDLIST pLinkedList, PVOID pData, WORD wIndex);

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: LinkedListRemoveLast

  Summary:  static function that is called by LinkedListRemove when the
			node being removed is the only in the list.

  Args:     PLINKEDLIST pLinkedList
			  Linked List struct that will have node removed.

  Returns:  static PVOID
			  Pointer to the data at specified position. If failure, NULL.
-----------------------------------------------------------------F-F*/
static PVOID
LinkedListRemoveLast(PLINKEDLIST pLinkedList);

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: LinkedListRemoveBegin

  Summary:  static function that is called by LinkedListRemove when the
			node being removed is the first (at index 0) in the list.

  Args:     PLINKEDLIST pLinkedList
			  Linked List struct that will have node removed.

  Returns:  static PVOID
			  Pointer to the data at specified position. If failure, NULL.
-----------------------------------------------------------------F-F*/
static PVOID
LinkedListRemoveBegin(PLINKEDLIST pLinkedList);

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: LinkedListRemoveAtIndex

  Summary:  static function that is called by LinkedListRemove when the
			node being removed is the first (at index 0) in the list.

  Args:     PLINKEDLIST pLinkedList
			  Linked List struct that will have node removed.
			WORD wIndex
			  Index for removal.

  Returns:  static PVOID
			  Pointer to the data at specified position. If failure, NULL.
-----------------------------------------------------------------F-F*/
static PVOID
LinkedListRemoveAtIndex(PLINKEDLIST pLinkedList, WORD wIndex);

/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
  Function: LinkedListRemove

  Summary:  Removes a node at the specified index.

  Args:     PLINKEDLIST pLinkedList
			  Linked List struct that will have node removed.
			WORD wIndex
			  specified index for node deletion.


  Returns:  WORD
			  EXIT_SUCCESS (0) or EXIT_FAILURE (1) returned.
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
WORD
LinkedListRemove(PLINKEDLIST pLinkedList, WORD wIndex,
	VOID (*pfnFreeFunction)(PVOID));

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: LinkedListReturnLast


  Summary:  static function that is called by LinkedListReturn when the
			node being returned is the last node left.

  Args:     PLINKEDLIST pLinkedList
			  Linked List struct that will have node returned.

  Returns:  static PVOID
			  Pointer to the data at specified position. If failure, NULL.
-----------------------------------------------------------------F-F*/
static PVOID
LinkedListReturnLast(PLINKEDLIST pLinkedList, WORD wFlags);

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: LinkedListReturnBegin


  Summary:  static function that is called by LinkedListReturn when the
			node being returned is the first (at index 0) in the list.

  Args:     PLINKEDLIST pLinkedList
			  Linked List struct that will have node returned.

  Returns:  static PVOID
			  Pointer to the data at specified position. If failure, NULL.
-----------------------------------------------------------------F-F*/
static PVOID
LinkedListReturnBegin(PLINKEDLIST pLinkedList, WORD wFlags);

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: LinkedListReturnAtIndex

  Summary:  static function that is called by LinkedListReturn when the
			node being returned is not the last one left or the first in
			the list.

  Args:     PLINKEDLIST pLinkedList
			  Linked List struct that will have node returned.
			WORD wIndex
			  Index for return.

  Returns:  static PVOID
			  Pointer to the data at specified position. If failure, NULL.
-----------------------------------------------------------------F-F*/
static PVOID
LinkedListReturnAtIndex(PLINKEDLIST pLinkedList, WORD wIndex, WORD wFlags);

/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
  Function: LinkedListRemove

  Summary:  Returns a node at the specified index. Works like pop in that
			the node is removed from the list.

  Args:     PLINKEDLIST pLinkedList
			  Linked List struct that will have node removed.
			WORD wIndex
			  specified index for node deletion.


  Returns:  PVOID
			  Pointer to the data at specified position. If failure, NULL.
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
PVOID
LinkedListReturn(PLINKEDLIST pLinkedList, WORD wIndex, WORD wFlags);

/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
  Function: LinkedListDestroy

  Summary:  Removes all nodes from the linked list and destroys it.

  Args:     PLINKEDLIST pLinkedList
			  Linked List struct that will be destroyed.
			VOID (*pfnFreeFunction)(PVOID))
			  Function to be used to free data with hash table entries.

  Returns:  WORD
			  EXIT_SUCCESS (0) or EXIT_FAILURE (1) returned.
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
WORD
LinkedListDestroy(PLINKEDLIST pLinkedList,
	VOID (*pfnFreeFunction)(PVOID));

//NOTE: Temporary function for testing.
VOID
PrintList(PLINKEDLIST pLinkedList);

// End of file


