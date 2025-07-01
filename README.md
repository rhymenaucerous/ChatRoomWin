# ChatRoom
### Version 1.0.0

![alt text](README_Folder/Images/WinChatRoom.gif)

*Figure 1. Chat Server Sample.*

# 1. Project Overview

The Chat Server provides a primitive chat server that allows for private messaging or broadcasts. It is no secure via authentication or confidentiality of network data. The purpose of the server is to demonstrate WinAPI utilization and also have a fun chat server. The server can be access be either the GUI or the CLI.

The server uses IO completion ports (IOCP) for sending and receiving packets from the client. This made the logic a little more complicated, as it's essential for a user to be able to asychronously receive messages while the client is sending messages using shared memory. The second diagram demonstrates some of the server's logic. The third diagram expands upon this logic.

The fourth figure, below, just describes about how the readers and writers interact with the users hash table. The interaction enables multiple readers - which support the message, broadcast, and list functionalities whil only supporting one writer at a time - for the register/login and logout functionalities.

![alt text](README_Folder/Images/ChatServerV1.png)

*Figure 2. Chat Server Overview Flowchart.*

![alt text](README_Folder/Images/IOCP_Srv_Logic.png)

*Figure 3. Chat Server Logic part 1.*

![alt text](README_Folder/Images/managequeueaddlogic.png)

*Figure 4. Chat Server Logic part 2.*

![alt text](README_Folder/Images/writer_reader_interactions.png)

*Figure 5. User Hash Table Reader/Writer Logic.*



# 2. Message protocols

The following message protocols are used for chat room communications. Most messages have opcodes request (to the server) and acknowledge/reject (to the client), but the packet type and subtype identify what communications are occuring. If the packet is of type failure, the packet will also include a reject code. 

### 2.1 Packet type codes 

|||
|-|-|
|Account|0x00|
|Chat|0x01|
|List|0x02|
|List|0x03|
|Failure|0xFF|

<br>

### 2.2 Sub-types: 
|||
|-|-|
|Empty|0x00|
|Login|0x01|
|Logout|0x02|

<br>

### 2.3 Opcodes: 
|||
|-|-|
|Request|0x00|
|Response|0x01|
|Acknowledge|0x02|

<br>

### 2.4 Reject codes: 
||||
|-|-|-|
|Server Busy|0x00|Server is unable to take anymore clients|
|Server Error|0x01|An error has occured on the server|
|Invalid Packet|0x02|The server received an invalid packet|
|Username length|0x03|The username length is not in the range 1 to 30 characters|
|User logged in|0x04|Username unavailable|
|User not exist|0x05|The specified user does not exist|
|message length|0x06|The message is too long|
|server full|0x07|The server has reached max capacity|

<br>

# 3. Testing

Integration testing was manually, via the command line. Unit testing for modular libraries in solution.

<br>

# 4. Product Backlog

1. Chats are not persistant
2. Packet protocol dictates list length meet message length requirements. The limitation is currently the size of WORD, meaning that the list cannot be more than 65535 characters in length. This means that the set limit of 65535 maximum clients would certainly not fit. Not to mention the practice limitations of each login/logout broadcasting.
3. Clients control the number of sends performed on an IOCP server because packets return on completion. The server could build up a queue of messages for a client that is staying connected but not receiving messages which could eventually take up a lot of memory on the server. A due out is to limit that number of messages.
4. Need to fuzz the server to find any bugs/segfaults.
5. Update reader/writer synchronization to use built-in windows reader/writers.
6. Update sends to use encryption library.
7. Add unit tests to pipeline.
8. Create automated tests for server/client interactions.
