# Network File System (NFS)

## Overview
This NFS implementation consists of three major components: 
1. **Clients**: Interact with the NFS to perform file operations.
2. **Naming Server (NM)**: Acts as a central hub, coordinating communication between clients and storage servers.
3. **Storage Servers (SS)**: Handle the physical storage, retrieval of files, and ensure data persistence and distribution across the network.

Clients can either interact directly with the Storage Servers or via the Naming Server, which maps client requests to the appropriate storage resources.

---

## Major Components

### 1. Initializing Naming Server
- The **Naming Server (NM)** serves as the central coordination point in NFS. The port for the NM is assumed to be constant.
- It is responsible for mapping client requests to the correct Storage Server for execution.

### 2. Initializing Storage Servers
- **Storage Servers (SS)** are initialized by providing accessible paths as user input. 
- Details of each SS (e.g., Port Number, IP Address, Accessible Paths, Replication Port) are registered with the NM and stored in a `Trie` for efficient searching.
- New Storage Servers can be added dynamically by executing the SS file and assigning a unique port for each new SS.

### 3. Commands Issued by Naming Server to Storage Servers
The NM can issue the following file-related commands to SS:
- **CREATE**: Creates a file or directory.
- **DELETE**: Deletes a file or directory.
- **COPY**: Copies files or directories between storage servers.

### 4. Commands Issued by Clients to Storage Servers
- For **READ**, **WRITE**, and **FILEINFO** commands, clients interact directly with the Storage Servers.
- For commands like **CREATE**, **DELETE**, and **COPY**, the client sends the request to the NM. The NM determines the correct SS, forwards the command, and waits for an acknowledgment (ACK) from the SS, which is then passed back to the client.
- In case of errors during command execution, the client will display the appropriate error code.

---

## Multiple Clients
- The NFS design supports multiple clients connecting to the NM simultaneously.
- **Threading** is used to handle multiple client connections efficiently.
- Only one client can write to a file at any given time. This is enforced using a `write_lock` to prevent concurrent write operations.
- While a client is writing, other clients are prevented from reading the file by using a `write_flag`. The flag is set to `1` when writing starts and set to `0` when writing stops, ensuring consistency between read and write operations.

---

## Error Codes
- A variety of error codes are implemented to communicate issues between clients, the NM, and SS components.
- These codes are used to enhance clarity and help in troubleshooting potential problems, ensuring smooth operation and better debugging.

---

## Efficient Search in Naming Server
- **Trie Structure**: To speed up search operations when looking for SS details, a `Trie` is used in the NM. This improves search efficiency compared to linear searches.
- **LRU Cache**: The NM also implements an **LRU (Least Recently Used)** cache using linked lists. 
   - When a client requests data from a particular SS, the system first searches the LRU cache.
   - If the SS is found in the cache, it is moved to the front of the linked list.
   - If not found, the system searches the `Trie`, and the SS is then added to the LRU cache.

---

## Redundancy and Replication
- **Replicated Storage Servers**: After initialization, storage servers are replicated.
- **Synchronization**: Every change made to a storage server is replicated across all other replicas to ensure data consistency.
- **Failover**: If a storage server disconnects, the NM automatically connects to its replica to maintain service availability.
- **Reconnection**: If the original storage server reconnects, the NM will switch back from the replica to the original server.

---

## Bookkeeping
- Every port connection and communication event is logged for auditing purposes.
- After each SS is initialized, its details (e.g., port number, paths, etc.) are written to a file.
- The NM logs all client requests, and after each command is executed, the result is recorded in a status log.
- This bookkeeping ensures traceability and helps in monitoring and debugging the system.

---

## Assumptions
- The IP addresses for the NM and SS are assumed to be constant.
- Each Storage Server is assigned a unique client port number, provided as a command-line argument during initialization.

