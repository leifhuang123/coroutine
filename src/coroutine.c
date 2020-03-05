#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NDEBUG
#include <assert.h>
#include "coroutine.h"

static void run(schedule_t *s)
{
    int id = s->running_coroutine;
    if (id != -1)
    {
        coroutine_t *c = s->coroutines[id];
        assert(c != NULL);
        c->func(s, c->args);
        c->state = DEAD;
        s->running_coroutine = -1;
    }
}

schedule_t * schedule_create(ssize_t coroutine_num)
{
    schedule_t *s = (schedule_t *)malloc(sizeof(schedule_t));
    assert(s != NULL);
    s->running_coroutine = -1;
    s->max_index = 0;
    s->coroutine_num = coroutine_num;
    s->coroutines = (coroutine_t **)malloc(sizeof(coroutine_t *) * coroutine_num);
    assert(s->coroutines != NULL);
    memset(s->coroutines, '\0', sizeof(coroutine_t *) * coroutine_num);
    return s;
}

void schedule_destroy(schedule_t *s)
{
    int i = 0;
    for (i=0; i<s->max_index; i++)
    {
        coroutine_destroy(s, i);
    }
    free(s->coroutines);
    free(s);
}

bool schedule_finished(schedule_t *s)
{
    int i = 0;
    if (s->running_coroutine != -1)
        return false;
    
    for (i=0; i<s->max_index; i++)
    {
        coroutine_t *c = s->coroutines[i];
        if (c != NULL && c->state != DEAD)
            return false;
    }
    return true;
}

void schedule_show(schedule_t *s)
{
    int i;
    for (i=0; i<s->max_index; i++)
    {
        coroutine_t *c = s->coroutines[i];
        if (c != NULL)
            printf("coroutine id %d state %d\n", i, c->state);
        else
            printf("coroutine id %d state %d\n", i, DEAD);
    }
}

int coroutine_create(schedule_t *s, coroutine_func func, void *args)
{
    coroutine_t *c = NULL;
    int i = 0;
    
    for (i=0; i<s->max_index; ++i)
    {
        c = s->coroutines[i];
        if (c == NULL)
        {
            s->coroutines[i] = (coroutine_t *)malloc(sizeof(coroutine_t));
            assert(s->coroutines[i] != NULL);
            break;
        }
        if (c->state == DEAD)
        {
            break;
        }
    }
    if (i == s->max_index)
    {
        if (s->max_index >= s->coroutine_num)
            return -1;
        else
        {
            s->coroutines[i] = (coroutine_t *)malloc(sizeof(coroutine_t));
            assert(s->coroutines[i] != NULL);
            s->max_index ++;  // s->max_index <= s->coroutine_num
        }
    }
    c = s->coroutines[i];
    c->state = READY;
    c->func = func;
    c->args = args;
    
    getcontext(&(c->ctx));
    c->ctx.uc_stack.ss_sp = c->stack;
    c->ctx.uc_stack.ss_size = STACK_SIZE;
    c->ctx.uc_stack.ss_flags = 0;
    c->ctx.uc_link = &(s->main_ctx);
    makecontext(&(c->ctx), (void(*)(void))run, 1, s);
    
    return i;
}

void coroutine_destroy(schedule_t *s, int id)
{
    if (id >= 0 && id < s->max_index)
    {
        coroutine_t *c = s->coroutines[id];
        if (c != NULL)
        {
            s->coroutines[id] = NULL;
            free(c);
            if (id == s->max_index)
                s->max_index--;
        }
    }
}

void coroutine_yield(schedule_t *s)
{
    int id = s->running_coroutine;
    if (id != -1)
    {
        coroutine_t *c = s->coroutines[id];
        assert(c != NULL);
        c->state = SUSPEND;
        s->running_coroutine = -1;
        swapcontext(&(c->ctx), &(s->main_ctx));
    }
}

void coroutine_resume(schedule_t *s, int id)
{
    if (id >= 0 && id < s->max_index)
    {
        coroutine_t *c = s->coroutines[id];
        if (c != NULL)
        {
            if (c->state == READY || c->state == SUSPEND)
            {
                c->state = RUNNING;
                s->running_coroutine = id;
                swapcontext(&(s->main_ctx), &(c->ctx));
            }
        }
    }
}

enum coroutine_state coroutine_get_state(schedule_t *s, int id)
{
    if (id < 0 || id >= s->max_index)
        return DEAD;
    
    coroutine_t *c = s->coroutines[id];
    if (c == NULL)
        return DEAD;
    
    return c->state;
}
