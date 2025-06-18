/*****************************************************************//**
 * \file   HashTable.h
 * \brief
 *
 * \author chris
 * \date   August 2024
 *********************************************************************/
#pragma once
#include "pch.h"
#include "LinkedList.h"

#define KEY_LENGTH 10
#define MIN_CAPACITY 7 //Do not set to 0 or library will divide by 0
#define LOAD_FACTOR_UPDATE 1.5 //Once the load factor is above this set
 //value, the hash table will be re-hashed.
 //This allows for maximum efficiency when
 //adding new values. Per wikipedia, optimal load
 //factor for separate chaining hash tables is
 //between 1 and 3.
#define MAX_PRIME_IN_RANGE 65521 //Largest prime in unsigned 16-bit int range
#define DUPLICATE_KEY 2

/// <summary>
/// Entry for hash table structure.
/// Has void pointer for data that will be held in the entry.
/// Has char array for 
/// </summary>
typedef struct HASHTABLEENTRY {
	PVOID m_pData;
	WCHAR m_caKey[KEY_LENGTH + 1];
	WORD  m_wKeyLen;
	VOID  (*m_pfnFreeFunction)(PVOID);
} HASHTABLEENTRY, *PHASHTABLEENTRY;

/// <summary>
/// Linked List structure
/// Contains pointer to head and tail of linked list.
/// Maintains linked list size in WORD value.
/// </summary>
typedef struct HASHTABLE {
	WORD         m_wSize; //Max size is 65535.
	WORD         m_wCapacity; //Number of rows in hash table
	INT64		 (*m_pfnHashFunction)(PVOID);
	PPLINKEDLIST m_ppTable;
} HASHTABLE, *PHASHTABLE;

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: ModularExponentiation

  Summary:  Recursive function that performs modular exponentiation much
			more efficiently than a normal calculation.

			Psuedo code pulled from (split up URL to meet coding style):
			https://en.wikipedia.org/wiki/Modular_exponentiation#:~
			:text=Modular%20exponentiation%20is%20the%20remainder,
			c%20%3D%20be%20mod%20m.

  Args:     WORD dwBase
			  The base of the exponent
			WORD dwExponent
			  The exponent that the base is raised to.
			WORD dwModulus
			  The modulus that is being used of the exponent group.

  Returns:  static WORD
			  The result of the expression.
-----------------------------------------------------------------F-F*/
static DWORD
ModularExponentiation(DWORD dwBase, DWORD dwExponent, DWORD dwModulus);

/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
  Function: IsPrime

  Summary:  Returns True if if value is prime and false if not. Uses the
			Miller-Rabin test. The number of rounds determines the accuracy
			of testing. We are only testing primes up to the value of 65535,
			and as such, X rounds will do.



			Note: A table of all primes up to 65535 (sieve of Eratosthenes)
				  would probably an alternative way to verify primes (i.e.
				  checking against a set)

  Args:     WORD wValue
			  Value being tested for primality.

  Returns:  BOOL
			  True or false.
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
BOOL
IsPrime(WORD wValue);

/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
  Function: NextPrime

  Summary:  Given a value, this function determines the next prime
			prime value in the series of natural numbers.

  Args:     WORD wValue
			  The starting value for the prime calculator.

  Returns:  WORD
			  The next prime.
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
WORD
NextPrime(WORD wValue);

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: DefaultHashFunction

  Summary:  function that generates hash given a string. will be used as
			default hash function for this library.

  Args:     PVOID pKey
			  char array with key. The reason it is void pointer type is to
			  allow user defined functions to replace it.

  Returns:  static INT64
			  The hash of the given key created using the FNV hash. The
			  reason it is type INT64 is to allow for the return of a negative
			  one to indicate failure. Otherwise DWORD would be fine.

  Warning:  The void pointer pKey must have allocated space up to KEY_LENGTH.
			This is on the user to ensure. The function will access the space
			following the pointer and if it isn't allocated, undefined behavior
			will follow.
-----------------------------------------------------------------F-F*/
static INT64
DefaultHashFunction(PVOID pKey);

/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
  Function: HashTableInit

  Summary:  Creates a hash table data structure.

  Args:     WORD iCapacity
			  The initial capacity for the hash table. determines the
			  number of chained linked lists.
			INT64 (*pfnHashFunction)(PVOID)
			  Hash function to hash the strings into the table. Must take
			  PVOID type as input and must output INT64 type.

  Returns:  PHASHTABLE
			  A pointer to the hash table that was created.
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
PHASHTABLE
HashTableInit(WORD iCapacity, INT64(*pfnHashFunction)(PVOID));

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: DuplicateDataCheck

  Summary:  Checks if the same key has already been used on the table,
			and then returns an error if it has.

  Args:     PLINKEDLIST pLinkedList
			  Pointer to the linked list that contains the entries with
			  the keys that make the hashes the have the same mod versus
			  the number of chained linked lists. If the key is already in
			  the hash table, it will be here.
			PWSTR pszKey
			  Pointer to the key.

  Returns:  static WORD
			  EXIT_SUCCESS (0) or EXIT_FAILURE (1) returned.
-----------------------------------------------------------------F-F*/
static WORD
DuplicateDataCheck(PLINKEDLIST pLinkedList, PWCHAR pszKey);

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: HashTableNewEntryCalc

  Summary:  Calculates the hash of the new entry, checks for duplicates,
			and then adds the new entry to the table.

  Args:     PHASHTABLE pHashTable
			  Pointer to the hash table data structure.
			PHASHTABLEENTRY pNewEntry
			  Pointer to the new entry.

  Returns:  static WORD
			  EXIT_SUCCESS (0) or EXIT_FAILURE (1) returned.
-----------------------------------------------------------------F-F*/
static WORD
HashTableNewEntryCalc(PHASHTABLE pHashTable, PHASHTABLEENTRY pNewEntry);

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: LoadFactor

  Summary:  Calculates the load factor of the table (entries/number of
			chained linked lists).

  Args:     PHASHTABLE pHashTable
			  Pointer to the hash table data structure.

  Returns:  static double
			  load factor or EXIT_FAILURE_NEGATIVE (-1) returned.
-----------------------------------------------------------------F-F*/
static double
LoadFactor(PHASHTABLE pHashTable);

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: HashTabletoList

  Summary:  Removes all the entries from the hash table and puts them
			into a list. Helper function to rehash.

  Args:     PHASHTABLE pHashTable
			  Pointer to the hash table data structure.

  Returns:  static PLINKEDLIST
			  pointer to the new linked list of NULL on failure.
-----------------------------------------------------------------F-F*/
static PLINKEDLIST
HashTabletoList(PHASHTABLE pHashTable);

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: HashTableListToTable

  Summary:  Takes the list made in HashTabletoList and puts all the
			entries into a new hash table. Helper function to rehash.

  Args:     PHASHTABLE pHashTable
			  Pointer to the hash table data structure.
			PLINKEDLIST pLinkedList
			  Linked list with all entries from hash table.

  Returns:  static double
			  EXIT_SUCCESS (0) or EXIT_FAILURE (1) returned.
-----------------------------------------------------------------F-F*/
static WORD
HashTableListToTable(PHASHTABLE pHashTable, PLINKEDLIST pLinkedList);

/*F+F+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Function: HashTableReHash

  Summary:  Rehashes the hash table to make it bigger (more chained lists)
			when it reaches a pre-defined load factor to maintain the
			table's efficieny.

  Args:     PHASHTABLE pHashTable
			  Pointer to the hash table data structure.

  Returns:  static double
			  EXIT_SUCCESS (0) or EXIT_FAILURE (1) returned.
-----------------------------------------------------------------F-F*/
static WORD
HashTableReHash(PHASHTABLE pHashTable);

/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
  Function: HashTableNewEntry

  Summary:  Creates a new entry in the hash table.

  Args:     PHASHTABLE pHashTable
			  Pointer to the hash table data structure.
			PVOID pData
			  Pointer to the data to be stored in the table.
			PWCHAR pKey
			  pointer to the key for the data.
			WORD wKeyLen
			  length of the data being copied, must be shorter than KEY_LENGTH.

  Returns:  WORD
			  EXIT_SUCCESS (0) or EXIT_FAILURE (1) returned.
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
WORD
HashTableNewEntry(PHASHTABLE pHashTable, PVOID pData, PWCHAR pszKey,
	WORD wKeyLen);

/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
  Function: HashTableReturnEntry

  Summary:  Returns data for a given key from the hash table.

  Args:     PHASHTABLE pHashTable
			  Pointer to the hash table data structure.
			PWSTR pKey
			  pointer to the key for the data.

  Returns:  PVOID
			  pointer to the requested data or NULL.
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
PVOID
HashTableReturnEntry(PHASHTABLE pHashTable, PWCHAR pszKey, WORD wKeyLen);

/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
  Function: HashTableReturnEntry

  Summary:  Destroys an entry for a given key from the hash table.

  Args:     PHASHTABLE pHashTable
			  Pointer to the hash table data structure.
			PWSTR pKey
			  pointer to the key for the data.

  Returns:  PVOID
			  pointer to the requested data or NULL.
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
PVOID
HashTableDestroyEntry(PHASHTABLE pHashTable, PWCHAR pszKey, WORD wKeyLen);

/*F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
  Function: HashTableDestroy

  Summary:  Removes all entries from the hash table and destroys it.

  Args:     PHASHTABLE pHashTable
			  Hash table struct that will be destroyed.
			VOID (*pfnFreeFunction)(void*)
			  Function to be used to free data with hash table entries.

  Returns:  WORD
			  EXIT_SUCCESS (0) or EXIT_FAILURE (1) returned.
F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F*/
WORD
HashTableDestroy(PHASHTABLE pHashTable, VOID (*pfnFreeFunction)(PVOID));

//End of file
