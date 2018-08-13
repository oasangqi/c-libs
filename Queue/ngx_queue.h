
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */



#ifndef _NGX_QUEUE_H_INCLUDED_
#define _NGX_QUEUE_H_INCLUDED_

#include <stddef.h>

typedef struct ngx_queue_s  ngx_queue_t;

/* 双向循环链表实现的队列
 * 队列中成员的类型可以不同，但必须有ngx_queue_t类型的成员 
 * */
struct ngx_queue_s {
    ngx_queue_t  *prev;
    ngx_queue_t  *next;
};


/* 初始化队列(哨兵) */
#define ngx_queue_init(q)                                                     \
    (q)->prev = q;                                                            \
    (q)->next = q


/* 判断队列是否为空(只有哨兵) */
#define ngx_queue_empty(h)                                                    \
    (h == (h)->prev)


/* 插入到队首(哨兵之后) */
#define ngx_queue_insert_head(h, x)                                           \
    (x)->next = (h)->next;                                                    \
    (x)->next->prev = x;                                                      \
    (x)->prev = h;                                                            \
    (h)->next = x


#define ngx_queue_insert_after   ngx_queue_insert_head


/* 插入到队尾(哨兵之前) */
#define ngx_queue_insert_tail(h, x)                                           \
    (x)->prev = (h)->prev;                                                    \
    (x)->prev->next = x;                                                      \
    (x)->next = h;                                                            \
    (h)->prev = x


/* 获取队首(哨兵的下一个) */
#define ngx_queue_head(h)                                                     \
    (h)->next


/* 获取队尾(哨兵的前一个) */
#define ngx_queue_last(h)                                                     \
    (h)->prev


/* 获取队列哨兵 */
#define ngx_queue_sentinel(h)                                                 \
    (h)


/* 获取下一个成员 */
#define ngx_queue_next(q)                                                     \
    (q)->next


/* 获取上一个成员 */
#define ngx_queue_prev(q)                                                     \
    (q)->prev


#if (NGX_DEBUG)

#define ngx_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next;                                              \
    (x)->prev = NULL;                                                         \
    (x)->next = NULL

#else

/* 移除一个成员 */
#define ngx_queue_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next

#endif


/* 将队列h于q成员处拆分成队列h和n，其中q位于n的队首 */
#define ngx_queue_split(h, q, n)                                              \
    (n)->prev = (h)->prev;                                                    \
    (n)->prev->next = n;                                                      \
    (n)->next = q;                                                            \
    (h)->prev = (q)->prev;                                                    \
    (h)->prev->next = h;                                                      \
    (q)->prev = n;


/* 将队列n加入到队列h的队尾 */
#define ngx_queue_add(h, n)                                                   \
    (h)->prev->next = (n)->next;                                              \
    (n)->next->prev = (h)->prev;                                              \
    (h)->prev = (n)->prev;                                                    \
    (h)->prev->next = h;


#define ngx_queue_data(q, type, link)                                         \
    (type *) ((u_char *) q - offsetof(type, link))


ngx_queue_t *ngx_queue_middle(ngx_queue_t *queue);
void ngx_queue_sort(ngx_queue_t *queue,
    int (*cmp)(const ngx_queue_t *, const ngx_queue_t *));


#endif /* _NGX_QUEUE_H_INCLUDED_ */
