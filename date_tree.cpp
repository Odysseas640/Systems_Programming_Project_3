#include "date_tree.h"

DateTree::DateTree() {
	this->Root = NULL;
}
void height_recursive(DateTreeNode* root, int& current_depth, int& max_depth) {
	// cout << "current_depth: " << current_depth << endl;
	current_depth++;
	if (current_depth > max_depth)
		max_depth = current_depth;
	if (root->Left != NULL)
		height_recursive(root->Left, current_depth, max_depth);
	if (root->Right != NULL)
		height_recursive(root->Right, current_depth, max_depth);
	current_depth--;
}
int height(DateTreeNode* root) {
	if (root == NULL)
		return 0;
	int current_depth = 0;
	int max_depth = 0;
	height_recursive(root, current_depth, max_depth);
	return max_depth;
}
int DateTree::insert(Date* entry_date, patientRecord* patient_ptr) {
	// DateTreeNode* noull = NULL;
	return this->insert(entry_date, patient_ptr, this->Root);
}
int DateTree::insert(Date* entry_date, patientRecord* patient_ptr, DateTreeNode*& root) { // I wanted to use the default argument "TreeNode* root = this->Root", but that is not allowed. So I had to simulate a default argument in the one-line function just above.
	// cout << "recursion\n";
	if (root == NULL) {
		root = new DateTreeNode;
		root->Left = NULL;
		root->Right = NULL;
		root->date_and_patient_ptr_list = new DateTreeListNode;
		root->date_and_patient_ptr_list->Next = NULL;
		// root->date_and_patient_ptr_list->date_ptr = entry_date;
		root->date_ptr = entry_date;
		root->date_and_patient_ptr_list->patient_ptr_to_big_tree = patient_ptr;
	}
	else if (root->date_ptr->earlier_than(entry_date)) {
		/*return */this->insert(entry_date, patient_ptr, root->Right);
		rebalance(root);
	}
	else if (entry_date->earlier_than(root->date_ptr)) {
		/*return */this->insert(entry_date, patient_ptr, root->Left);
		rebalance(root);
		// getchar();
	}
	else if (root->date_ptr->equal(entry_date)) {
		// cout << "Equal date, adding patient to list.\n";
		DateTreeListNode* current_node = root->date_and_patient_ptr_list;
		while (current_node->Next != NULL)
			current_node = current_node->Next;
		current_node->Next = new DateTreeListNode;
		current_node->Next->Next = NULL;
		// current_node->Next->date_ptr = entry_date;
		delete entry_date;
		current_node->Next->patient_ptr_to_big_tree = patient_ptr;
	}
	else // DIAGNOSTIC
		cout << "MISTAKE IN DATE TREE" << endl;
	return 0;
}
void DateTree::rebalance(DateTreeNode*& root) {
	if (root == NULL)
		return;
	if (root->Left != NULL && root->Left->Left != NULL && height(root->Left) - height(root->Right) >= 2 && height(root->Left->Left) - height(root->Left->Right) >= 1) {
		// cout << "a - Left Left rotation" << endl;
		DateTreeNode* y = root->Left;
		DateTreeNode* x = root->Left->Left;
		DateTreeNode* z = root;
		DateTreeNode* T3 = y->Right;
		DateTreeNode* T4 = root->Right;
		root = y;
		root->Left = x;
		root->Right = z;
		z->Left = T3;
		z->Right = T4;
	}
	else if (root->Left != NULL && root->Left->Right != NULL && height(root->Left) - height(root->Right) >= 2 && height(root->Left->Right) - height(root->Left->Left) >= 1) {
		// cout << "b - Left Right rotation" << endl; // NOT TESTED
		DateTreeNode* y = root->Left;
		DateTreeNode* x = root->Left->Right;
		DateTreeNode* z = root;
		DateTreeNode* T2 = x->Left;
		DateTreeNode* T3 = x->Right;
		root = x;
		x->Right = z;
		z->Left = T3;
		x->Left = y;
		y->Right = T2;
	}
	else if (root->Right != NULL && root->Right->Right != NULL && height(root->Right) - height(root->Left) >= 2 && height(root->Right->Right) - height(root->Right->Left) >= 1) {
		// cout << "c - Right Right rotation" << endl;
		DateTreeNode* y = root->Right;
		DateTreeNode* x = root->Right->Right;
		DateTreeNode* z = root;
		DateTreeNode* T1 = root->Left;
		DateTreeNode* T2 = y->Left;
		root = y;
		root->Right = x;
		root->Left = z;
		z->Left = T1;
		z->Right = T2;
	}
	else if (root->Right != NULL && root->Right->Left != NULL && height(root->Right) - height(root->Left) >= 2 && height(root->Right->Left) - height(root->Right->Right) >= 1) {
		// cout << "d - Right Left rotation" << endl;
		DateTreeNode* y = root->Right;
		DateTreeNode* x = root->Right->Left;
		DateTreeNode* z = root;
		DateTreeNode* T2 = x->Left;
		DateTreeNode* T3 = x->Right;
		root = x;
		x->Left = z;
		x->Right = y;
		z->Right = T2;
		y->Left = T3;
	}
}

int DateTree::count_patients_CASE_2_5(Date* date1, Date* date2, string disease) {
	int patient_count = 0;
	// cout << "count_patients - disease: " << disease << endl;
	this->count_patients_recursive_CASE_2_5(this->Root, patient_count, date1, date2, disease);
	// cout << "patient count: " << patient_count << endl;
	return patient_count;
}
void DateTree::count_patients_recursive_CASE_2_5(DateTreeNode* root, int& patient_count, Date* date1, Date* date2, string disease) {
	if (root->Left != NULL)
		count_patients_recursive_CASE_2_5(root->Left, patient_count, date1, date2, disease);

	DateTreeListNode* current_node = root->date_and_patient_ptr_list;
	if (disease == "") {
		while (current_node != NULL) {
			if (current_node->patient_ptr_to_big_tree->hospitalized_between_dates(date1, date2)) {
				// cout << "COUNT+++++++" << endl;
				patient_count++;
			}
			current_node = current_node->Next;
		}
	}
	else {
		while (current_node != NULL) {
			// cout << "DISEASE: " << disease << endl;
			// cout << "NODE: " << current_node->patient_ptr_to_big_tree->getDiseaseID() << endl;
			if (current_node->patient_ptr_to_big_tree->hospitalized_between_dates(date1, date2) && disease == current_node->patient_ptr_to_big_tree->getDiseaseID()) {
				// cout << "COUNT+++++++" << endl;
				patient_count++;
			}
			current_node = current_node->Next;
		}
	}

	if (root->Right != NULL)
		count_patients_recursive_CASE_2_5(root->Right, patient_count, date1, date2, disease);
}
int DateTree::count_patients_CASE_6(Date* date1, Date* date2, string disease) {
	int patient_count = 0;
	// cout << "count_patients - disease: " << disease << endl;
	this->count_patients_recursive_CASE_6(this->Root, patient_count, date1, date2, disease);
	// cout << "patient count: " << patient_count << endl;
	return patient_count;
}
void DateTree::count_patients_recursive_CASE_6(DateTreeNode* root, int& patient_count, Date* date1, Date* date2, string disease) {
	if (root->Left != NULL)
		count_patients_recursive_CASE_6(root->Left, patient_count, date1, date2, disease);

	DateTreeListNode* current_node = root->date_and_patient_ptr_list;
	if (disease == "") {
		while (current_node != NULL) {
			if (current_node->patient_ptr_to_big_tree->discharged_between_dates(date1, date2)) {
				// cout << "COUNT+++++++" << endl;
				patient_count++;
			}
			current_node = current_node->Next;
		}
	}
	else {
		while (current_node != NULL) {
			// cout << "DISEASE: " << disease << endl;
			// cout << "NODE: " << current_node->patient_ptr_to_big_tree->getDiseaseID() << endl;
			if (current_node->patient_ptr_to_big_tree->discharged_between_dates(date1, date2) && disease == current_node->patient_ptr_to_big_tree->getDiseaseID()) {
				// cout << "COUNT+++++++" << endl;
				patient_count++;
			}
			current_node = current_node->Next;
		}
	}

	if (root->Right != NULL)
		count_patients_recursive_CASE_6(root->Right, patient_count, date1, date2, disease);
}
int DateTree::count_patients_topk(Date* date1, Date* date2, string country, int& age1, int& age2, int& age3, int& age4) { // CASE 2
	// int patient_count = 0;
	// cout << "count_patients - country: " << country << endl;
	this->count_patients_recursive_topk(this->Root, date1, date2, country, age1, age2, age3, age4);
	// cout << "patient count: " << patient_count << endl;
	return 0;
}
void DateTree::count_patients_recursive_topk(DateTreeNode* root, Date* date1, Date* date2, string country, int& age1, int& age2, int& age3, int& age4) { // CASE 2
	if (root->Left != NULL)
		count_patients_recursive_topk(root->Left, date1, date2, country, age1, age2, age3, age4);

	DateTreeListNode* current_node = root->date_and_patient_ptr_list;
	while (current_node != NULL) {
		if (current_node->patient_ptr_to_big_tree->hospitalized_between_dates(date1, date2) && country == current_node->patient_ptr_to_big_tree->getCountry()) {
			int age = current_node->patient_ptr_to_big_tree->get_age();
			if (age <= 20)
				age1++;
			else if (age <= 40)
				age2++;
			else if (age <= 60)
				age3++;
			else
				age4++;
		}
		current_node = current_node->Next;
	}

	if (root->Right != NULL)
		count_patients_recursive_topk(root->Right, date1, date2, country, age1, age2, age3, age4);
}
void DateTree::print() {
	this->print_recursive(this->Root);
	// cout << "Root: "; Root->date_and_patient_ptr_list->date_ptr->print(); cout << endl;
	// cout << "Left: "; Root->Left->date_and_patient_ptr_list->date_ptr->print(); cout << endl;
	// cout << "Right: "; Root->Right->date_and_patient_ptr_list->date_ptr->print(); cout << endl;
}
void DateTree::print_recursive(DateTreeNode* root) { // I wanted to use the default argument "TreeNode* root = this->Root", but that is not allowed. So I had to simulate a default argument in the one-line function just above.
	if (root == NULL) {
		return;
	}
	if (root->Left != NULL)
		this->print_recursive(root->Left);

	DateTreeListNode* current_node = root->date_and_patient_ptr_list;
	while (current_node != NULL) {
		// current_node->date_ptr->print(); cout << endl;
		root->date_ptr->print(); cout << endl;
		cout << current_node->patient_ptr_to_big_tree->get_recordID() << endl;
		current_node = current_node->Next;
	}

	cout << endl;
	if (root->Right != NULL)
		this->print_recursive(root->Right);
}
void DateTree::tree_destructor_recursive(DateTreeNode* root) {
	// cout << "tree_destructor\n";
	if (root == NULL)
		return;
	if (root->Left != NULL)
		tree_destructor_recursive(root->Left);
	if (root->Right != NULL)
		tree_destructor_recursive(root->Right);

	// delete root->date_ptr;
	DateTreeListNode* current_node = root->date_and_patient_ptr_list;
	DateTreeListNode* to_delete;
	while (current_node != NULL) {
		// current_node->date_ptr->print();
		// delete current_node->date_ptr;
		to_delete = current_node;
		current_node = current_node->Next;
		delete to_delete;
	}
	delete root->date_ptr;

	delete root;
}
int DateTree::depth_to_bottom() {
	int current_depth = 0;
	int max_depth = 0;
	depth_to_bottom(this->Root, current_depth, max_depth);
	return max_depth;
}
void DateTree::depth_to_bottom(DateTreeNode* root, int& current_depth, int& max_depth) {
	current_depth++;
	if (current_depth > max_depth)
		max_depth = current_depth;
	if (root->Left != NULL)
		depth_to_bottom(root->Left, current_depth, max_depth);
	if (root->Right != NULL)
		depth_to_bottom(root->Right, current_depth, max_depth);
	current_depth--;
}
DateTree::~DateTree() {
	// cout << "TREE DESTRUCTOR\n";
	this->tree_destructor_recursive((this->Root));
}
DateTreeNode* DateTree::getRoot() {
	return this->Root;
}