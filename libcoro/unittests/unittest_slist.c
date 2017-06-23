#include "../src/slist.h"
#include <stdio.h>
#include <stdlib.h>

struct list_item_name(task)
{
	int a;
	double b;
	char* c;
	list_next_ptr(task);
};
list_def(task);

int main() 
{
	list_new(task, task_list);
	list_item_new(task, task_item1);
}

