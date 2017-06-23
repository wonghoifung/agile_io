#ifndef SLIST_HEADER
#define SLIST_HEADER

#include <fcntl.h>
#include <unistd.h>
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <pthread.h>

#ifndef __offset_of
	#define __offset_of(type, field) ((size_t)(&((type*)0)->field))
#endif

#ifndef mem_calloc
	#ifdef SLAB_CALLOC
		#define mem_calloc slab_calloc
	#else
		#define mem_calloc malloc
	#endif
#endif

#ifndef mem_alloc
	#ifdef SLAB_ALLOC
		#define mem_alloc slab_alloc
	#else
		#define mem_alloc malloc
	#endif
#endif

#ifndef mem_free
	#ifdef SLAB_FREE
		#define mem_free slab_free
	#else
		#define mem_free free
	#endif
#endif

#define STAILQ_HEAD(name, type) \
struct name {                   \
	struct type* stqh_first;    \
	struct type** stqh_last;    \
	pthread_mutex_t lock;       \
	unsigned int size;          \
}

#define STAILQ_HEAD_INITIALIZER(head) {NULL, &(head).stqh_first, PTHREAD_MUTEX_INITIALIZER, 0}

#define STAILQ_ENTRY(type) struct {struct type* stqe_next;}

#define STAILQ_CONCAT(head1, head2) do {           \
	if (!STAILQ_EMPTY((head2)))	 {                 \
		*(head1)->stqh_last = (head2)->stqh_first; \
		(head1)->stqh_last = (head2)->stqh_last;   \
		(head1)->size += (head2)->size;            \
		STAILQ_INIT((head2));                      \
	}                                              \
} while(0)                                         \

#define STAILQ_EMPTY(head) ((head)->stqh_first == NULL)

#define STAILQ_FIRST(head) ((head)->stqh_first)

#define STAILQ_FOREACH(var, head, field) \
for ((var)=STAILQ_FIRST((head)); (var); (var)=STAILQ_NEXT((var), field))

// TODO
#define STAILQ_FOREACH_SAFE(vae, head, field, tvar) \
for ((var)=STAILQ_FIRST((head)); (var) && ((tvar)=STAILQ_NEXT((var), field), 1); (var)=(tvar))

#define STAILQ_INIT(head) do {                 \
	STAILQ_FIRST((head)) = NULL;               \
	(head)->stqh_last = &STAILQ_FIRST((head)); \
	pthread_mutex_init(&(head)->lock, NULL);   \
	(head)->size = 0;                          \
} while(0)

// TODO
#define STAILQ_INSERT_AFTER(head, tqelm, elm, field) do {                  \
	if ((STAILQ_NEXT((elm), field) = STAILQ_NEXT((tqelm), field)) == NULL) \
		(head)->stqh_last = &STAILQ_NEXT((elm), field);                    \
	STAILQ_NEXT((tqelm), field) = (elm);                                   \
	(head)->size++;                                                        \
} while(0)

// TODO
#define STAILQ_INSERT_HEAD(head, elm, field) do {                   \
	if ((STAILQ_NEXT((elm), field) = STAILQ_FIRST((head))) == NULL) \
		(head)->stqh_last = &STAILQ_NEXT((elm), field);             \
	STAILQ_FIRST((head)) = (elm);                                   \
	(head)->size++;                                                 \
} while(0)

#define STAILQ_INSERT_TAIL(head, elm, field) do {   \
	STAILQ_NEXT((elm), field) = NULL;               \
	*(head)->stqh_last = (elm);                     \
	(head)->stqh_last = &STAILQ_NEXT((elm), field); \
	(head)->size++;                                 \
} while(0)

// TODO
#define STAILQ_LAST(head, field) \
(STAILQ_EMPTY((head)) ? NULL : ((__typeof__(head->stqh_first))((char*)((head)->stqh_last) - __offset_of(__typeof__(*(head->stqh_first)), field))))

#define STAILQ_NEXT(elm, field) ((elm)->field.stqe_next)

// TODO
#define STAILQ_REMOVE(head, elm, field) do {                                                       \
	if (STAILQ_FIRST((head)) == (elm))	 {                                                         \
		STAILQ_REMOVE_HEAD((head), field);                                                         \
	}                                                                                              \
	else {                                                                                         \
		__typeof__(head->stqh_first) curelm = STAILQ_FIRST((head));                                \
		while (STAILQ_NEXT(curelm, field) != (elm))                                                \
			curelm = STAILQ_NEXT(curelm, field);                                                   \
		if ((STAILQ_NEXT(curelm, field) = STAILQ_NEXT(STAILQ_NEXT(curelm, field), field)) == NULL) \
			(head)->stqh_last = &STAILQ_NEXT((curelm), field);                                     \
		(head)->size--;                                                                            \
	}                                                                                              \
} while(0)

// TODO
#define STAILQ_REMOVE_HEAD(head, field) do {                                       \
	if ((STAILQ_FIRST((head)) = STAILQ_NEXT(STAILQ_FIRST((head)), field)) == NULL) \
		(head)->stqh_last = &STAILQ_FIRST((head));                                 \
	(head)->size--;                                                                \
} while(0)

// TODO
#define STAILQ_REMOVE_HEAD_UNTIL(head, elm, field) do {             \
	if ((STAILQ_FIRST((head)) = STAILQ_NEXT((elm), field)) == NULL) \
		(head)->stqh_last = &STAILQ_FIRST((head));                  \
} while(0)

// 链表头struct名
#define list_head_name(name) name##_Head_S

// 链表元素struct名
#define list_item_name(name) name##_Item_S

// 链表头struct指针
#define list_head_ptr(listname) struct list_head_name(listname)*

// 链表元素struct指针
#define list_item_ptr(listname) struct list_item_name(listname)*

// 用于侵入用户自定义struct
#define list_next_ptr(listname) STAILQ_ENTRY(list_item_name(listname)) LIST_NEXT_PTR

// 定义链表头struct
#define list_def(listname) STAILQ_HEAD(list_head_name(listname), list_item_name(listname))

// new一个链表头struct, 分配空间, 初始化
#define list_new(listname, head_ptr)                                                    \
list_head_ptr(listname) head_ptr = mem_calloc(sizeof(struct list_head_name(listname))); \
STAILQ_INIT(head_ptr)

// new一个链表元素struct, 分配空间
#define list_item_new(listname, item_ptr) list_item_ptr(listname) item_ptr = mem_alloc(sizeof(struct list_item_name(listname)))

// 为已声明的链表元素struct指针分配空间
#define list_item_init(listname, item_ptr) item_ptr = mem_alloc(sizeof(struct list_item_name(listname)))

// 链表头struct加锁
#define list_lock(head_ptr) do {pthread_mutex_lock(&((head_ptr)->lock));} while(0)

// 链表头struct解锁
#define list_unlock(head_ptr) do {pthread_mutex_unlock(&((head_ptr)->lock));} while(0)

// 获取链表有多少元素
#define list_size(head_ptr) ((*(head_ptr)).size)

#define list_push(head_ptr, item_ptr) //

#define list_unshift(head_ptr, item_ptr) //

#define list_insert(head_ptr, pos_ptr, item_ptr) //

#define list_shift(head_ptr, item_ptr) //

#define list_pop(head_ptr, item_ptr) //

#define list_first(head_ptr) //

#define list_last(head_ptr) //

#define list_foreach(head_ptr, item_ptr) //

#define list_foreach_safe(head_ptr, item_ptr, temp_ptr) //

#define list_next(item_ptr) //

#define list_addr(head_ptr, n, item_ptr) //

#define list_empty(head_ptr) //

#define list_cat(head1, head2) //

#define list_remove(head_ptr, item_ptr) //

#define list_remove_head(head_ptr) //

#define list_delete(head_ptr, item_ptr) //

#define list_delete_head(head_ptr) //

#define list_destroy(head_ptr) //

#define list_clear(head_ptr) //

#endif
