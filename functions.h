#ifndef functions_h
#define functions_h

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <syslog.h>
#include <ctype.h> 

typedef struct Node
{
	int hour;
	int min;
	char* command;
	int mode;
	int prog_count;
	
	struct Node* next;
}Node;

struct tm* get_time(void);
int calc_sleep(struct tm* systime, int task_hour, int task_min);

void read_tasks(char* taskfile, int* task_count);
void delete_tasks(void);

//void print_task(struct Node* task);
//void print_tasks(void);

void MergeSort(struct Node** headRef);
struct Node* SortedMerge(struct Node* a, struct Node* b);
void FrontBackSplit(struct Node* source, struct Node** frontRef, struct Node** backRef);
void Chronological(void);

void signal_handler(int signum);
void open_log (void);

size_t trimwhitespace(char *out, size_t len, const char *str);
char** get_prog(char* input, char** prog);

void execute_task(void);

#endif
