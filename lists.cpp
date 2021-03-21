#include "lists.h"

int insert_to_list(DiseaseListNode*& diseases_list_ptr, string new_disease) {
	if (diseases_list_ptr == NULL) {
		diseases_list_ptr = new DiseaseListNode;
		diseases_list_ptr->Next = NULL;
		diseases_list_ptr->data = new_disease;
	}
	else {
		if (diseases_list_ptr->data == new_disease)
			return 1;
		DiseaseListNode* temp_node = diseases_list_ptr;
		while (temp_node->Next != NULL) {
			temp_node = temp_node->Next;
			if (temp_node->data == new_disease)
				return 1;
		}
		temp_node->Next = new DiseaseListNode;
		temp_node->Next->Next = NULL;
		temp_node->Next->data = new_disease;
	}
	return 0;
}

int delete_list(DiseaseListNode*& diseases_list_ptr) {
	DiseaseListNode* current_node = diseases_list_ptr;
	DiseaseListNode* to_delete;
	while (current_node != NULL) {
		// current_node->date_ptr->print();
		// delete current_node->date_ptr;
		to_delete = current_node;
		current_node = current_node->Next;
		delete to_delete;
	}
	diseases_list_ptr = NULL;
	return 0;
}

int date_list_insert(DateListNode*& ListNode, char* new_date) {
	DateListNode* temp_node = ListNode;
	if (ListNode == NULL) {
		ListNode = new DateListNode;
		ListNode->Next = NULL;
		ListNode->data = new char[15];
		strcpy(ListNode->data, new_date);
	}
	else if (earlier_or_equal(new_date, ListNode->data) == 1) {
		DateListNode* new_node = new DateListNode;
		new_node->data = new char[15];
		strcpy(new_node->data, new_date);
		ListNode = new_node;
		new_node->Next = temp_node;
	}
	else {
		while (temp_node->Next != NULL) {
			if (earlier_or_equal(new_date, temp_node->Next->data) == 1) {
				break;
			}
			temp_node = temp_node->Next;
		}
		DateListNode* new_node = new DateListNode;
		new_node->data = new char[15];
		new_node->Next = temp_node->Next;
		strcpy(new_node->data, new_date);
		temp_node->Next = new_node;
	}
	return 0;
}

void print_date_list(DateListNode* ListNode) {
	// cout << "-----------------------------------------------" << endl;
	while (ListNode != NULL) {
		cout << ListNode->data << endl;
		ListNode = ListNode->Next;
	}
	// cout << "-----------------------------------------------" << endl;
}

int delete_date_list(DateListNode*& date_list_ptr) {
	DateListNode* current_node = date_list_ptr;
	DateListNode* to_delete;
	while (current_node != NULL) {
		// current_node->date_ptr->print();
		// delete current_node->date_ptr;
		to_delete = current_node;
		current_node = current_node->Next;
		delete[] to_delete->data;
		delete to_delete;
	}
	date_list_ptr = NULL;
	return 0;
}

int earlier_or_equal(char* date1, char* date2) {
	char date11[15];
	strcpy(date11, date1);
	char date22[15];
	strcpy(date22, date2);
	if (date11 == NULL || date22 == NULL) {
		return -1;
	}
	char* saveptr = date11;
	int day1 = atoi(strtok_r(saveptr, "-", &saveptr));
	int month1 = atoi(strtok_r(saveptr, "-", &saveptr));
	int year1 = atoi(strtok_r(saveptr, "-", &saveptr));

	saveptr = date22;
	int day2 = atoi(strtok_r(saveptr, "-", &saveptr));
	int month2 = atoi(strtok_r(saveptr, "-", &saveptr));
	int year2 = atoi(strtok_r(saveptr, "-", &saveptr));

	if (year1 < year2)
		return 1;
	else if (year1 > year2)
		return 0;
	else if (month1 < month2)
		return 1;
	else if (month1 > month2)
		return 0;
	else if (day1 < day2)
		return 1;
	else if (day1 > day2)
		return 0;
	else
		return 1;
}

int insert_to_worker_info_list(WorkerInfoListNode*& list_node, int port, char* countries) {
	// Make a new node, insert port.
	if (list_node == NULL) {
		list_node = new WorkerInfoListNode;
		list_node->Next = NULL;
		pthread_mutex_init(&list_node->worker_mutex, 0);
		list_node->port = port;
		int n_spaces = 0;
		for (int i = 0; i < (int) strlen(countries); ++i) { // Count spaces in the countries char array, n_countries = n_spaces+1
			if (countries[i] == ' ')
				n_spaces++;
		}
		cout << "n_spaces: " << n_spaces << endl;
		list_node->n_countries = n_spaces + 1;
		list_node->countries_array = new char*[list_node->n_countries];
		char* saveptr = countries;
		for (int i = 0; i < list_node->n_countries; ++i) { // strtok_r countries and put them in the char** array
			if (saveptr[0] == '\0') { // If we counted the countries wrong and there's actually fewer
				list_node->countries_array[i] = NULL;
				cout << "----------------------------" << endl;
				continue;
			}
			char* token = strtok_r(saveptr, " ", &saveptr);
			list_node->countries_array[i] = new char[strlen(token) + 1];
			strcpy(list_node->countries_array[i], token);
		}
	}
	else {
		WorkerInfoListNode* temp_node = list_node;
		while (temp_node->Next != NULL) {
			temp_node = temp_node->Next;
		}
		temp_node->Next = new WorkerInfoListNode;
		temp_node->Next->Next = NULL;
		pthread_mutex_init(&temp_node->Next->worker_mutex, 0);
		temp_node->Next->port = port;
		int n_spaces = 0;
		for (int i = 0; i < (int) strlen(countries); ++i) { // Count spaces in the countries char array, n_countries = n_spaces+1
			if (countries[i] == ' ')
				n_spaces++;
		}
		temp_node->Next->n_countries = n_spaces + 1;
		temp_node->Next->countries_array = new char*[temp_node->Next->n_countries];
		char* saveptr = countries;
		for (int i = 0; i < temp_node->Next->n_countries; ++i) { // strtok_r countries and put them in the char** array
			if (saveptr[0] == '\0') { // If we counted the countries wrong and there's actually fewer
				temp_node->Next->countries_array[i] = NULL;
				cout << "MISTAKE IN WORKER INFO LIST INSERT (it should be compensated for)" << endl;
				continue;
			}
			char* token = strtok_r(saveptr, " ", &saveptr);
			temp_node->Next->countries_array[i] = new char[strlen(token) + 1];
			strcpy(temp_node->Next->countries_array[i], token);
		}
	}
	return 0;
}
int delete_worker_info_list(WorkerInfoListNode*& list_node) {
	WorkerInfoListNode* current_node = list_node;
	WorkerInfoListNode* to_delete;
	while (current_node != NULL) {
		to_delete = current_node;
		current_node = current_node->Next;
		for (int i = 0; i < to_delete->n_countries; ++i) {
			if (to_delete->countries_array[i] != NULL)
				delete[] to_delete->countries_array[i];
		}
		delete[] to_delete->countries_array;
		delete to_delete;
	}
	list_node = NULL;
	return 0;
}
void print_worker_info_list(WorkerInfoListNode* list_node) {
	cout << "print_worker_info_list" << endl;
	while (list_node != NULL) {
		cout << list_node->port << endl; // SEG FAULT WHEN MASTER IS CALLED A SECOND TIME, AFTER THIS LINE
		for (int i = 0; i < list_node->n_countries; ++i) {
			if (list_node->countries_array[i] != NULL)
				cout << "country: " << list_node->countries_array[i] << endl;
			else
				cout << "NULL-----------" << endl;
		}
		list_node = list_node->Next;
	}
	cout << "print_worker_info_list end" << endl;
}
int get_port_of_worker_that_has_country(WorkerInfoListNode* list_node, char* country, pthread_mutex_t*& mutex_ptr) {
	while (list_node != NULL) {
		for (int i = 0; i < list_node->n_countries; ++i) {
			if (list_node->countries_array[i] != NULL) {
				if (strcmp(list_node->countries_array[i], country) == 0) {
					mutex_ptr = &list_node->worker_mutex; // This is the mutex that the server has to lock
					return list_node->port;
				}
			}
		}
		list_node = list_node->Next;
	}
	return -1; // Didn't find a worker who has this country
}