# Server
The server is compiled by running 
```make server``` 
It compiles on macOS and linux

# Client
The client is compiled by running
```make client```
To compile on macOS the makefile must be edited and the lncurses flag must be added behind ```client: $(CLIENT)```

# What was finished and what not
Due to lack of understanding on the project along with what we think are very unclear and confusing instructions
we did not have time enough to implement it fully. It took way to long to get started in some way and that sucks.

What we have is a server that can connect to other servers and get connected to by other servers. Upon connection it sends a CMD
requesting the ID of the newly connected server. With the ID request the server sends it's port so that the other server can update
it's serverlist structure. After connection is established between our server and maximum 5 other servers. The C&C can issue LISTSERVERS,FETCH,ID and all other P2 commands to our server, it can also communicate to other servers via CMD and RSP but only for ID,LISTSERVERS and FETCH so far. Also when a CMD or RSP is received that is not to us, we check if it is for a neighbor and then forward it there, else it gets lost. Other servers can send us KEEPALIVE and we send other servers KEEPALIVE as well. 

The next step would have to build a routing table, that would be done automatic in a scheduled way. We would ping each neighbor with the
LISTSERVERS command and for each of their neighbors we have not already information about, we would ping them also and so forth and so on.
Until we would have all the information we need. But there was not time to implement this. Also we did not implement the bytestuffing since we ran out of time but we understand the process 100%. We scan each byte we send and if we see a SOH or EOT token we stuff an identical token before it and keep on going. When we unpack the buffer we first check if we have a SOH token at the start and an EOT at the end. We then scan each byte and when we see two consecutive SOH/EOT tokens we remove one of them and keep going.

# Version Control
We used github for version control, each small functionality was tested and commited/pushed to git.



