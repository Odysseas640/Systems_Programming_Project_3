#include <iostream>
#include <fstream>
#include "date.h"
#include "patientRecord.h"
#include "hash_table.h"
#include <unistd.h>
#include <dirent.h>
#include "lists.h"
#include <fcntl.h>
#include <signal.h>
#include "signals_worker.h"
#include <sys/wait.h>
#include <sys/select.h>
using namespace std;
extern int errno;
extern int successes;
extern int fails;
extern int n_of_directories;
// extern char** directories_array;

void receive_summary_from_new_files(int numWorkers, int* writefd, int* readfd, char* bufferSize) {
	cout << "PARENT SIGUSR1 FUNCTION CALLED" << endl;
		// Make a set with the workers' writefd's, and the one returned by select() is the one sending a summary of new date files
		fd_set read_fds;
		FD_ZERO(&read_fds);
		for (int i = 0; i < numWorkers; ++i) {
			FD_SET(readfd[i], &read_fds);
		}
		for (int i = 0; i < numWorkers; ++i) {
			cout << "i: " << i << ", readfd: " << readfd[i] << ", writefd: " << writefd[i] << endl;
		}
		// int n_fds = readfd[numWorkers - 1];
		// cout << "n_fds: " << n_fds << endl;
		// if (n_fds < writefd[numWorkers - 1])
		// 	n_fds = writefd[numWorkers - 1];
		// n_fds++; // Set this to the highest file descriptor +1

		int n_fds = 0;
		for (int i = 0; i < numWorkers; ++i) {
			if (n_fds < readfd[i])
				n_fds = readfd[i];
			if (n_fds < writefd[i])
				n_fds = writefd[i];
		}
		n_fds++;


		cout << "n_fds: " << n_fds << endl;
		struct timeval tv1;
		tv1.tv_sec = 10;
		tv1.tv_usec = 42;
		select(n_fds, &read_fds, NULL, NULL, &tv1);
		int summary_readfd = 0, summary_writefd = 0;
		int foundd = 0;
		for (int i = 0; i < numWorkers; ++i) {
			if (FD_ISSET(readfd[i], &read_fds)) {
				cout << "+ FOUND: only supposed to appear once" << endl;
				foundd = 1;
				summary_readfd = readfd[i];
				summary_writefd = writefd[i];
			}
		}
		if (foundd == 0)
			cout << "UNRECOVERABLE ERROR - Parent cannot find which worker is sending a summary for his new files." << endl;
		cout << "Readfd returned by select: " << summary_readfd << endl;
		// for (int i = 0; i < numWorkers; ++i) {
		// 	if (readfd[i] == summary_readfd) {
		// 		summary_writefd = writefd[i];
		// 		break;
		// 	}
		// 	cout << "CORRECT FD NOT FOUND" << endl;
		// }
	int bufferSize_int = atoi(bufferSize);
	do { // READ SUMMARY FROM ALL FILES OF ONE WORKER
		char transfer_size[15];
		char ok_response[10];
		// char response[200];
		read(summary_readfd, transfer_size, 10); // READ RESPONSE SIZE
		// cout << "size received: " << transfer_size << endl;
		if (strncmp(transfer_size, "#end#", 5) == 0) {
			break; // NO MORE SUMMARY TO READ
		}
		// cout << "Parent: 'response size' received: " << transfer_size << endl;
		strcpy(ok_response, "OKres");
		write(summary_writefd, ok_response, 6); // SEND 'RESPONSE SIZE OK'
		char response[atoi(transfer_size)];
		int response_size_int = atoi(transfer_size);
		// read(readfd, response, response_size_int + 1); // READ RESPONSE
		int write_index = 0;
		int receive_size = bufferSize_int;
		do {
			if (write_index + receive_size > response_size_int + 1)
				receive_size = response_size_int + 1 - write_index;
			read(summary_readfd, response + write_index, receive_size);
			write_index += receive_size;
			response[write_index] = '\0';
			// cout << "Received so far: " << response << endl;
			// getchar();
			write(summary_writefd, ok_response, 6);
		} while (write_index < response_size_int + 1);
		read(summary_readfd, ok_response, 6);
		// cout << "ok_response: " << ok_response << endl;
		cout << response << endl;

		write(summary_writefd, ok_response, 6);
	} while (/*strncmp(ok_response, "#end#", 5) != 0*/1);
	// cout << "SUMMARY RECEIVED" << endl;
}

void parent_replace_kid(int*& workers_pid_array, int numWorkers, int* writefd, int* readfd, char** fifo1, char** fifo2, char* bufferSize, int directories_per_worker, char*** workers_directory_array) {
	cout << "Parent received SIGCHLD" << endl;
	// cout << "Checking whether kids are still running" << endl;
	int a0;
	int retourn = waitpid(-1, &a0, WNOHANG);
	// cout << "waitpid return: " << retourn << endl;
	int worker_replace_index;
	for (int i = 0; i < numWorkers; ++i) {
		if (workers_pid_array[i] == retourn) {
			worker_replace_index = i;
			cout << "Replacing worker " << retourn << ", i=" << worker_replace_index << endl;
			break;
		}
	}
	
	int fork_return = fork();
	// wait(&aa);
	cout << "fork return: " << fork_return << endl;
	if (fork_return == 0) { // Only the KIDD should exec this, not the parent.
		cout << "NEW WORKER PID: " << getpid() << endl;
		cout << "Parent PID: " << getppid() << endl;
		cout << "New worker FDs: " << fifo1[worker_replace_index] << " - " << fifo2[worker_replace_index] << endl;
		// execv("./worker", workers_directory_array[0]);
		execl("./worker", "./worker", fifo1[worker_replace_index], fifo2[worker_replace_index], bufferSize, "n", NULL); // For some reason, valgrind replaces the first argument
		// ...with the executable name, while, without valgrind, worker's argv[0] is the first argument
	}
	// cout << "Parent: about to open fifos" << endl;
	if ((readfd[worker_replace_index] = open(fifo1[worker_replace_index], 0)) < 0) {
		perror("Parent: cannot open read FIFO");
	}
	// cout << "Opened one\n";
	if ((writefd[worker_replace_index] = open(fifo2[worker_replace_index], 1)) < 0) {
		perror("Parent: cannot open write FIFO");
	}
	// cout << "Parent: opening done" << endl;
	workers_pid_array[worker_replace_index] = fork_return; // Update worker pid with new worker
	
	char buff[100] = "-nothing-";
	sprintf(buff, "%d", directories_per_worker);
	buff[3] = '\0';
	write(writefd[worker_replace_index], buff, 4); // SEND WORKER NUMBER OF DIRECTORIES
	read(readfd[worker_replace_index], buff, 4);
	for (int j = 0; j < directories_per_worker; ++j) { // SEND DIRECTORIES TO NEW WORKER
		if (workers_directory_array[worker_replace_index][j] != NULL) {
			char ok_response[10];
			char answer_size_array[10];
			strcpy(answer_size_array, "_________");
			int answer_size = strlen(workers_directory_array[worker_replace_index][j]);
			sprintf(answer_size_array, "%d", answer_size);
			write(writefd[worker_replace_index], answer_size_array, 10); // SEND RESPONSE SIZE
			read(readfd[worker_replace_index], ok_response, 6); // READ 'RESPONSE SIZE RECEIVED OK'
			// write(writefd, answer_array, strlen(answer_array) + 1); // SEND ACTUAL RESPONSE
			int send_index = 0;
			int send_size = atoi(bufferSize);
			do {
				// cout << send_index << "-" << send_size << "-" << answer_size << endl;
				if (send_index + send_size > answer_size + 1)
					send_size = answer_size + 1 - send_index;
				// cout << "sending: " << send_size << endl;
				write(writefd[worker_replace_index], workers_directory_array[worker_replace_index][j] + send_index, send_size);
				send_index += send_size;
				read(readfd[worker_replace_index], ok_response, 6);
			} while (send_index < answer_size + 1);
			write(writefd[worker_replace_index], ok_response, 6);
		}
	}
	cout << "DONE SENDING DIRECTORIES" << endl;

	// REPLACEMENT WORKER DOES NOT SEND A SUMMARY
	read(readfd[worker_replace_index], buff, 6); // Read "#end#" to move past the summary sending code
	cout << "New worker is up and running, type a command:" << endl;
}

void kill_workers_and_terminate(int numWorkers, int* workers_pid_array, int*& writefd, int*&readfd, char**& fifo1, char**&fifo2, char*& bufferSize, char***& workers_directory_array, int directories_per_worker, int successes, int fails, char* input_dir, string& input) {
	cout << "Parent killing workers and terminating" << endl;
	for (int i = 0; i < numWorkers; ++i) {
		kill(workers_pid_array[i], SIGKILL);
	}
	for (int i = 0; i < numWorkers; ++i) {
		int exit_code = 0;
		waitpid(workers_pid_array[i], &exit_code, 0);
		// exit_code = exit_code >> 8;
		// cout << exit_code << endl;
	}
	char log_file_name[15];
	sprintf(log_file_name, "log_file.%d", getpid());
	ofstream log_file;
	log_file.open(log_file_name);
	for (int i = 0; i < numWorkers; ++i) {
		for (int j = 0; j < directories_per_worker; ++j) {
			if (strncmp(workers_directory_array[i][j], "(none)", 6) == 0) // Some workers receive one directory less
				break;
			int country_letter_index = strlen(workers_directory_array[i][j]) - 2;
			int country_length = 0;
			while (workers_directory_array[i][j][country_letter_index] != '/') {
				country_length++;
				country_letter_index--;
			}
			int country_start_index = strlen(workers_directory_array[i][j]) - country_length;
			char* country_to_write = new char[country_length + 1];
			strncpy(country_to_write, workers_directory_array[i][j] + country_start_index - 1, country_length);
			strcpy(country_to_write + country_length, "\0");
			log_file << country_to_write << endl;
			delete[] country_to_write;
		}
	}
	log_file << "TOTAL " << successes + fails << endl;
	log_file << "SUCCESS " << successes << endl;
	log_file << "FAIL " << fails << endl;
	log_file.close();
	delete[] workers_pid_array;
	for (int i = 0; i < numWorkers; ++i) {
		close(readfd[i]);
		close(writefd[i]);
	}
	delete[] writefd;
	delete[] readfd;
	for (int i = 0; i < numWorkers; ++i) {
		unlink(fifo1[i]);
		unlink(fifo2[i]);
		delete[] fifo1[i];
		delete[] fifo2[i];
	}
	input.clear();
	delete[] fifo1;
	delete[] fifo2;
	delete[] bufferSize;
	delete[] input_dir;
	for (int i = 0; i < numWorkers; ++i) {
		for (int j = 0; j < directories_per_worker; ++j) {
			delete[] workers_directory_array[i][j];
		}
		delete[] workers_directory_array[i];
	}
	delete[] workers_directory_array;
	exit(5);
}