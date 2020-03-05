#ifndef COROUTINE_H
#define COROUTINE_H

#include <ucontext.h>
#include <stdbool.h>

#define STACK_SIZE          (8192)

enum coroutine_state{DEAD, READY, RUNNING, SUSPEND};

struct schedule;

typedef void (*coroutine_func)(struct schedule *s, void *args);

typedef struct coroutine
{
    coroutine_func func;        /* 被执行的函数 */
    void *args;                 /* 函数参数 */
    ucontext_t ctx;             /* 当前协程上下文 */
    enum coroutine_state state; /* 协程状态 */
    char stack[STACK_SIZE];     /* 栈区 */
} coroutine_t;

typedef struct schedule
{
    ucontext_t main_ctx;        /* 保存主逻辑上下文 */
    int running_coroutine;      /* 当前运行的协程ID */
    coroutine_t **coroutines;   /* 协程指针列表 */
    int coroutine_num;          /* 协程总数 */
    int max_index;              /* 当前使用的最大ID */
} schedule_t;

// 创建调度器
schedule_t * schedule_create(ssize_t coroutine_num);
// 销毁调度器
void schedule_destroy(schedule_t *s);
// 检查是否全部执行完成
bool schedule_finished(schedule_t *s);
// 打印协程信息
void schedule_show(schedule_t *s);

// 创建协程
int coroutine_create(schedule_t *s, coroutine_func func, void *args);
// 删除协程
void coroutine_destroy(schedule_t *s, int id);
// 切换协程
void coroutine_yield(schedule_t *s);
// 运行/恢复协程
void coroutine_resume(schedule_t *s, int id);
// 查看协程状态
enum coroutine_state coroutine_get_state(schedule_t *s, int id);

#endif
