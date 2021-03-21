#ifndef __ODYS_LIST__
#define __ODYS_LIST__
#include <string>
#include <cstring>
#include <iostream>
using namespace std;

typedef struct disease_list_node DiseaseListNode;
struct disease_list_node {
	// Date* date_ptr;
	string data;
	DiseaseListNode* Next;
};
int insert_to_list(DiseaseListNode*&, string);
int delete_list(DiseaseListNode*&);
int earlier_or_equal(char*, char*);

typedef struct date_list_node DateListNode;
struct date_list_node {
	char* data;
	DateListNode* Next;
};
void print_date_list(DateListNode*);
int date_list_insert(DateListNode*&, char*);
int delete_date_list(DateListNode*&);

typedef struct worker_info_list_node WorkerInfoListNode;
struct worker_info_list_node {
	int port;
	int n_countries;
	char** countries_array;
	pthread_mutex_t worker_mutex;
	WorkerInfoListNode* Next;
};
int insert_to_worker_info_list(WorkerInfoListNode*&, int, char*);
int delete_worker_info_list(WorkerInfoListNode*&);
void print_worker_info_list(WorkerInfoListNode*);
int get_port_of_worker_that_has_country(WorkerInfoListNode*, char*, pthread_mutex_t*&);
#endif