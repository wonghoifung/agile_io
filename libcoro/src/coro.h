#ifndef CORO_HEADER
#define CORO_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
#include <ucontext.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "slist.h"

#ifdef __cplusplus
	#ifndef restrict
		#define restrict __restrict
	#endif
	extern "C" {
#endif

struct coroutine_attr_s
{
    size_t stacksize;
};

struct crt_channel_s
{
    volatile int value;
    list_item_ptr(task_queue) task_ptr;
    size_t buf_capacity;
    size_t buf_offset;
    void* buf;
};

typedef void* (*launch_routine_t)(void*);
typedef int (*bg_routine_t)(void*, void*);
typedef uint64_t coroutine_t;
typedef struct coroutine_attr_s coroutine_attr_t;
typedef struct crt_channel_s crt_sem_t;

#define CRT_ERR_SOCKET (-1)
#define CRT_ERR_BIND (-2)
#define CRT_ERR_LISTEN (-3)
#define CRT_ERR_SET_NONBLOCK (-4)
#define CRT_ERR_SETSOCKOPT (-5)

#define CRT_SEM_NORMAL_PRIORITY (0)
#define CRT_SEM_HIGH_PRIORITY (1)
#define CRT_SEM_CRITICAL_PRIORITY (2)

// 管理线程通知工作线程帮忙干活
void manager_notify_worker(void);
// 获得manager thread context和current worker thread context
void coroutine_get_context(ucontext_t** manager_context, ucontext_t** task_context);
// 管理线程设置协程退出（宏GenSetActionFunc）
void coroutine_set_finished_coroutine();
// 管理线程设置打开文件任务
void coroutine_set_disk_open(const char* pathname, int flags, ...);
// 管理线程设置读文件任务
void coroutine_set_disk_read(int fd, void* buf, size_t count);
// 管理线程设置写文件任务
void coroutine_set_disk_write(int fd, void* buf, size_t count);
// 管理线程设置读socket任务
void coroutine_set_sock_read(int fd, void* buf, size_t count, int msec);
// 管理线程设置写socket任务
void coroutine_set_sock_write(int fd, void* buf, size_t count, int msec);
// 管理线程设置tcp主动连接任务
void coroutine_set_sock_connect(in_addr_t ip, in_port_t port, int msec);
// 协程返回值
int  coroutine_get_retval();
// 初始化协程间信号量
int crt_sem_init(crt_sem_t* sem, int pshared __attribute__((unused)), unsigned int value);
// 协程信号量post
int crt_sem_post(crt_sem_t* sem);
// 协程信号量post，带优先级
int crt_sem_priority_post(crt_sem_t* sem, int flag);
// 协程信号量wait
int crt_sem_wait(crt_sem_t* sem);
// 协程信号量销毁
int crt_sem_destroy(crt_sem_t* sem);
// 初始化协程环境
int init_coroutine_env();
// 销毁回收协程环境
int destroy_coroutine_env();
// 创建协程
int crt_create(coroutine_t* cid, const void* restrict attr, launch_routine_t br, void* restrict arg);
// 设置stack大小
int crt_attr_setstacksize(coroutine_attr_t* attr, size_t stacksize);
// 协程让出线程
int crt_sched_yield(void);
// 协程返回码
#define crt_errno (crt_get_err_code())
// 设置为非阻塞
int crt_set_nonblock(int fd);
// 设置为阻塞
int crt_set_block(int fd);
// 任务或者协程执行完的返回码
int crt_get_err_code();
// 协程睡眠
int crt_msleep(uint64_t msec);
// 管理线程通知工作线程运行一个后台函数
int crt_bg_run(bg_routine_t bg_routine, void* arg, void* result);
// int crt_bg_order_run(bg_routine_t bg_routine, void* arg, void* result);
// 管理线程准备服务器监听端口
int crt_tcp_prepare_sock(in_addr_t addr, uint16_t port);
// 管理线程通知工作线程帮忙等待新连接
int crt_tcp_accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
// 管理线程的协程退出
#define crt_exit(value_ptr) do {        \
    coroutine_set_finished_coroutine(); \
    return NULL;                        \
} while (0)
// 管理线程的协程打开文件
#define crt_disk_open(pathname, flags, ...) ({                  \
    coroutine_set_disk_open(pathname, flags, ##__VA_ARGS__);    \
    manager_notify_worker();                                    \
    ucontext_t *manager_context, *task_context;                 \
    coroutine_get_context(&manager_context, &task_context);     \
    swapcontext(task_context, manager_context);                 \
    int retval = coroutine_get_retval();                        \
    retval;                                                     \
})
// 管理线程的协程读文件
#define crt_disk_read(fd, buf, count) ({                        \
    coroutine_set_disk_read(fd, buf, count);                    \
    manager_notify_worker();                                    \
    ucontext_t *manager_context, *task_context;                 \
    coroutine_get_context(&manager_context, &task_context);     \
    swapcontext(task_context, manager_context);                 \
    int retval = coroutine_get_retval();                        \
    retval;                                                     \
})
// 管理线程的协程写文件
#define crt_disk_write(fd, buf, count) ({                       \
    coroutine_set_disk_write(fd, buf, count);                   \
    manager_notify_worker();                                    \
    ucontext_t *manager_context, *task_context;                 \
    coroutine_get_context(&manager_context, &task_context);     \
    swapcontext(task_context, manager_context);                 \
    int retval = coroutine_get_retval();                        \
    retval;                                                     \
})
// 管理线程的协程关闭文件
#define crt_disk_close(fd) ({                                   \
    int retval = close(fd);                                     \
    retval;                                                     \
})
// 管理线程的协程读socket
#define crt_tcp_read(fd, buf, count) ({                         \
    coroutine_set_sock_read(fd, buf, count, 0);                 \
    manager_notify_worker();                                    \
    ucontext_t *manager_context, *task_context;                 \
    coroutine_get_context(&manager_context, &task_context);     \
    swapcontext(task_context, manager_context);                 \
    int retval = coroutine_get_retval();                        \
    retval;                                                     \
})
// 管理线程的协程写socket
#define crt_tcp_write(fd, buf, count) ({                        \
    coroutine_set_sock_write(fd, buf, count, 0);                \
    manager_notify_worker();                                    \
    ucontext_t *manager_context, *task_context;                 \
    coroutine_get_context(&manager_context, &task_context);     \
    swapcontext(task_context, manager_context);                 \
    int retval = coroutine_get_retval();                        \
    retval;                                                     \
})
// 管理线程的协程超时读socket
#define crt_tcp_read_to(fd, buf, count, msec) ({                \
    coroutine_set_sock_read(fd, buf, count, msec);              \
    manager_notify_worker();                                    \
    ucontext_t *manager_context, *task_context;                 \
    coroutine_get_context(&manager_context, &task_context);     \
    swapcontext(task_context, manager_context);                 \
    int retval = coroutine_get_retval();                        \
    retval;                                                     \
})
// 管理线程的协程超时写socket
#define crt_tcp_write_to(fd, buf, count, msec) ({               \
    coroutine_set_sock_write(fd, buf, count, msec);             \
    manager_notify_worker();                                    \
    ucontext_t *manager_context, *task_context;                 \
    coroutine_get_context(&manager_context, &task_context);     \
    swapcontext(task_context, manager_context);                 \
    int retval = coroutine_get_retval();                        \
    retval;                                                     \
})
// 管理线程的协程阻塞主动连接tcp
#define crt_tcp_blocked_connect(ip, port) ({                    \
    coroutine_set_sock_connect(ip, port, -1);                   \
    manager_notify_worker();                                    \
    ucontext_t *manager_context, *task_context;                 \
    coroutine_get_context(&manager_context, &task_context);     \
    swapcontext(task_context, manager_context);                 \
    int retval = coroutine_get_retval();                        \
    retval;                                                     \
})
// 管理线程的协程超时主动连接tcp
#define crt_tcp_timeout_connect(ip, port, msec) ({              \
    coroutine_set_sock_connect(ip, port, msec);                 \
    manager_notify_worker();                                    \
    ucontext_t *manager_context, *task_context;                 \
    coroutine_get_context(&manager_context, &task_context);     \
    swapcontext(task_context, manager_context);                 \
    int retval = coroutine_get_retval();                        \
    retval;                                                     \
})
// 管理线程的协程关闭socket
#define crt_sock_close(fd) ({                                   \
    int retval = close(fd);                                     \
    retval;                                                     \
})
// 协程mutex lock(现在没用)
#define crt_pthread_mutex_lock(mutex) ({                        \
    (void)mutex;                                                \
    0;                                                          \
})
// 协程mutex unlock(现在没用)
#define crt_pthread_mutex_unlock(mutex) ({                      \
    (void)mutex;                                                \
    0;                                                          \
})

#ifdef __cplusplus
	}
#endif

#endif
