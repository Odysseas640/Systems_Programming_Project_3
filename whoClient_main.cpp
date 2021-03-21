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
#include <stdlib.h>	         /* exit */
#include <pthread.h>
#include <fstream>
#include <fcntl.h>
#include <signal.h>
using namespace std;
pthread_mutex_t ready_set_go = PTHREAD_MUTEX_INITIALIZER; // Set up mechanism to make all threads go at once
pthread_mutex_t printing_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t n_queries_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fd_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cvar;
//int n_queries;
int go;
//int q1, q2, q3, q4, q5, q6;

struct thread_arguments {
	char* file_line;
};
struct sockaddr *serverptr;

void* thread_f(void* thread_args);

int main(int argc, char const *argv[]) {
//	n_queries = 0;
//	q1 = 0; q2 = 0; q3 = 0; q4 = 0; q5 = 0; q6 = 0;
	int queryFile_index = -1, numThreads = -1, serverPort = -1, serverIP_index = -1;

	for (int i = 1; i < argc-1; i=i+2) {
		if (strcmp(argv[i],"-q") == 0) {
			queryFile_index = i+1; // CHECK IF THIS IS A NUMBER
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
		else if (strcmp(argv[i],"-sp") == 0) {
			for (int j = 0; j < (int) strlen(argv[i+1]); ++j) {
				if (argv[i+1][j] > '9' || argv[i+1][j] < '0') {
					cout << "serverPort is not a number. Terminating." << endl;
					return 1;
				}
			}
			serverPort = atoi(argv[i+1]); // CHECK IF THIS IS A NUMBER
		}
		else if (strcmp(argv[i],"-sip") == 0) {
			serverIP_index = i+1; // CHECK IF THIS IS A NUMBER
		}
		else {
			cout << "MISTAKE" << endl;
		}
	}
	if (queryFile_index <= 0 || numThreads <= 0 || serverPort < 0 || serverIP_index < 0) {
		cout << "Could not find expected arguments. Terminating." << endl;
		return 1;
	}
	cout << "queryFile: " << argv[queryFile_index] << endl;
	cout << "numThreads: " << numThreads << endl;
	cout << "serverPort: " << serverPort << endl;
	cout << "serverIP: " << argv[serverIP_index] << endl;
	///////////////////////////////////////////////////////////////////// I copied a few lines here and there from the professor's code
	int* sockfd = new int[numThreads];
	struct thread_arguments *thread_args = new struct thread_arguments[numThreads];
	for (int i = 0; i < numThreads; ++i) {
		thread_args[i].file_line = new char[200];
	}
	pthread_t* thr = new pthread_t[numThreads];
    struct sockaddr_in server;
    serverptr = (struct sockaddr*)&server;
    struct hostent *rem;
	/* Find server address */
    if ((rem = gethostbyname(argv[serverIP_index])) == NULL) {	
	   herror("gethostbyname");
	   exit(1);
    }
    server.sin_family = AF_INET;       /* Internet domain */
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(serverPort);         /* Server port */
    /////////////////////////////////////////////////////////////// Open queries file
    char file_line[256];
	ifstream queries_file;
	queries_file.open(argv[queryFile_index]);
	string file_line_string;
	int retourn, exitt = 0, threads_used;
	pthread_cond_init(&cvar, 0);
	pthread_mutex_init(&ready_set_go, 0);
	int max_threads_used = 0;
	do {
		threads_used = 0;
		go = 0;
		for (int it = 0; it < numThreads; ++it) {
			getline(queries_file, file_line_string); // For every line, create a thread and give it the line
			if (file_line_string == "") {
				cout << "EOF" << endl;
				exitt = 1;
				break;
			}
			threads_used++;
			if (threads_used > max_threads_used) {
				max_threads_used = threads_used; // This is for closing the correct number of FDs
			}
			for (int i1 = 0; i1 < (int) file_line_string.length(); ++i1) {
				file_line[i1] = file_line_string[i1];
			}
			file_line[file_line_string.length()] = '\0';
			strcpy(thread_args[it].file_line, file_line);
			if ((retourn = pthread_create(thr+it, NULL, thread_f, thread_args+it)) != 0) { 
				perror("pthread_create");
				exit(1);
			}
		}
		pthread_mutex_lock(&ready_set_go);
		go = 1;
		pthread_mutex_unlock(&ready_set_go);
		pthread_cond_broadcast(&cvar);
		for (int it = 0; it < threads_used; ++it) { // Only free threads that were used (necessary if numThreads < file_lines)
			if ((retourn = pthread_join(thr[it], NULL)) != 0) {
				perror("pthread_join");
				exit(1);
			}
		}
//		cout << q1+q2+q3+q4+q5+q6 << " - " << q1 << " " << q2 << " " << q3 << " " << q4 << " " << q5 << " " << q6 << endl;
	} while (exitt == 0); // While there are still lines in the file, use the same threads again.
	pthread_cond_destroy(&cvar);
	pthread_mutex_destroy(&ready_set_go);
    queries_file.close();
    delete[] thr;
    for (int i = 0; i < numThreads; ++i) {
    	delete[] thread_args[i].file_line;
    }
    delete[] thread_args;
    delete[] sockfd;
    return 0;
}

void* thread_f(void* thread_args) {
	int sockfd;
	char buf[200];
	memset(buf, 0, 200);
	struct thread_arguments *argz = (struct thread_arguments*) thread_args;
	pthread_mutex_lock(&fd_mutex);
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    	perror("socket");
    }
    pthread_mutex_unlock(&fd_mutex);

	// Threads stop here, and start all at once
	pthread_mutex_lock(&ready_set_go);
	while (go == 0) {
		pthread_cond_wait(&cvar, &ready_set_go);
	}
	pthread_mutex_unlock(&ready_set_go);
	
	// Connect to server
    if (connect(sockfd, serverptr, sizeof(*serverptr)) < 0) {
		perror("connect");
		exit(3);
    }
	// Send query
	char transfer_size[10];
	memset(transfer_size, '\0', 10);
	strcpy(buf, argz->file_line);
	sprintf(transfer_size, "%d", (int) strlen(buf) + 1);
	write(sockfd, transfer_size, 10);
	write(sockfd, buf, ((int) strlen(buf)) + 1);
	// Done sending query
	pthread_mutex_lock(&printing_mutex);
	cout << "CLIENT SENT QUERY: " << buf << endl;
	pthread_mutex_unlock(&printing_mutex);
	// Read answer
	read(sockfd, transfer_size, 10);
	read(sockfd, buf, atoi(transfer_size) + 1);
	// Done reading answer
	pthread_mutex_lock(&printing_mutex);
	cout << buf << endl;
	pthread_mutex_unlock(&printing_mutex);
/*	pthread_mutex_lock(&n_queries_mutex);
	if (strcmp(buf, "!None!") == 0)
		q1++;
	else if (strcmp(buf, "Gaul 1\nTurnkey 1\n") == 0)
		q2++;
	else if (strcmp(buf, "1") == 0)
		q3++;
	else if (strcmp(buf, "4") == 0)
		q4++;
	else if (strcmp(buf, "7512 Judith May 67 Parkinson's 23-9-1998 --") == 0)
		q5++;
	else if (strcmp(buf, "60+: 100%\n(other age ranges are 0)\n") == 0)
		q6++;

	n_queries++;
	pthread_mutex_unlock(&n_queries_mutex);*/
	pthread_mutex_lock(&fd_mutex);
	close(sockfd);
	pthread_mutex_unlock(&fd_mutex);
	pthread_exit(NULL);
}