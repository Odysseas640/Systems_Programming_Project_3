#ifndef __ODYS_SIGNALS__
#define __ODYS_SIGNALS__

void create_log_file_and_quit(BSTree*& big_tree, HashTable*& disease_hash_table_entry_date, HashTable*& country_hash_table_entry_date, HashTable*& country_hash_table_exit_date, int n_of_directories, char**& directories_array, DateListNode**& date_files_list_array, int writefd, int readfd, int bufferSize, string& line);
void sig_usr_1(BSTree*& big_tree, HashTable*& disease_hash_table_entry_date, HashTable*& country_hash_table_entry_date, HashTable*& country_hash_table_exit_date, int n_of_directories, char**& directories_array, DateListNode**& date_files_list_array, int writefd, int readfd, int bufferSize);

#endif