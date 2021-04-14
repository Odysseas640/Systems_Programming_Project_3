# Systems_Programming_Project_3
Contains multiple processes, multithreading, pipes and networking/sockets. Built on top of the 2nd assignment.

This project is meant to be run in 3 different computers within a local network.
In one of the computers, there are "worker" processes, that read some files each and save the data in structures in the RAM. Then they connect to the "server" process in another computer, and report in short what data they have. The server is multithreaded, and each thread handles one incoming connection.
In the third computer, there is a multithreaded "client" process. The client reads some queries from a file, and each of its threads tries to connect to the server to send its query. The server's threads receive the queries, and forward them to the correct worker. The worker answers to the server, and the serverforwards the response to the client.

I ran it with 100+ threads for the client and the server, and it worked fine.
