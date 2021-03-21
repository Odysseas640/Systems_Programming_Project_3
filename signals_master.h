#ifndef __ODYS_SIGNALS_PARENT__
#define __ODYS_SIGNALS_PARENT__
#include <string>
using namespace std;
void parent_replace_kid(int*& workers_pid_array, int numWorkers, int* writefd, int* readfd, char** fifo1, char** fifo2, char* bufferSize, int directories_per_worker, char*** workers_directory_array);
void receive_summary_from_new_files(int numWorkers, int* writefd, int* readfd, char* bufferSize);
void kill_workers_and_terminate(int numWorkers, int* workers_pid_array, int*& writefd, int*&readfd, char**& fifo1, char**&fifo2, char*& bufferSize, char***& workers_directory_array, int directories_per_worker, int successes, int fails, char* input_dir, string& input);

#endif