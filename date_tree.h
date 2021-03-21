#ifndef __ODYS_DATE_TREE__
#define __ODYS_DATE_TREE__
#include <iostream>
#include <string>
#include "date.h"
#include "hash_table.h"
#include "patientRecord.h"
using namespace std;

typedef struct date_tree_list_node DateTreeListNode;
struct date_tree_list_node {
	// Date* date_ptr;
	patientRecord* patient_ptr_to_big_tree;
	DateTreeListNode* Next;
};

typedef struct date_tree_node DateTreeNode;
struct date_tree_node {
	Date* date_ptr;
	DateTreeListNode* date_and_patient_ptr_list;
	DateTreeNode* Left;
	DateTreeNode* Right;
};

class DateTree {
private:
	DateTreeNode* Root;
	int insert(Date* entry_date, patientRecord* patient_ptr, DateTreeNode*& root);
	void print_recursive(DateTreeNode* root);
	void tree_destructor_recursive(DateTreeNode* root);
	void count_patients_recursive_CASE_2_5(DateTreeNode* root, int& patient_count, Date* date1, Date* date2, string country);
	void count_patients_recursive_CASE_6(DateTreeNode* root, int& patient_count, Date* date1, Date* date2, string country);
	void count_patients_recursive_topk(DateTreeNode* root, Date* date1, Date* date2, string country, int& age1, int& age2, int& age3, int& age4); // top-k
	void rebalance(DateTreeNode*& root);
public:
	DateTree();
	int insert(Date* entry_date, patientRecord* patient_ptr);
	int count_patients_CASE_2_5(Date* date1, Date* date2, string country);
	int count_patients_CASE_6(Date* date1, Date* date2, string country);
	int count_patients_topk(Date* date1, Date* date2, string country, int& age1, int& age2, int& age3, int& age4);

	void print();
	int depth_to_bottom();
	void depth_to_bottom(DateTreeNode* root, int& current_depth, int& max_depth);
	~DateTree();
	DateTreeNode* getRoot();
};
#endif