#include "../src/slist.h"
#include <stdio.h>
#include <stdlib.h>

// 1.定义元素结构，里面要入侵一个指针指向下一个元素
struct list_item_name(task)
{
	int a;
	double b;
	const char* c;
	list_next_ptr(task);
};

static void construct_task(list_item_ptr(task) t, int a, double b, const char* c)
{
	t->a = a;
	t->b = b;
	t->c = c;
}

// 2.定义链表结构
list_def(task);

int main() 
{
	// 3.声明链表变量task_list并且分配内存
	list_new(task, task_list);

	// 4.声明链表元素变量task_item1...task_item5并分配内存
	list_item_new(task, task_item1); construct_task(task_item1, 1, 1.1, "1");
	list_item_new(task, task_item2); construct_task(task_item2, 2, 1.2, "2");
	list_item_new(task, task_item3); construct_task(task_item3, 3, 1.3, "3");
	list_item_new(task, task_item4); construct_task(task_item4, 4, 1.4, "4");
	list_item_new(task, task_item5); construct_task(task_item5, 5, 1.5, "5");

	// 5.把元素都放进task_list，4 3 5 1 2
	list_push(task_list, task_item1);
	list_push(task_list, task_item2);
	list_unshift(task_list, task_item3);
	list_unshift(task_list, task_item4);
	list_insert(task_list, task_item3, task_item5);

	// 6.遍历打印链表
	list_item_ptr(task) item = NULL;
	list_foreach(task_list, item)
	{
		printf("a:%d, b:%lf, c:%s\n", item->a, item->b, item->c);
	}

	// 7.数组索引，慢
	unsigned int i=0;
	for (; i<list_size(task_list); ++i) 
	{
		list_addr(task_list, i, item);
		printf("* a:%d, b:%lf, c:%s\n", item->a, item->b, item->c);
	}

	// 8.释放整个链表
	list_destroy(task_list);
}

