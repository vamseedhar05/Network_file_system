# Network File System.

## Overview
- This NFS implementation consists of three major components: Clients, Naming Server, and Storage Servers.
- Clients interact with the NFS to perform various file-related operations, while the Naming Server acts as a central hub for coordinating communication between clients and storage servers. 
- Storage Servers handle the physical storage and retrieval of files, ensuring data persistence and distribution across the network.
- Clients can also interact directly with the Storage Server

## Major Components
### Initialising Naming Server
- We first initialise the Naming Server , which serves as central coordination point in NFS.The port for Naming Server is assumed to be constant.
- The Naming Server is responsible for mapping client requests to the appropriate Storage Server.

### Initialising Storage Server
- All the Storage Servers are initialised and accessable paths for each Storage Server are given as user input.The details of the SS are sent to the NM which are stored in `Trie` for faster Search operation.
- The details of SS include `Port Number` , `Path` , `ipAddress` , `accessable paths` , `No of accessable paths` ,`replicable port number`. 
- We can add a new SS at any point of time by executing the SS file in the respective directory and define a unique port for the new SS.

### Commands By NM to SS:
- NM can issue Create / Delete / Copy files or directories.
- Create : execute the 

### Commands By Clients to SS
- If the commands are READ , WRITE or FILEINFO then the command is executed directly by the SS.
- In the above case the Client sends the request to the NM and NM sends the correct SS ip & port number to which the CLient connects and waits for execution of the command and waits for the ACK from the SS.
- If the commands are CREATE , DELETE or COPY then the command is sent to NM . Once the NM determines the correct SS the command is forwarded to the correct SS and waits for the acknowledgement.
- After the execution of the command SS sends an ACK to the NM which in turn sends it to the Client providing it the status of the task.
- Error code is printed in the Client if there is any error while performing the command.

### Multiple Clients 
- NFS design accomodates multiple clients attempting to connect to NM simultaneously.
- We can achieve this using threads and  an initial and final ACK from the relevant SS are employed.
- Only 1 Client can write at a time in a file so in order to achieve this we use a write_lock to avoid WRITE by multiple Clients.
- Also while 1 client is writing no other Client can read the file so here we use the flag `write_flag`.Make it 1 when we start writing and 0 when writing to file stops.We don't read the file when `write_flag` is 1.

### Error Codes
- Various error codes are employed within the NFS system to communicate issues and exceptional scenarios between clients and the NFS components (Naming Server, Storage Servers). 
- These error codes enhance the clarity of error messages and assist in troubleshooting potential problems.

### Search in NM
- In order to reduce the time complexity of the Search operations while inserting the details of SS we are inserting them into `Trie` which has faster Search than Linear Searches.
- Also we have implemented `LRU` cache in NM using `Linked Lists`.When Client requests data from a particular `SS` we search first in the `LRU` cache.
- If `SS` is present in `LRU` then it's brought to the front of the `Linked List` , if it's not present in `LRU` then it searches in the `Trie` and also inserts into the `LRU` at the front.

### Redundancy / Replication:
- Replicated storage servers after every initialisation.
- Updated every change that happened to storage server to every other replicated servers.
- if any one of the storage server is disconnected then the naming server tries to connect with the replicated servers.
- if the storage server connects again, then naming server reconnects to the storage server by disconnecting to its replicas.


### Bookkeeping
- Each and every port connection is stored in the `log`.
- After each SS is initialised the details of the `SS` are writen to the file.
- After the Client sends for request NM records the data sent by the Client.
- After the execution of the command , the Status of the command is recorded into the file.


## ASSUMPTIONS:
- ip to be constant for NM and SS.
- For each Storage Server unique Client Port number is given as command line argument .

