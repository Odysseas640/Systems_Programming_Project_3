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
using namespace std;
extern int errno;
extern int successes;
extern int fails;
extern int n_of_directories;
// extern char** directories_array;

void create_log_file_and_quit(BSTree*& big_tree, HashTable*& disease_hash_table_entry_date, HashTable*& country_hash_table_entry_date, HashTable*& country_hash_table_exit_date, int n_of_directories, char**& directories_array, DateListNode**& date_files_list_array, int writefd, int readfd, int bufferSize, string& line) {
	// sleep(0.5); // When you press Ctrl+C, this prevents the worker from creating log files and sending SIGCHLD to the parent and causing it to make replacement workers before it gets the SIGINT
	struct timeval tv; // Sleep doesn't work, so I had to use this
	tv.tv_sec = 0;
	tv.tv_usec = 100000;
	select(0, NULL, NULL, NULL, &tv);
	cout << "WORKER'S SIGINT and SIGQUIT function called" << endl;
	char log_file_name[15];
	sprintf(log_file_name, "log_file.%d", getpid());
	ofstream log_file;
	log_file.open(log_file_name);
	for (int i = 0; i < n_of_directories; ++i) {
		// Extract country from directory
		if (strncmp(directories_array[i], "(none)", 6) == 0) // Some workers receive one directory less
			break;
		int country_letter_index = strlen(directories_array[i]) - 2;
		int country_length = 0;
		while (directories_array[i][country_letter_index] != '/') {
			country_length++;
			country_letter_index--;
		}
		int country_start_index = strlen(directories_array[i]) - country_length;
		char* country_to_write = new char[country_length + 1];
		strncpy(country_to_write, directories_array[i] + country_start_index - 1, country_length);
		strcpy(country_to_write + country_length, "\0");
		log_file << country_to_write << endl;
		delete[] country_to_write;
	}
	log_file << "TOTAL " << successes + fails << endl;
	log_file << "SUCCESS " << successes << endl;
	log_file << "FAIL " << fails << endl;
	log_file.close();
	close(readfd);
	close(writefd);
	// create_log_file_and_quit(n_of_directories, directories_array, 0, 0);
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
	line.clear();
	// sleep(0.5);
	// struct timeval tv; // sleep doesn't work, so I had to use select
	// tv.tv_sec = 0;
	// tv.tv_usec = 100000;
	// select(0, NULL, NULL, NULL, &tv);
	exit(38);
}

void sig_usr_1(BSTree*& big_tree, HashTable*& disease_hash_table_entry_date, HashTable*& country_hash_table_entry_date, HashTable*& country_hash_table_exit_date, int n_of_directories, char**& directories_array, DateListNode**& date_files_list_array, int writefd, int readfd, int bufferSize) {
	cout << "Worker's SIGUSR1 function called, looking for new date files" << endl;
	int entered_send_summary_loop = 0;
	// For every directory
	// For every date file, look for it in the date file list.
	// If it's not in the list, import data from it and add it to the list
	string line;
	Date* temp_entry_date, *temp_exit_date, *temp_entry_date2;
	string temp_diseaseID, temp_country;
	ifstream input_file;
	patientRecord* temp;
	int kontinue;
	for (int i = 0; i < n_of_directories; ++i) { // For every directory
		// cout << "Current directory: " << directories_array[i] << endl;
		kontinue = 0;
		char current_date_file_path[320];
		strcpy(current_date_file_path, directories_array[i]);
		int file_count = 0;
		DIR * dirp;
		struct dirent * entry;
		dirp = opendir(directories_array[i]); /* There should be error handling after this */
		if (strncmp(directories_array[i], "(none)", 6) == 0) // Some workers have one directory less
			break;
		if (dirp == NULL) {
			// cout << "Could not open specified directory: " << directories_array[i] << "\nMoving to next one." << endl;
			break;
		}
		while ((entry = readdir(dirp)) != NULL) { // For every date file in this directory
			// cout << "Current date file: " << entry->d_name << endl;
			kontinue = 0;
			if (entry->d_type != DT_REG)
				continue;
			DateListNode* current_date_list_node = date_files_list_array[i];
			while (current_date_list_node != NULL) { // Look for it in the list
				// cout << "List loop" << endl;
				if (strcmp(current_date_list_node->data, entry->d_name) == 0) {
					cout << "DATE FILE " << entry->d_name << " HAS BEEN IMPORTED" << endl;
					kontinue = 1;
					break;
				}
				current_date_list_node = current_date_list_node->Next;
			}
			if (kontinue == 1)
				continue;
			cout << "New date file: " << entry->d_name << endl;
			date_list_insert(date_files_list_array[i], entry->d_name);

			char current_date[15];
			// while (current_date_list_node != NULL) { // For every date in this list
			strcpy(current_date_file_path, directories_array[i]);
			strcpy(current_date, entry->d_name);
			// cout << "strlen before name: " << strlen(current_date_file_path) << endl;
			strcat(current_date_file_path, current_date); // PATH MUST END IN /    // Append date file name to path
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
				// cout << "LINE: " << line << endl;
				temp = new patientRecord(); // This goes in the big tree and pointer is re-used for next line in the file // OK
				string update_exit_date_id = "";
				string update_exit_date_date = "";
				int retourn = temp->initialize(line, current_date, directories_array[i], update_exit_date_id);
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
					insert_to_list(diseases_list, temp_diseaseID);
				}
				// disease, entry date and pointer to patientRecord go in the disease hash table
				// country, entry date and pointer to patientRecord go in the disease hash table
			} while (line != ""); // This isn't actually used, this loop stops at the "break" a few lines above.
			input_file.close();
			if (diseases_list != NULL) { // SEND SUMMARY
				cout << "Entered send summary loop" << endl;
				if (entered_send_summary_loop == 0) { // Parent should only be called to read summary ONCE, and IF there are new files.
					kill(getppid(), SIGUSR1); // NOW call parent to receive summary
					cout << "SENT SIGUSR1 TO PARENT TO RECEIVE SUMMARY" << endl;
					cout << "worker's writefd: " << writefd << endl;
				}
				entered_send_summary_loop = 1;
				DiseaseListNode* temp_node = diseases_list;
				while (temp_node != NULL) { // SEND SUMMARY FOR ALL DISEASES IN CURRENT DATE FILE
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
					/////////////////////////////////////////////////////////////////
					char ok_response[10];
					char answer_size_array[10];
					strcpy(answer_size_array, "_________");
					int answer_size = strlen(summary_send);
					sprintf(answer_size_array, "%d", answer_size);
					write(writefd, answer_size_array, 10); // SEND RESPONSE SIZE
					read(readfd, ok_response, 6); // READ 'RESPONSE SIZE RECEIVED OK'
					// write(writefd, answer_array, strlen(answer_array) + 1); // SEND ACTUAL RESPONSE
					int send_index = 0;
					int send_size = bufferSize;
					do {
						if (send_index + send_size > answer_size + 1)
							send_size = answer_size + 1 - send_index;
						// cout << "sending: " << send_size << endl;
						write(writefd, summary_send + send_index, send_size);
						send_index += send_size;
						read(readfd, ok_response, 6);
					} while (send_index < answer_size + 1);
					write(writefd, ok_response, 6);
					temp_node = temp_node->Next;

					read(readfd, ok_response, 6);
				}
				//delete list
				delete_list(diseases_list);
			}
		}
		closedir(dirp);
	}
	if (entered_send_summary_loop == 1) {
		char endd[10];
		// cout << "sending #end#\n";
		strcpy(endd, "#end#"); // Tell parent "no more summaries"
		write(writefd, endd, 10);
		// cout << "sent #end#\n";
		cout << "Done importing new files, you can type a command now:" << endl;
	}
	else
		cout << "No new files found, you can type a command now:" << endl;
}