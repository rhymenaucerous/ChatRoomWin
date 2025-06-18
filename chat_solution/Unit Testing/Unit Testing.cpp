#include "CppUnitTest.h"

extern "C"
{
// Project libraries
#include "../networking/networking.h"
#include "../hashtable/hashtable.h"
#include "../linkedlist/linkedlist.h"
}

// Global BOOL for this client's state
volatile BOOL g_bClientState = CONTINUE;

// Global HANDLE for this server's state
HANDLE g_hShutdown = NULL;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ModularLibraryTesting
{
TEST_CLASS(LinkedListTest){public : TEST_METHOD(
    DeletewithNodes) // Test The successful creation and deletion of a list.
                           {PLINKEDLIST LinkedList = NULL;
LinkedListInit(&LinkedList);
Assert::IsNotNull(LinkedList);

WORD wValue[3] = {1, 2, 3};

LinkedListInsert(LinkedList, wValue, 0);
LinkedListInsert(LinkedList, wValue, 0);
LinkedListInsert(LinkedList, wValue, 0);

WORD wResult = LinkedListDestroy(LinkedList, NULL);
Assert::AreEqual((WORD)EXIT_SUCCESS, wResult);
} // namespace ModularLibraryTesting
TEST_METHOD(RemoveNodes)
{
    // Test safe insertion and removal of nodes.
    PLINKEDLIST LinkedList = NULL;
    LinkedListInit(&LinkedList);
    Assert::IsNotNull(LinkedList);

    WORD wValue[3] = {1, 2, 3};

    LinkedListInsert(LinkedList, wValue, 0);
    LinkedListInsert(LinkedList, wValue, 0);
    LinkedListInsert(LinkedList, wValue, 0);

    LinkedListRemove(LinkedList, 0, NULL);
    LinkedListRemove(LinkedList, 1, NULL);
    LinkedListRemove(LinkedList, 0, NULL);

    WORD wResult = LinkedListDestroy(LinkedList, NULL);
    Assert::AreEqual((WORD)EXIT_SUCCESS, wResult);
} // TEST_METHOD(RemoveNodes)
TEST_METHOD(ReturnNodes)
{
    // Test return functions in linked list library.
    PLINKEDLIST LinkedList = NULL;
    LinkedListInit(&LinkedList);
    Assert::IsNotNull(LinkedList);

    WORD wValue[3] = {1, 2, 3};

    LinkedListInsert(LinkedList, wValue, 0);       // 1 inserted first
    LinkedListInsert(LinkedList, (wValue + 1), 0); // 2 inserted at 0
    LinkedListInsert(LinkedList, (wValue + 2), 2); // 3 inserted at last

    PWORD value1 = (WORD *)LinkedListReturn(LinkedList, 0);
    Assert::AreEqual(wValue[1], *value1); // should be 2
    PWORD value2 = (WORD *)LinkedListReturn(LinkedList, 2);
    Assert::AreEqual(wValue[2], *value2); // should be 3
    PWORD value3 = (WORD *)LinkedListReturn(LinkedList, 1);
    Assert::AreEqual(wValue[0], *value3); // should be 0

    WORD wResult = LinkedListDestroy(LinkedList, NULL);
    Assert::AreEqual((WORD)EXIT_SUCCESS, wResult);
} // TEST_METHOD(ReturnNodes)
TEST_METHOD(FreeFn)
{
    PWORD wValue =
        (PWORD)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WORD));
    Assert::IsNotNull(wValue);

    PLINKEDLIST LinkedList = NULL;
    LinkedListInit(&LinkedList);
    Assert::IsNotNull(LinkedList);

    WORD wResult = LinkedListDestroy(LinkedList, free);
    Assert::AreEqual((WORD)EXIT_SUCCESS, wResult);
} // TEST_METHOD(FreeFn)
} // TEST_CLASS(LinkedListTest)
;
TEST_CLASS(HashTableTest){public :
                              TEST_METHOD(PrimeTest){Assert::IsTrue(IsPrime(7));
Assert::IsTrue(IsPrime(11003));
Assert::IsFalse(IsPrime(65432));
// 65521 is the highest prime within 16-bit unsigned ints
Assert::IsTrue(IsPrime(60013));
Assert::IsTrue(IsPrime(47017));
Assert::IsTrue(IsPrime(55001));
Assert::IsTrue(IsPrime(46807));
Assert::AreEqual((WORD)11, NextPrime(7));
} // TEST_METHOD(PrimeTest)
TEST_METHOD(HashInitDestroy)
{
    WORD       wInitCapacity = 5;
    HASHTABLE *pHashTable    = NULL;
    Assert::AreEqual((int)SUCCESS,
                     (int)HashTableInit(&pHashTable, wInitCapacity, NULL));
    Assert::IsNotNull(pHashTable);

    Assert::AreEqual((int)SUCCESS, (int)HashTableDestroy(pHashTable, NULL));
} // TEST_METHOD(HashInitDestroy)
TEST_METHOD(HashAddEntries)
{
    WORD       wInitCapacity = 5;
    HASHTABLE *pHashTable    = NULL;
    Assert::AreEqual((int)SUCCESS,
                     (int)HashTableInit(&pHashTable, wInitCapacity, NULL));
    Assert::IsNotNull(pHashTable);

    WORD wValue[3] = {1, 2, 3};
    WORD wCheck    = EXIT_SUCCESS;
    WORD wCheck2   = EXIT_FAILURE;

    Assert::AreEqual(wCheck,
                     (WORD)HashTableNewEntry(pHashTable, wValue, "yeet", 4));
    Assert::AreEqual(
        wCheck, (WORD)HashTableNewEntry(pHashTable, (wValue + 1), "yeet1", 5));
    Assert::AreEqual(
        wCheck, (WORD)HashTableNewEntry(pHashTable, (wValue + 2), "yeet2", 5));
    Assert::AreEqual(wCheck2,
                     (WORD)HashTableNewEntry(
                         pHashTable, (wValue + 2), "yeet2",
                         5)); // Check for duplicate keys getting rejected

    PWORD piICheckTwo  = (PWORD)wValue;
    PWORD piIResultTwo = (PWORD)HashTableReturnEntry(pHashTable, "yeet", 4);

    WORD iICheckTwo  = *piICheckTwo;
    WORD iIResultTwo = *piIResultTwo;

    Assert::AreEqual(iICheckTwo, iIResultTwo);

    Assert::AreEqual((int)SUCCESS, (int)HashTableDestroy(pHashTable, NULL));
} // TEST_METHOD(HashAddEntries)
TEST_METHOD(LargeCapacity)
{
    WORD       wInitCapacity = 65534;
    HASHTABLE *pHashTable    = NULL;
    Assert::AreEqual((int)SUCCESS,
                     (int)HashTableInit(&pHashTable, wInitCapacity, NULL));
    Assert::IsNotNull(pHashTable);

    Assert::AreEqual((int)SUCCESS, (int)HashTableDestroy(pHashTable, NULL));
} // TEST_METHOD(LargeCapacity)
TEST_METHOD(ReHash)
{
    WORD       wInitCapacity = 5;
    HASHTABLE *pHashTable    = NULL;
    Assert::AreEqual((int)SUCCESS,
                     (int)HashTableInit(&pHashTable, wInitCapacity, NULL));
    Assert::IsNotNull(pHashTable);

    WORD wValue[3] = {1, 2, 3};
    WORD wCheck    = EXIT_SUCCESS;
    WORD wCheck2   = EXIT_FAILURE;

    Assert::AreEqual(wCheck,
                     (WORD)HashTableNewEntry(pHashTable, wValue, "1", 1));
    Assert::AreEqual(wCheck,
                     (WORD)HashTableNewEntry(pHashTable, wValue, "2", 1));
    Assert::AreEqual(wCheck,
                     (WORD)HashTableNewEntry(pHashTable, wValue, "3", 1));
    Assert::AreEqual(wCheck,
                     (WORD)HashTableNewEntry(pHashTable, wValue, "4", 1));
    Assert::AreEqual(wCheck,
                     (WORD)HashTableNewEntry(pHashTable, wValue, "5", 1));
    Assert::AreEqual(wCheck,
                     (WORD)HashTableNewEntry(pHashTable, wValue, "6", 1));
    Assert::AreEqual(wCheck,
                     (WORD)HashTableNewEntry(pHashTable, wValue, "7", 1));
    Assert::AreEqual(wCheck,
                     (WORD)HashTableNewEntry(pHashTable, wValue, "8", 1));
    Assert::AreEqual(wCheck,
                     (WORD)HashTableNewEntry(pHashTable, wValue, "9", 1));
    Assert::AreEqual(wCheck,
                     (WORD)HashTableNewEntry(pHashTable, wValue, "10", 2));
    Assert::AreEqual(wCheck,
                     (WORD)HashTableNewEntry(pHashTable, wValue, "11", 2));
    Assert::AreEqual(wCheck,
                     (WORD)HashTableNewEntry(pHashTable, wValue, "12", 2));

    Assert::AreEqual((int)SUCCESS, (int)HashTableDestroy(pHashTable, NULL));
} // TEST_METHOD(ReHash)
} // TEST_CLASS(HashTableTest)
;

TEST_CLASS(NetworkTest){public : TEST_METHOD(EasyConnect){
    Assert::AreEqual((int)SUCCESS, (int)NetSetUp());

SOCKET ListenSocket = NetListen(L"127.0.0.1", L"8080");
Assert::AreNotEqual(INVALID_SOCKET, ListenSocket);

SOCKET ClientSocket = ListenSocket;

// Create thread pool work for accepting connections
PTP_WORK acceptWork = CreateThreadpoolWork(WorkCallback, &ClientSocket, NULL);

// Submit the work to the thread pool
SubmitThreadpoolWork(acceptWork);

// Connect from the client side
SOCKET ServerSocket = NetConnect(L"127.0.0.1", L"8080");
Assert::AreNotEqual(INVALID_SOCKET, ServerSocket);

// Wait for the thread pool work to complete
WaitForThreadpoolWorkCallbacks(acceptWork, FALSE);

// Verify the connection was accepted
Assert::AreNotEqual(INVALID_SOCKET, ClientSocket);

// Clean up
CloseThreadpoolWork(acceptWork);
NetCleanup(ListenSocket, ClientSocket);
closesocket(ServerSocket);
} // TEST_METHOD(EasyConnect)
public:
TEST_METHOD(IPV6)
{
    Assert::AreEqual((int)SUCCESS, (int)NetSetUp());

    SOCKET ListenSocket = NetListen(L"::1", L"8080");
    Assert::AreNotEqual(INVALID_SOCKET, ListenSocket);

    SOCKET ClientSocket = ListenSocket;

    // Create thread pool work for accepting connections
    PTP_WORK acceptWork =
        CreateThreadpoolWork(WorkCallback, &ClientSocket, NULL);

    // Submit the work to the thread pool
    SubmitThreadpoolWork(acceptWork);

    // Connect from the client side
    SOCKET ServerSocket = NetConnect(L"::1", L"8080");
    Assert::AreNotEqual(INVALID_SOCKET, ServerSocket);

    // Wait for the thread pool work to complete
    WaitForThreadpoolWorkCallbacks(acceptWork, FALSE);

    // Verify the connection was accepted
    Assert::AreNotEqual(INVALID_SOCKET, ClientSocket);

    // Clean up
    CloseThreadpoolWork(acceptWork);
    NetCleanup(ListenSocket, ClientSocket);
    closesocket(ServerSocket);
} // TEST_METHOD(IPV6)
public:
TEST_METHOD(ConnectClosedAddr)
{
    Assert::AreEqual((int)SUCCESS, (int)NetSetUp());
    Assert::AreEqual((SOCKET)INVALID_SOCKET,
                     (SOCKET)NetConnect(L"::1", L"8080"));
} // TEST_METHOD(IPV6)
} // TEST_CLASS(NetworkTest)
;
} // namespace ModularLibraryTesting
