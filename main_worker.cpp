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
#include <sys/types.h>	     /* sockets */
#include <sys/socket.h>	     /* sockets */
#include <netinet/in.h>	     /* internet sockets */
#include <netdb.h>	         /* gethostbyaddr */
#include <stdlib.h>	         /* exit */
#include <arpa/inet.h>
using namespace std;
extern int errno;
int successes = 0;
int fails = 0;
int n_of_directories;
char** directories_array;
DateListNode** date_files_list_array;
BSTree* big_tree;
HashTable* disease_hash_table_entry_date;
HashTable* country_hash_table_entry_date;
HashTable* country_hash_table_exit_date;
int send_summary; // When a worker is made to replace one that has quit, it shouldn't send a summary to the parent.
int bufferSize;
int writefd, readfd;
string line;

char* serverIP;
char* serverPort_char;

void sig_handler(int signo) {
	if (signo == SIGINT || signo == SIGQUIT) {
		create_log_file_and_quit(big_tree, disease_hash_table_entry_date, country_hash_table_entry_date, country_hash_table_exit_date, n_of_directories, directories_array, date_files_list_array, writefd, readfd, bufferSize, line);
	}
	if (signo == SIGUSR1) {
		sig_usr_1(big_tree, disease_hash_table_entry_date, country_hash_table_entry_date, country_hash_table_exit_date, n_of_directories, directories_array, date_files_list_array, writefd, readfd, bufferSize);
	}
}

int main(int argc, char const *argv[]) { // ./main dir1 dir2 dir3.....
	signal(SIGINT, sig_handler);
	signal(SIGQUIT, sig_handler);
	signal(SIGUSR1, sig_handler);
	sigset_t mask1;
	sigemptyset(&mask1);
	sigaddset(&mask1, SIGINT);
	sigaddset(&mask1, SIGQUIT);
	sigaddset(&mask1, SIGUSR1);
	sigprocmask(SIG_BLOCK, &mask1, NULL); // Block these signals while setting up workers
	cout << "Worker " << getpid() << " started\n";
	for (int i = 0; i < argc; ++i) {
		cout << i << " - " << argv[i] << endl;
	}
	if (argv[4][0] == 'y')
		send_summary = 1;
	else
		send_summary = 1; // This was 0 in the 2nd assignment
	bufferSize = atoi(argv[3]);

	if ( (writefd = open(argv[1], O_WRONLY)) < 0) {
		perror("Worker: Cannot open write fifo");
	}
	if ( (readfd = open(argv[2], O_RDONLY)) < 0) {
		perror("Worker: Cannot open read fifo");
	}
	cout << "Worker: FIFO opening done" << endl;
	cout << "READ: " << readfd << " - WRITE: " << writefd << endl;

////////////////////////////////////////////////////////////////////////////////////////////////
	// Maybe count files and lines in each date file, and set hash table size accordingly.
	int hashTableNumOfEntries = 10; // These sizes should be fine
	int bucketSize = 128;
	big_tree = new BSTree;
	disease_hash_table_entry_date = new HashTable(hashTableNumOfEntries, bucketSize);
	country_hash_table_entry_date = new HashTable(hashTableNumOfEntries, bucketSize);
	country_hash_table_exit_date = new HashTable(hashTableNumOfEntries, bucketSize);
	Date* temp_entry_date, *temp_exit_date, *temp_entry_date2;
	string temp_diseaseID, temp_country;
	ifstream input_file;
	patientRecord* temp;
	int quitt = 0;
	int retourn;
	// int argv_index = 1; // Set to 0 for exec call, 1 to run on its own
	// while (argv_index < argc) { // For every directory assigned to this worker // Set to < argc for exec call
	// while (argv[argv_index] != NULL) { // For every directory assigned to this worker

	char ok_response[10]; // Receive IP address
	char transfer_size[15];
	read(readfd, transfer_size, 10); // READ RESPONSE SIZE
	serverIP = new char[atoi(transfer_size) + 2];
	serverPort_char = new char[10];
	strcpy(ok_response, "OKres");
	write(writefd, ok_response, 6); // SEND 'RESPONSE SIZE OK'
	int response_size_int = atoi(transfer_size);
	int write_index = 0;
	int receive_size = bufferSize;
	do {
		if (write_index + receive_size > response_size_int + 1)
			receive_size = response_size_int + 1 - write_index;
		read(readfd, serverIP + write_index, receive_size);
		write_index += receive_size;
		serverIP[write_index] = '\0';
		write(writefd, ok_response, 6);
	} while (write_index < response_size_int + 1);
	read(readfd, serverPort_char, 10); // Receive server Port from parent
	write(writefd, ok_response, 6);
	cout << "serverIP: " << serverIP << endl;
	cout << "serverPort: " << serverPort_char << endl;
	int serverPort = atoi(serverPort_char);

	char temp_n_directories[4];
	read(readfd, temp_n_directories, 4);
	write(writefd, temp_n_directories, 4);
	n_of_directories = atoi(temp_n_directories);
	// cout << "Number of directories: " << n_of_directories << endl;
	// Make an array of char arrays (strings) that hold all directories assigned to this worker.
	directories_array = new char*[n_of_directories];
	date_files_list_array = new DateListNode*[n_of_directories];
	for (int i = 0; i < n_of_directories; ++i) {
		// Read directory size and allocate a char array to save it
		read(readfd, transfer_size, 10); // READ RESPONSE SIZE
		if (strncmp(transfer_size, "D/ok", 4) == 0)
			break;
		// cout << "Worker: Directory size received: " << transfer_size << endl;
		directories_array[i] = new char[atoi(transfer_size) + 2]; // Maybe make this +2 or +3, just for good measure
		strcpy(ok_response, "OKres");
		write(writefd, ok_response, 6); // SEND 'RESPONSE SIZE OK'
		// char response[atoi(transfer_size) + 1];
		response_size_int = atoi(transfer_size);

		write_index = 0;
		receive_size = bufferSize;
		do {
			if (write_index + receive_size > response_size_int + 1)
				receive_size = response_size_int + 1 - write_index;
			read(readfd, directories_array[i] + write_index, receive_size);
			write_index += receive_size;
			directories_array[i][write_index] = '\0';
			write(writefd, ok_response, 6);
		} while (write_index < response_size_int + 1);
		// cout << "Received: " << directories_array[i] << endl;
		read(readfd, ok_response, 6);
		// Make an array of lists. Each list contains the date files of one directory, in chronological order.
		// Read date files here, for the current (i) directory. Insert them all in the list.
		// When opening the date files, get their names from the list and strcat() them to the directory.
		date_files_list_array[i] = NULL;
		DIR * dirp;
		struct dirent * entry;
		if (strncmp(directories_array[i], "(none)", 6) == 0) // This worker has received one directory less
			continue;
		dirp = opendir(directories_array[i]);
		if (dirp == NULL) {
			cout << "Could not open specified directory: " << directories_array[i] << "\nMoving to next one." << endl;
			// return 1;
		}
		while ((entry = readdir(dirp)) != NULL) { // For every date file in this directory
			if (entry->d_type != DT_REG)
				continue;
			date_list_insert(date_files_list_array[i], entry->d_name);
		}
		closedir(dirp);
	}
	// cout << "DONE RECEIVING DIRECTORIES" << endl;
	///////////////////////////////////////// Connect to server
	// int workerPort = getpid(); // MAKE THIS AUTO-ASSIGNED BY O.S.
	char hostname[100];
	memset(hostname, 0, 100);
	gethostname(hostname, sizeof(hostname));
	cout << "Worker's hostname: " << hostname << endl;
	struct hostent* host_entry = gethostbyname(hostname);
	char* IPbuffer = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0])); // I took this line from GeeksforGeeks
	cout << "Worker's IP: " << IPbuffer << endl;

	////////////////////////////////////////////////////////////// Create socket so the server can send queries
	int sock2, newsock;
	struct sockaddr_in server2, client;
    socklen_t clientlen;
    socklen_t server2len;
    struct sockaddr *serverptr2=(struct sockaddr *)&server2;
    struct sockaddr *clientptr=(struct sockaddr *)&client;
    // struct hostent *rem;
    if ((sock2 = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        perror("socket");
    server2.sin_family = AF_INET; // Internet domain
    server2.sin_addr.s_addr = htonl(INADDR_ANY);
    server2.sin_port = htons(0); // OS choose a port
    // inet_pton(AF_INET, "127.73.67.124", &server2.sin_addr);
    if (bind(sock2, serverptr2, sizeof(server2)) < 0) {
        perror("bind-------------------------w----");
        exit(1);
    }
    server2len = sizeof(server2);
    getsockname(sock2, serverptr2, &server2len);
    cout << "--- Worker port: " << ntohs(server2.sin_port) << " --- " << endl;
    int workerPort = ntohs(server2.sin_port);

    int opt = 1;
    if (setsockopt(sock2, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { // I found this on geeksforgeeks
       	perror("setsockopt");
       	exit(EXIT_FAILURE);
   	}
	////////////////////////////////////////////////////////////// Done creating socket

	int             sock;
    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr*)&server;
    struct hostent *rem;
	/* Create socket */
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	/* Find server address */
	if ((rem = gethostbyname(serverIP)) == NULL) {	
	   herror("gethostbyname");
	   exit(1);
	}
    server.sin_family = AF_INET;       /* Internet domain */
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(serverPort);         /* Server port */
    /* Initiate connection */
    if (connect(sock, serverptr, sizeof(server)) < 0) {
	   perror("connect00");
	   return 10;
    }
    printf("Connecting to serverPort %d\n", serverPort);
    ////////////////////////////////////////////////////////////////////// Connected
    char workerPort_char[10];
    memset(workerPort_char, 0, 10);
    sprintf(workerPort_char, "%d", workerPort);
    write(sock, workerPort_char, 10);
    read(sock, ok_response, 6); // Sent this worker's port to Server

    // Extract countries from directories array and send them to server
    char countries_to_send[1024];
    countries_to_send[0] = '\0';
    countries_to_send[1023] = '\0';
    if (strncmp(directories_array[n_of_directories - 1], "(none)", 6) == 0) {
    	n_of_directories--;
    }
    for (int i = 0; i < n_of_directories; ++i) {
		int country_letter_index = strlen(directories_array[i]) - 2;
		int country_length = 0;
		while (directories_array[i][country_letter_index] != '/') {
			country_length++;
			country_letter_index--;
		}
		int country_start_index = strlen(directories_array[i]) - country_length;
		char country[33];
		// for (int j = country_start_index - 1; j < country_start_index + country_length - 1; ++j) {
		// 	country = country + directories_array[i][j];
		// }
		strncpy(country, directories_array[i] + country_start_index - 1, country_length);
		country[country_length] = '\0';
		strcat(countries_to_send, country);
		if (i < n_of_directories - 1)
			strcat(countries_to_send, " ");
	}
	cout << "countries_to_send: " << countries_to_send << endl;
	char answer_size_array[10];
	strcpy(answer_size_array, "_________");
	int answer_size = strlen(countries_to_send);
	sprintf(answer_size_array, "%d", answer_size);
	write(sock, answer_size_array, 10); // SEND COUNTRIES STRING SIZE
	read(sock, ok_response, 6); // READ 'SIZE RECEIVED OK'
	int send_index = 0;
	int send_size = 100; // SOCKET BUFFER
	do { // SEND ACTUAL COUNTRIES STRING
		if (send_index + send_size > answer_size + 1)
			send_size = answer_size + 1 - send_index;
		write(sock, countries_to_send + send_index, send_size);
		send_index += send_size;
		read(sock, ok_response, 6);
	} while (send_index < answer_size + 1);
	
	for (int i = 0; i < n_of_directories; ++i) {
		char current_date_file_path[320];
		int file_count = 0;
		char current_date[15];
		DateListNode* current_date_list_node = date_files_list_array[i];
		while (current_date_list_node != NULL) { // For every date in this list
			strcpy(current_date_file_path, directories_array[i]);
			strcpy(current_date, current_date_list_node->data);
			// cout << "strlen before name: " << strlen(current_date_file_path) << endl;
			strcat(current_date_file_path, current_date); // PATH MUST END IN /    // Concatenate file name to path
			// cout << "About to open: " << current_date_file_path << endl; // Final path of current date file
			current_date_file_path[strlen(current_date_file_path)] = '\0';
			file_count++;
			input_file.open(current_date_file_path);
			// cout << "File: " << current_date_file_path << endl;
			DiseaseListNode* diseases_list = NULL;
			do { // For every line in file
				getline(input_file, line);
				if (line == "") {
					// cout << "EOF\n";
					break;
				}
				temp = new patientRecord(); // This goes in the big tree and pointer is re-used for next line in the file // OK
				string update_exit_date_id = "";
				string update_exit_date_date = "";
				retourn = temp->initialize(line, current_date, directories_array[i], update_exit_date_id);
				if (retourn == 1) {
					fprintf(stderr, "ERROR\n");
					delete temp;
					continue;
				}
				else if (retourn == 107) { // Arbitrary number to show that we found an EXIT record
					// cout << "Found line with exit date, updating existing record." << endl;
					retourn = big_tree->update_patient_exit(update_exit_date_id, current_date);
					delete temp;
					// Look for update_exit_date_id in big tree
					patientRecord* search_result = big_tree->search(update_exit_date_id);
					if (search_result == NULL) {
						// cout << " --- MISTAKE --- update_exit_date_id not found in big tree." << endl;
						fprintf(stderr, "ERROR\n");
						continue;
					}
					temp_exit_date = new Date(current_date);
					temp_diseaseID = search_result->getCountry();
					country_hash_table_exit_date->insert(temp_diseaseID, temp_exit_date, search_result);
				}
				else { // It's an ENTRY record
					if (big_tree->insert(temp) == 1) {
						// cout << "Duplicate ENTER ID. Discarding." << endl;
						fprintf(stderr, "ERROR\n");
						delete temp;
						continue;
					}
					temp_entry_date = new Date(*temp->getEntryDate()); // Make a copy of this date for the diseases hash table
					temp_entry_date2 = new Date(*temp->getEntryDate()); // And another for the countries hash table

					temp_diseaseID = temp->getDiseaseID();
					disease_hash_table_entry_date->insert(temp_diseaseID, temp_entry_date, temp);
					temp_country = temp->getCountry();
					country_hash_table_entry_date->insert(temp_country, temp_entry_date2, temp);
					// ADD DISEASE TO LIST, IF IT'S ALREADY IN STOP
					if (send_summary == 1)
						insert_to_list(diseases_list, temp_diseaseID);
					// country_hash_table.insert(temp_country, temp_entry_date2, temp);
				}
				// disease, entry date and pointer to patientRecord go in the disease hash table
				// country, entry date and pointer to patientRecord go in the disease hash table
			} while (line != ""); // This isn't actually used, this loop stops at the "break" a few lines above.
			input_file.close();
			if (diseases_list != NULL && send_summary == 1) { // SEND SUMMARY FOR ALL DISEASES IN CURRENT DATE FILE
				DiseaseListNode* temp_node = diseases_list;
				// DiseaseListNode* temp_node0 = diseases_list;
				// while (temp_node0 != NULL) {
				// 	cout << temp_node0->data << endl;
				// 	temp_node0 = temp_node0->Next;
				// }
				while (temp_node != NULL) {
					int age1 = 0, age2 = 0, age3 = 0, age4 = 0;
					disease_hash_table_entry_date->count_top_k_age_ranges(temp_country, temp_node->data, current_date, current_date, age1, age2, age3, age4);
					char summary_send[200];
					char temp_number[10];
					char country_char_array[100];
					char disease_char_array[100];
					for (int i = 0; i < (int) temp_country.length() && i < 99; ++i) {
						country_char_array[i] = temp_country[i];
					}
					if (temp_country.length() > 98)
						country_char_array[99] = '\0';
					else
						country_char_array[temp_country.length()] = '\0';
					for (int i = 0; i < (int) temp_node->data.length() && i < 99; ++i) {
						disease_char_array[i] = temp_node->data[i];
					}
					if (temp_node->data.length() > 98)
						disease_char_array[99] = '\0';
					else
						disease_char_array[temp_node->data.length()] = '\0';
					if (temp_node == diseases_list) {
						strcpy(summary_send, current_date);
						strcat(summary_send, "\n");
						strcat(summary_send, country_char_array);
						strcat(summary_send, "\n");
						strcat(summary_send, disease_char_array);
					}
					else
						strcpy(summary_send, disease_char_array);
					strcat(summary_send, "\n");
					strcat(summary_send, "Age range 0-20 years: ");
					sprintf(temp_number, "%d", age1);
					strcat(summary_send, temp_number);
					strcat(summary_send, " cases\nAge range 21-40 years: ");
					sprintf(temp_number, "%d", age2);
					strcat(summary_send, temp_number);
					strcat(summary_send, " cases\nAge range 41-60 years: ");
					sprintf(temp_number, "%d", age3);
					strcat(summary_send, temp_number);
					strcat(summary_send, " cases\nAge range 60+ years: ");
					sprintf(temp_number, "%d", age4);
					strcat(summary_send, temp_number);
					// cout << "About to send summary" << endl;
					/////////////////////////////////////////////////////////////////
					char ok_response[10];
					char answer_size_array[10];
					strcpy(answer_size_array, "_________");
					int answer_size = strlen(summary_send);
					sprintf(answer_size_array, "%d", answer_size);
					write(sock, answer_size_array, 10); // SEND SUMMARY SIZE
					read(sock, ok_response, 6); // READ 'SUMMARY SIZE RECEIVED OK'
					int send_index = 0;
					int send_size = 100; // SOCKET BUFFER
					do { // SEND ACTUAL SUMMARY
						if (send_index + send_size > answer_size + 1)
							send_size = answer_size + 1 - send_index;
						write(sock, summary_send + send_index, send_size);
						send_index += send_size;
						read(sock, ok_response, 6);
					} while (send_index < answer_size + 1);
					write(sock, ok_response, 6);
					// cout << "Summary sent" << endl;
					temp_node = temp_node->Next;
				}
				delete_list(diseases_list);
			}
			current_date_list_node = current_date_list_node->Next;
		}
	}
	char endd[6];
	strcpy(endd, "#end#");
	send_summary = 1; // If this worker started as a replacement, it was set to not send a summary. Now, if it receives SIGUSR1 to get new date files, then it should send a summary.
	write(writefd, endd, 6);
	write(sock, endd, 6);
	close(sock);

	string input;
	string input_tokens[8];
	char answer_array[500];
	while (1) { // READ USER INPUT FROM SERVER
		sigprocmask(SIG_UNBLOCK, &mask1, NULL); // Now that worker is ready, accept signals
	    if (listen(sock2, 5) < 0) {
	    	perror("listen");
	    }
	    cout << "WORKER: Accepting...  ";
	    clientlen = sizeof(client);
	   	if ((newsock = accept(sock2, clientptr, &clientlen)) < 0) {
	   		perror("accept");
	   	}
	   	cout << "...accepted connection from server2" << endl;
		/////////////////////////////////////////////////////////////// Receive user input
		char user_input[100];
		read(newsock, transfer_size, 10);
		read(newsock, user_input, atoi(transfer_size) + 1);
		cout << "user_input: " << user_input << endl;
		///////////////////////////////////////////////////////////////
		char* saveptr = user_input;
		for (int i = 0; i < 8; ++i) {
			if (strcmp(saveptr, "") != 0 && saveptr[0] != ' ') // Ignore any spaces ' ' in a row
				input_tokens[i] = strtok_r(saveptr, " ", &saveptr);
			else
				input_tokens[i] = ""; // Stop if there's no more arguments
		}
		char* dynamic_answer_array = NULL;
		if (input_tokens[0] == "/diseaseFrequency") { // virusName date1 date2 [country]
			// cout << "CASE 2" << endl;
			// Check whether a country has been typed
			if (input_tokens[1] == "" || input_tokens[2] == "" || input_tokens[3] == "") {
				strcpy(answer_array, "!Couldn't find required arguments. Try again if you want.");
				fails++;
			}
			successes++;
			char* temp_answer = country_hash_table_entry_date->print_n_patients_within_CASE_2_5(2, input_tokens[1], input_tokens[2], input_tokens[3], input_tokens[4]);
			strcpy(answer_array, temp_answer);
			delete[] temp_answer;
		}
		else if (input_tokens[0] == "/topk-AgeRanges") { // k country disease date1 date2 : disease hash table with enter date
			// cout << "CASE 3" << endl;
			int brake = 0;
			if (input_tokens[1] == "" || input_tokens[2] == "") {
				strcpy(answer_array, "!Could not find required arguments. Aborting.");
				fails++;
			}
			else if (input_tokens[1][0] == '0') {
				strcpy(answer_array, "!k cannot be 0. Aborting.");
				brake = 1;
				fails++;
				break;
			}
			for (int i = 0; i < (int)input_tokens[1].length(); ++i) {
				if (input_tokens[1][i] < '0' || input_tokens[1][i] > '9') {
					strcpy(answer_array, "!You didn't enter a k number. Aborting.");
					brake = 1;
					fails++;
					break;
				}
			}
			if (brake == 0) {
				if (!is_date_string_OK(input_tokens[4]) || !is_date_string_OK(input_tokens[5])) {
					strcpy(answer_array, "!Invalid date format, please try again.");
					fails++;
				}
				else { // No mistakes found
					answer_array[0] = '\0';
					int k = stoi(input_tokens[1]);
					int age1 = 0, age2 = 0, age3 = 0, age4 = 0;
					disease_hash_table_entry_date->count_top_k_age_ranges(input_tokens[2], input_tokens[3], input_tokens[4], input_tokens[5], age1, age2, age3, age4);
					if (age1 == 0 && age2 == 0 && age3 == 0 && age4 == 0) {
						strcpy(answer_array, "!No diseases found");
					}
					else {
						float ages[5];
						ages[0] = age1;
						ages[1] = age2;
						ages[2] = age3;
						ages[3] = age4;
						ages[4] = age1 + age2 + age3 + age4;
						if (ages[4] < 0.00001)
							ages[4] = 1.0; // If there's no cases, don't print NaN
						char temp[10];
						if (k > 4)
							k = 4;
						for (int ik = 0; ik < k; ++ik) {
							if (ages[0] >= ages[1] && ages[0] >= ages[2] && ages[0] >= ages[3] && ages[0] > 0.00001) {
								strcat(answer_array, "0-20: ");
								sprintf(temp, "%.0f", 100 * ages[0] / ages[4]);
								strcat(answer_array, temp);
								strcat(answer_array, "%\n");
								ages[0] = 0.0;
							}
							else if (ages[1] >= ages[0] && ages[1] >= ages[2] && ages[1] >= ages[3] && ages[1] > 0.00001) {
								strcat(answer_array, "21-40: ");
								sprintf(temp, "%.0f", 100 * ages[1] / ages[4]);
								strcat(answer_array, temp);
								strcat(answer_array, "%\n");
								ages[1] = 0.0;
							}
							else if (ages[2] >= ages[0] && ages[2] >= ages[1] && ages[2] >= ages[3] && ages[2] > 0.00001) {
								strcat(answer_array, "41-60: ");
								sprintf(temp, "%.0f", 100 * ages[2] / ages[4]);
								strcat(answer_array, temp);
								strcat(answer_array, "%\n");
								ages[2] = 0.0;
							}
							else if (ages[3] >= ages[0] && ages[3] >= ages[1] && ages[3] >= ages[2] && ages[3] > 0.00001) {
								strcat(answer_array, "60+: ");
								sprintf(temp, "%.0f", 100 * ages[3] / ages[4]);
								strcat(answer_array, temp);
								strcat(answer_array, "%\n");
								ages[3] = 0.0;
							}
							else {
								strcat(answer_array, "(other age ranges are 0)\n");
								break;
							}
						}
					}
					successes++;
				}
			}
		}
		else if (input_tokens[0] == "/searchPatientRecord") { // recordID
			// cout << "CASE 4" << endl;
			patientRecord* search_result = big_tree->search(input_tokens[1]);
			if (search_result == NULL) {
				strcpy(answer_array, "!Not found");
				fails++;
			}
			else {
				char* temp_answer = search_result->turn_into_char_array();
				strcpy(answer_array, temp_answer);
				successes++;
				delete[] temp_answer;
			}
		}
		else if (input_tokens[0] == "/numPatientAdmissions") { // disease date1 date2 [country]
			// cout << "CASE 5" << endl;
			// Check whether a country has been typed
			if (input_tokens[1] == "" || input_tokens[2] == "" || input_tokens[3] == "") {
				strcpy(answer_array, "!Couldn't find required arguments. Try again if you want.");
				fails++;
			}
			else {
				dynamic_answer_array = country_hash_table_entry_date->print_n_patients_within_CASE_2_5(5, input_tokens[1], input_tokens[2], input_tokens[3], input_tokens[4]);
				strcpy(answer_array, dynamic_answer_array);
				successes++;
				delete[] dynamic_answer_array;
			}
		}
		else if (input_tokens[0] == "/numPatientDischarges") { // disease date1 date2 [country]
			// cout << "CASE 6" << endl;
			// Check whether a country has been typed
			if (input_tokens[1] == "" || input_tokens[2] == "" || input_tokens[3] == "") {
				strcpy(answer_array, "!Couldn't find required arguments. Try again if you want.");
				fails++;
			}
			else {
				dynamic_answer_array = country_hash_table_exit_date->print_n_patients_within_CASE_6(input_tokens[1], input_tokens[2], input_tokens[3], input_tokens[4]);
				strcpy(answer_array, dynamic_answer_array);
				successes++;
				delete[] dynamic_answer_array;
			}
		}
		else if (input_tokens[0] == "/exit") {
			quitt = 1;
			strcpy(answer_array, "exiting");
			sprintf(answer_array+7, "%d", getpid());
		}
		else {
			strcpy(answer_array, "!Unrecognized command, try again.");
		}
		// cout << "worker about to send answer with length " << strlen(answer_array) + 1 << endl;
		// write(newsock, answer_array, 100);
		sprintf(transfer_size, "%d", (int) strlen(answer_array));
		write(newsock, transfer_size, 10);
		write(newsock, answer_array, strlen(answer_array) + 1);
		cout << "worker just sent: " << answer_array << endl;
		/////////////////////////////////////////////////////////////////////
		if (quitt == 1) {
			cout << "WORKER EXITING QUITT=1" << endl;
			close(newsock);
			break;
		}
		close(newsock);
	}
	sigprocmask(SIG_UNBLOCK, &mask1, NULL); // Now that worker is ready again, accept signals
	close(writefd);
	close(readfd);
	close(sock2);
	for (int i = 0; i < n_of_directories; ++i) {
		delete[] directories_array[i];
		delete_date_list(date_files_list_array[i]);
	}
	delete[] directories_array;
	delete[] date_files_list_array;
	delete big_tree;
	delete disease_hash_table_entry_date;
	delete country_hash_table_entry_date;
	delete country_hash_table_exit_date;
	delete[] serverIP;
	delete[] serverPort_char;
	return 0;
}