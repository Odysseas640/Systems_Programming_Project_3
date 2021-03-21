#include <iostream>
using namespace std;

typedef struct {
	int* sockfd;
	char* type;
	int start;
	int end;
	int items;
	int size;
} circular_buffer;

void buf_initialize(circular_buffer*& buf, int size);
int buf_insert(circular_buffer*& buf, int sockfd, char type);
int buf_pop(circular_buffer*& buf, int& sockfd_return, char& type);
void buf_delete(circular_buffer*& buf);
void buf_print(circular_buffer* buf);