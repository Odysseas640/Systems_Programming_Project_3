#include "circular_buffer.h"

void buf_initialize(circular_buffer*& buf, int size) { // I don't save the end, I find it on the fly
	buf = new circular_buffer;
	buf->sockfd = new int[size];
	buf->type = new char[size];
	for (int i = 0; i < size; ++i) {
		buf->type[i] = '0'; // 0 means it's empty
	}
	buf->start = 0;
	buf->end = 0;
	buf->items = 0;
	buf->size = size;
}
void buf_print(circular_buffer* buf) { // Valgrind finds errors here. No idea why. I don't use this function anyway, it was just for debugging.
	cout << "start: " << buf->start << ", items: " << buf->items << ", size: " << buf->size << endl;
	for (int i = 0; i < buf->size; ++i) {
		cout << "i: " 
		<< i << ", sock: " 
		<< buf->sockfd[i] << ", type: " 
		<< buf->type[i] << endl; // This is where the error is.
	}
}
int buf_insert(circular_buffer*& buf, int sockfd, char type) {
	buf->sockfd[buf->end] = sockfd;
	buf->type[buf->end] = type;
	buf->items++;
	buf->end++;
	buf->end = buf->end % buf->size;
	return 0;
}
int buf_pop(circular_buffer*& buf, int& sockfd_return, char& type_return) {
	if (buf->type[buf->start] != '0') {
		sockfd_return = buf->sockfd[buf->start];
		type_return = buf->type[buf->start];
		buf->type[buf->start] = '0';
		buf->start++;
		buf->start = buf->start % buf->size;
		buf->items--;
		return 0;
	}
	cout << "POP FAILED------------------------------------" << endl;
	return 1; // Pop failed
}
void buf_delete(circular_buffer*& buf) {
	delete[] buf->sockfd;
	delete[] buf->type;
	delete buf;
	buf = NULL;
}