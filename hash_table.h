#ifndef __ODYS_HASH_TABLE__
#define __ODYS_HASH_TABLE__
#include <iostream>
#include <string>
#include "date.h"
#include "date_tree.h"
#include "patientRecord.h"
using namespace std;
class patientRecord;
class DateTree;
struct date_tree_node;

typedef struct bucket Bucket;
struct bucket {
	string* disease_or_country_list; // Dynamic array of strings. Turns out string size is always 32.
	DateTree** date_trees; // Dynamic array of Date Trees, depending on bucket size
	Bucket* next; // Next bucket in the list
};

class HashTable {
private:
	int array_size, bucket_size;
	Bucket** hash_table; // Dynamic array of buckets. Each bucket is a list node and may point to another bucket.
public:
	HashTable(int array_size, int bucket_size);
	void insert(string disease_or_country, Date* entry_date, patientRecord* patient_ptr); // Use hash function to insert disease.
	char* print_n_patients_within_CASE_2_5(int caase, string virusName, string arg2, string arg3, string arg4);
	char* print_n_patients_within_CASE_6(string virusName, string arg2, string arg3, string arg4);
	// If disease exists in the array, its bucket(s) are updated with a new case date
	// that points to a patient case in the big tree.
	void count_top_k_age_ranges(string country, string disease, string date1, string date2, int& age1, int& age2, int& age3, int& age4);
	void print();
	~HashTable();
};

#endif