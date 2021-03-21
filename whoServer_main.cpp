#include <iostream>
#include <cstring>
#include <string>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/types.h>	     /* sockets */
#include <sys/socket.h>	     /* sockets */
#include <netinet/in.h>	     /* internet sockets */
#include <netdb.h>	         /* gethostbyaddr */
#include <ctype.h>	         /* toupper */
#include <fstream>
#include <fcntl.h>
#include <signal.h>
#include "circular_buffer.h"
#include "lists.h"
#include <arpa/inet.h>
using namespace std;
#include <errno.h>
extern int errno;
pthread_mutex_t circular_buffer_mutex;
pthread_mutex_t printing_mutex;
pthread_mutex_t worker_info_list_mutex;
pthread_mutex_t fd_mutex;
pthread_cond_t bufer_not_empty;
pthread_cond_t bufer_not_full;
struct hostent *rem_worker, *rem_client;

char* workers_ip;
char* clients_ip;
pthread_mutex_t workers_ip_mutex;
pthread_mutex_t clients_ip_mutex;

struct handling_thread_arguments {
	circular_buffer* bufer;
	WorkerInfoListNode** worker_info_list;
	// int thread_index; // This is largely diagnostic, and also more understandable than a thread ID
};
void* handle_connection(void* thread_args); // numThreads of these: Takes an FD and type from the buffer. If it's a worker, read summary and put worker's port in dynamic array (may need realloc, by, say, 10 spots each time)

void mutex_print(char* char_string) {
	pthread_mutex_lock(&printing_mutex);
	cout << char_string << endl;
	pthread_mutex_unlock(&printing_mutex);
}

void* handle_connection(void* thread_args) {
	struct handling_thread_arguments *argz = (struct handling_thread_arguments*) thread_args;
	// cout << "Handle connections thread" << endl;
	int n;
	while (1) { // Reusable thread
		pthread_mutex_lock(&circular_buffer_mutex);
		while (argz->bufer->items <= 0) {
			pthread_cond_wait(&bufer_not_empty, &circular_buffer_mutex);
		}
		int fd_from_buffer;
		char type;
		buf_pop(argz->bufer, fd_from_buffer, type);
		pthread_mutex_unlock(&circular_buffer_mutex);
		pthread_cond_signal(&bufer_not_full);

		char buf[256];
		memset(buf, 0, 256);
		char ok_response[10];
		strcpy(ok_response, "OKres");
		char transfer_size[10];
		memset(transfer_size, '\0', 10);
		if (type == 'c') {
			// READ CLIENT QUERY
			// cout << "Handling client FD" << endl;
			read(fd_from_buffer, transfer_size, 10);
			n = read(fd_from_buffer, buf, atoi(transfer_size) + 1);
			if (n < 1) {
				strcpy(buf, "!Bad file descriptor error again");
				write(fd_from_buffer, "33", 3);
				write(fd_from_buffer, buf, 33);
				continue;
			}
			pthread_mutex_lock(&printing_mutex);
			cout << "Server received query: " << buf << endl;
			pthread_mutex_unlock(&printing_mutex);

			int worker_port_we_want = -2; // Doesn't matter, just something negative
			// Look through query to find out if it has a country
			pthread_mutex_t* mutex_ptr = NULL;
			if (strncmp(buf, "/diseaseFrequency", 17) == 0 || strncmp(buf, "/numPatientAdmissions", 21) == 0 || strncmp(buf, "/numPatientDischarges", 21) == 0) {
				// If query contains a country, look for that country in the worker info list.
				// Extract the last argument of the client's query, which is supposed to be a country, and look for it in the worker info list.
				char buf2[100];
				strcpy(buf2, buf); // strtok_r messes up buf
				char* saveptr = buf2;
				char query_tokens[5][33];
				for (int i = 0; i < 5; ++i) {
					if (strcmp(saveptr, "") != 0 && saveptr[0] != ' ') // Ignore any spaces ' ' in a row
						strcpy(query_tokens[i], strtok_r(saveptr, " ", &saveptr));
					else
						query_tokens[i][0] = '\0'; // Stop if there's no more arguments
				}
				if (strncmp(query_tokens[4], " ", 1) != 0 && query_tokens[4][0] != '\0') {
					// pthread_mutex_lock(&worker_info_list_mutex); // Keep another thread from modifying this data and messing it up halfway through
					worker_port_we_want = get_port_of_worker_that_has_country(*argz->worker_info_list, query_tokens[4], mutex_ptr);
					// pthread_mutex_unlock(&worker_info_list_mutex);
				} // If no worker is found with this country, it'll be -1
			}
			int worker_fd;
		    struct sockaddr_in server;
		    struct sockaddr *serverptr = (struct sockaddr*)&server;
		    // struct hostent *rem;
		    pthread_mutex_lock(&fd_mutex);
		    if ((worker_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) // Create socket
		    	perror("socket");
		    pthread_mutex_unlock(&fd_mutex);
		    // cout << "Connecting to worker at " << workers_ip << endl;
		    // if (rem_worker == NULL) {
		    	pthread_mutex_lock(&workers_ip_mutex);
			    if ((rem_worker = gethostbyname(workers_ip)) == NULL) { // Find (worker who's acting as a) server address
				   herror("gethostbyname");
				   exit(1);
			    }
			// }
		    server.sin_family = AF_INET;       // Internet domain
		    memcpy(&server.sin_addr, rem_worker->h_addr, rem_worker->h_length);
		    	pthread_mutex_unlock(&workers_ip_mutex);
			if (worker_port_we_want >= 0) {
				// CONNECT TO WORKER
			    server.sin_port = htons(worker_port_we_want);
			    pthread_mutex_lock(mutex_ptr); // Lock this worker's mutex
			    if (connect(worker_fd, serverptr, sizeof(server)) < 0) {
				   perror("----connect 1");
				   exit(1);
			    }
			    // SEND QUERY TO WORKER
				sprintf(transfer_size, "%d", (int) strlen(buf));
				write(worker_fd, transfer_size, 10);
				write(worker_fd, buf, strlen(buf) + 1);

				// RECEIVE ANSWER FROM WORKER
				char response[500];
				read(worker_fd, transfer_size, 10);
				read(worker_fd, response, atoi(transfer_size) + 1);
				close(worker_fd); // Done with this worker
				pthread_mutex_unlock(mutex_ptr);

				pthread_mutex_lock(&printing_mutex);
				cout << "Sending response to client: " << endl << response << endl;
				pthread_mutex_unlock(&printing_mutex);
				sprintf(transfer_size, "%d", (int) strlen(response) + 1);
				write(fd_from_buffer, transfer_size, 10);
				write(fd_from_buffer, response, ((int) strlen(response)) + 1);
			}
			else { // If query doesn't have a country
				// CONNECT TO WORKER
			    WorkerInfoListNode* current_node = *argz->worker_info_list;
				int total_cases = 0;
				char send_this_to_client[500];
				memset(send_this_to_client, '_', 500);
				char this_workers_response[500];
			    while (current_node != NULL) {
			    	if (current_node != *argz->worker_info_list) { // Second+ iteration, create socket again, otherwise it doesn't work.
			    		pthread_mutex_lock(&fd_mutex);
			    		if ((worker_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) { // Create socket
		    				perror("--------socket 2");
		    				exit(1);
		    			}
			    		pthread_mutex_unlock(&fd_mutex);
			    	}
				    pthread_mutex_lock(&current_node->worker_mutex); // Only one thread should send to the same worker
				    server.sin_port = htons(current_node->port); // Server port
				    if (connect(worker_fd, serverptr, sizeof(server)) < 0) { // Initiate connection
						perror("--------connect 3");
						exit(1);
					}
				    // Send query to current worker in the list
					sprintf(transfer_size, "%d", (int) strlen(buf));
					write(worker_fd, transfer_size, 10);
					write(worker_fd, buf, strlen(buf) + 1);
				    // Read this worker's response
					read(worker_fd, transfer_size, 10);
					read(worker_fd, this_workers_response, atoi(transfer_size) + 1);
					close(worker_fd);
				    pthread_mutex_unlock(&current_node->worker_mutex);

					if (strncmp(buf, "/diseaseFrequency", 17) == 0 && this_workers_response[0] != '!') { // This query's answer is just numbers, so add them up
						if (this_workers_response[0] >= '0' && this_workers_response[0] <= '9') {
							total_cases += atoi(this_workers_response);
						}
						if (current_node->Next == NULL) { // Last worker, copy total cases to the string we're about to send to the client
							sprintf(send_this_to_client, "%d", total_cases);
						}
					}
					else if ((strncmp(buf, "/numPatientAdmissions", 21) == 0 || strncmp(buf, "/numPatientDischarges", 21) == 0) && this_workers_response[0] != '!') {
						if (send_this_to_client[0] == '_' || send_this_to_client[0] == '!')
							strcpy(send_this_to_client, this_workers_response);
						else
							strcat(send_this_to_client, this_workers_response);
					}
					else if (this_workers_response[0] == '!') { // If worker didn't find a result
						if (send_this_to_client[0] == '_') { // If this is the first answer we got, save it, it's better than nothing
							strcpy(send_this_to_client, this_workers_response);
						}
					}
					else { // We got a proper answer for a query other than the above
						strcpy(send_this_to_client, this_workers_response);
					}
					// cout << "SERVER DONE WITH A QUERY" << endl;
					current_node = current_node->Next; // Next worker in worker_info_list
				}
				// SEND WORKER RESPONSE TO CLIENT THAT ASKED
				pthread_mutex_lock(&printing_mutex);
				if (send_this_to_client[0] == '_') {
					strcpy(send_this_to_client, "No worker is connected!");
				}
				cout << "Sending response to client: " << send_this_to_client << endl;
				pthread_mutex_unlock(&printing_mutex);
				sprintf(transfer_size, "%d", ((int) strlen(send_this_to_client)) + 1);
				write(fd_from_buffer, transfer_size, 10);
				write(fd_from_buffer, send_this_to_client, ((int) strlen(send_this_to_client)) + 1);

				if (strncmp(send_this_to_client, "exiting", 7) == 0) { // If client sent /exit as in the 2nd assignment, workers have quit, so delete their info
					pthread_mutex_lock(&printing_mutex);
					cout << "Deleting worker info list" << endl;
					pthread_mutex_unlock(&printing_mutex);
					delete_worker_info_list(*argz->worker_info_list);
				}
			}
		}
		else if (type == 'w') { // This file descriptor is from a worker
			// Receive worker's port and save it in the workers info list
			char workerPort[10];
			read(fd_from_buffer, workerPort, 10);
			write(fd_from_buffer, ok_response, 6);

			// Receive countries, all in a char array separated by ' ', and insert them in the worker info list along with the worker's port
			read(fd_from_buffer, transfer_size, 10); // READ TRANSFER SIZE
			write(fd_from_buffer, ok_response, 6); // SEND 'RECEIVED SIZE OK'
			int response_size_int = atoi(transfer_size);
			int write_index = 0;
			int receive_size = 100; // SOCKET BUFFER
			do {
				if (write_index + receive_size > response_size_int + 1)
					receive_size = response_size_int + 1 - write_index;
				read(fd_from_buffer, buf + write_index, receive_size);
				write_index += receive_size;
				buf[write_index] = '\0';
				write(fd_from_buffer, ok_response, 6);
			} while (write_index < response_size_int + 1);

			pthread_mutex_lock(&worker_info_list_mutex);
			insert_to_worker_info_list(*argz->worker_info_list, atoi(workerPort), buf); // Save worker's info so we can send him queries later
			pthread_mutex_unlock(&worker_info_list_mutex);
			// print_worker_info_list(*argz->worker_info_list);

			// Receive summary and print it with a printing mutex
			do {
				read(fd_from_buffer, transfer_size, 10);
				if (strncmp(transfer_size, "#end#", 5) == 0)
					break; // NO MORE SUMMARY TO READ
				write(fd_from_buffer, ok_response, 6); // SEND 'SUMMARY SIZE OK'
				int response_size_int = atoi(transfer_size);
				int write_index = 0;
				int receive_size = 100; // SOCKET BUFFER
				do {
					if (write_index + receive_size > response_size_int + 1)
						receive_size = response_size_int + 1 - write_index;
					read(fd_from_buffer, buf + write_index, receive_size);
					write_index += receive_size;
					buf[write_index] = '\0';
					write(fd_from_buffer, ok_response, 6);
				} while (write_index < response_size_int + 1);
				read(fd_from_buffer, ok_response, 6);
				mutex_print(buf); // Print buffer
			} while (1);
		}
		close(fd_from_buffer);
	}
	return NULL;
}

int main(int argc, char const *argv[]) {
	pthread_mutex_init(&circular_buffer_mutex, 0);
	pthread_mutex_init(&printing_mutex, 0);
	pthread_mutex_init(&worker_info_list_mutex, 0);
	pthread_mutex_init(&workers_ip_mutex, 0);
	pthread_mutex_init(&clients_ip_mutex, 0);
	pthread_cond_init(&bufer_not_empty, 0); // These are from the professor's code
	pthread_cond_init(&bufer_not_full, 0);
	int queryPortNum = -1, statisticsPortNum = -1, numThreads = -1, bufferSize = -1;
	workers_ip = NULL;
	clients_ip = NULL;
	for (int i = 1; i < argc-1; i=i+2) {
		if (strcmp(argv[i],"-q") == 0) {
			for (int j = 0; j < (int) strlen(argv[i+1]); ++j) {
				if (argv[i+1][j] > '9' || argv[i+1][j] < '0') {
					cout << "queryPortNum is not a number. Terminating." << endl;
					return 1;
				}
			}
			queryPortNum = atoi(argv[i+1]); // CHECK IF THIS IS A NUMBER
		}
		else if (strcmp(argv[i],"-s") == 0) {
			for (int j = 0; j < (int) strlen(argv[i+1]); ++j) {
				if (argv[i+1][j] > '9' || argv[i+1][j] < '0') {
					cout << "statisticsPortNum is not a number. Terminating." << endl;
					return 1;
				}
			}
			statisticsPortNum = atoi(argv[i+1]); // CHECK IF THIS IS A NUMBER
		}
		else if (strcmp(argv[i],"-w") == 0) {
			for (int j = 0; j < (int) strlen(argv[i+1]); ++j) {
				if (argv[i+1][j] > '9' || argv[i+1][j] < '0') {
					cout << "numThreads is not a number. Terminating." << endl;
					return 1;
				}
			}
			numThreads = atoi(argv[i+1]); // CHECK IF THIS IS A NUMBER
		}
		else if (strcmp(argv[i],"-b") == 0) {
			for (int j = 0; j < (int) strlen(argv[i+1]); ++j) {
				if (argv[i+1][j] > '9' || argv[i+1][j] < '0') {
					cout << "bufferSize is not a number. Terminating." << endl;
					return 1;
				}
			}
			bufferSize = atoi(argv[i+1]); // CHECK IF THIS IS A NUMBER
		}
		else
			cout << "MISTAKE" << endl;
	}
	char hostname[100];
	memset(hostname, 0, 100);
	gethostname(hostname, sizeof(hostname));
	struct hostent* host_entry = gethostbyname(hostname);
	char* IPbuffer = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0])); // I took this line from GeeksforGeeks
	// cout << "Server's IP: " << IPbuffer << endl;
	// mutex_print(IPbuffer);
	cout << "Server's IP: " << IPbuffer << endl;

	if (numThreads <= 0 || queryPortNum <= 0 || bufferSize <= 0 || statisticsPortNum <= 0) {
		cout << "Could not find expected arguments. Terminating." << endl;
		return 1;
	}
	cout << "queryPortNum: " << queryPortNum << endl << "statisticsPortNum: " << statisticsPortNum << endl << "numThreads: " << numThreads << endl << "bufferSize: " << bufferSize << endl;
	///////////////////////////////////////////////////////////////////// I copied a few lines here and there from the professor's code
	// Each element in the circular buffer has an FD, AND a 'c' or 'w' character, so the thread that picks up the FD knows what to do with it.
	int retourn = 0;
	pthread_t* handling_threads = new pthread_t[numThreads];
	circular_buffer* bufer = NULL;
	WorkerInfoListNode* w_info_list = NULL;

	buf_initialize(bufer, bufferSize);
	struct handling_thread_arguments* handling_thread_args = new handling_thread_arguments[numThreads];

	for (int it = 0; it < numThreads; ++it) {
		handling_thread_args[it].bufer = bufer;
		handling_thread_args[it].worker_info_list = &w_info_list;
		// handling_thread_args[it].thread_index = it;
		if ((retourn = pthread_create(handling_threads+it, NULL, handle_connection, handling_thread_args+it)) != 0) { 
			perror("pthread_create");
			exit(1);
		}
	}
	///////////////////////////////////////////////////////////////////////////////
	int client_fd, worker_fd, sock_client, sock_worker;
	struct sockaddr_in client_server, worker_server, client;
    socklen_t clientlen;
    struct sockaddr *client_serverptr=(struct sockaddr *)&client_server; // For queryPortNum
    struct sockaddr *worker_serverptr=(struct sockaddr *)&worker_server; // For statisticsPortNum
    struct sockaddr *clientptr=(struct sockaddr *)&client;
    // struct hostent *rem;
    rem_worker = NULL;
    rem_client = NULL;
    pthread_mutex_lock(&fd_mutex);
    if ((sock_client = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        perror("socket");
    if ((sock_worker = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        perror("socket");
    pthread_mutex_unlock(&fd_mutex);
    client_server.sin_family = AF_INET;       /* Internet domain */
    client_server.sin_addr.s_addr = htonl(INADDR_ANY);
    client_server.sin_port = htons(queryPortNum);      /* The given port */
    // inet_pton(AF_INET, "127.72.66.123", &client_server.sin_addr);
    worker_server.sin_family = AF_INET;       /* Internet domain */
    worker_server.sin_addr.s_addr = htonl(INADDR_ANY);
    worker_server.sin_port = htons(statisticsPortNum);      /* The given port */
    // inet_pton(AF_INET, "127.72.66.123", &worker_server.sin_addr);
    /* Bind socket to address */
    if (bind(sock_client, client_serverptr, sizeof(client_server)) < 0) {
        perror("Clients' bind failed");
        delete[] handling_thread_args;
        delete[] handling_threads;
        exit(1);
    }
    if (bind(sock_worker, worker_serverptr, sizeof(worker_server)) < 0) {
        perror("Workers' bind failed");
        delete[] handling_thread_args;
        delete[] handling_threads;
        exit(1);
    }
    int opt = 1;
    if (setsockopt(sock_client, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { // I found this on geeksforgeeks
       	perror("setsockopt");
       	exit(EXIT_FAILURE);
   	}
   	opt = 1; // I don't know why I'm doing this, I still get an error within 30 seconds after I terminate the server.
    if (setsockopt(sock_worker, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { // I found this on geeksforgeeks
       	perror("setsockopt");
       	exit(EXIT_FAILURE);
   	}
    /* Listen for connections */
    if (listen(sock_client, 500) < 0)
    	perror("listen");
    pthread_mutex_lock(&printing_mutex);
    printf("Listening for client connections to port %d\n", ntohs(client_server.sin_port));
    pthread_mutex_unlock(&printing_mutex);
    if (listen(sock_worker, 500) < 0)
    	perror("listen");
    pthread_mutex_lock(&printing_mutex);
    printf("Listening for worker connections to port %d\n", ntohs(worker_server.sin_port));
    cout << "Ready for connections now." << endl;
    pthread_mutex_unlock(&printing_mutex);
	struct timeval tv1;
	int n_fds;
	clientlen = sizeof(client);
    while (1) {
    	fd_set two_sockets;
		FD_ZERO(&two_sockets);
		pthread_mutex_lock(&fd_mutex);
		FD_SET(sock_client, &two_sockets);
		FD_SET(sock_worker, &two_sockets);
		tv1.tv_sec = 65000; // Make this indefinite
		tv1.tv_usec = 42;
		n_fds = sock_client;
		if (sock_worker > n_fds)
			n_fds = sock_worker;
		n_fds++;
		pthread_mutex_unlock(&fd_mutex);
		// pthread_mutex_lock(&printing_mutex);
		// cout << "SELECTING... ";
		// pthread_mutex_unlock(&printing_mutex);
		select(n_fds, &two_sockets, NULL, NULL, &tv1);
		// pthread_mutex_lock(&printing_mutex);
		// cout << "...SELECTED" << endl;
		// pthread_mutex_unlock(&printing_mutex);
		if (FD_ISSET(sock_client, &two_sockets)) {
			pthread_mutex_lock(&fd_mutex);
	    	if ((client_fd = accept(sock_client, clientptr, &clientlen)) < 0) { // Accept connection
	    		perror("accept");
	    	}
	    	pthread_mutex_unlock(&fd_mutex);
	    	if (clients_ip == NULL) { // This was causing errors in the loop, so I set it to only execute once.
			   	if ((rem_client = gethostbyaddr((char *) &client.sin_addr.s_addr, sizeof(client.sin_addr.s_addr), client.sin_family)) == NULL) { // Find client's address
			  	    herror("gethostbyaddr1");
			  	    exit(1);
			  	}
			  	pthread_mutex_lock(&printing_mutex);
			   	printf("Accepted connection from client %s\n", rem_client->h_name);
			   	pthread_mutex_unlock(&printing_mutex);
			   	if (clients_ip == NULL) {
			   		pthread_mutex_lock(&clients_ip_mutex);
			   		clients_ip = new char[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &client.sin_addr, clients_ip, INET_ADDRSTRLEN);
					pthread_mutex_unlock(&clients_ip_mutex);
					pthread_mutex_lock(&printing_mutex);
			   		cout << "Client's IP: " << clients_ip << endl;
			   		pthread_mutex_unlock(&printing_mutex);
				}
			}
		   	// Now put it in the circular buffer
		   	pthread_mutex_lock(&circular_buffer_mutex); // Lock mutex
		   	while (bufer->items >= bufer->size) {
		   		pthread_cond_wait(&bufer_not_full, &circular_buffer_mutex);
		   	}
		   	buf_insert(bufer, client_fd, 'c');
		   	pthread_mutex_unlock(&circular_buffer_mutex); // Unlock mutex
		   	pthread_cond_signal(&bufer_not_empty);
		}
		if (FD_ISSET(sock_worker, &two_sockets)) {
			pthread_mutex_lock(&fd_mutex);
	    	if ((worker_fd = accept(sock_worker, clientptr, &clientlen)) < 0) { // Accept connection
	    		perror("accept");
	    	}
	    	pthread_mutex_unlock(&fd_mutex);
	    	if (workers_ip == NULL) {
			   	if ((rem_worker = gethostbyaddr((char *) &client.sin_addr.s_addr, sizeof(client.sin_addr.s_addr), client.sin_family)) == NULL) { // Find client's address
			  	    herror("gethostbyaddr2");
			  	    exit(1);
			  	}
			  	pthread_mutex_lock(&printing_mutex);
			   	printf("Accepted connection from worker %s\n", rem_worker->h_name);
			   	pthread_mutex_unlock(&printing_mutex);
			   	if (workers_ip == NULL) {
			   		pthread_mutex_lock(&workers_ip_mutex);
			   		workers_ip = new char[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &client.sin_addr, workers_ip, INET_ADDRSTRLEN);
			   		pthread_mutex_unlock(&workers_ip_mutex);
					pthread_mutex_lock(&printing_mutex);
			   		cout << "Worker's IP: " << workers_ip << endl;
			   		pthread_mutex_unlock(&printing_mutex);
				}
			}
		   	// Now put it in the circular buffer
		   	pthread_mutex_lock(&circular_buffer_mutex); // Lock mutex
		   	while (bufer->items >= bufer->size) {
		   		pthread_cond_wait(&bufer_not_full, &circular_buffer_mutex);
		   	}
		   	buf_insert(bufer, worker_fd, 'w');
		   	pthread_cond_signal(&bufer_not_empty);
		   	pthread_mutex_unlock(&circular_buffer_mutex); // Unlock mutex
		}
		FD_ZERO(&two_sockets);
    }
	///////////////////////////////////////////////////////////////////////////////
	// Threads do most of the work now.
	// And now wait for them to finish.
	for (int it = 0; it < numThreads; ++it) {
		if ((retourn = pthread_join(handling_threads[it], NULL)) != 0) {
			perror("pthread_join");
			exit(1);
		}
	}
	pthread_cond_destroy(&bufer_not_empty);
	pthread_cond_destroy(&bufer_not_full);
	pthread_mutex_destroy(&circular_buffer_mutex);
	pthread_mutex_destroy(&printing_mutex);
	pthread_mutex_destroy(&worker_info_list_mutex);
	pthread_mutex_destroy(&fd_mutex);
	cout << "END REACHED" << endl;
	delete[] workers_ip;
	delete[] clients_ip;
	delete[] handling_threads;
	delete[] handling_thread_args;
	return 0;
}