#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "ngx_queue.h"

typedef struct colors_s {
	ngx_queue_t queue;
} colors_t;

typedef struct color_node_s {
	char	name[128];
	int order;
	ngx_queue_t queue;
} color_node_t;

int sort_cb(const ngx_queue_t *q1, const ngx_queue_t *q2)
{
	color_node_t *n1 = ngx_queue_data(q1, color_node_t, queue);
	color_node_t *n2 = ngx_queue_data(q2, color_node_t, queue);
	return n1->order > n2->order;
}

int main(int argc, char *argv[])
{
	colors_t colors;
	printf("%d\n", sizeof("abc"));

	// 初始化队列
	ngx_queue_init(&colors.queue);

	// 插入节点
	color_node_t	red,yellow,blue;
	strncpy(red.name, "red", 128);
	red.order = 1;
	ngx_queue_insert_head(&colors.queue, &red.queue);

	strncpy(yellow.name, "yellow", 128);
	yellow.order = 2;
	ngx_queue_insert_head(&colors.queue, &yellow.queue);

	strncpy(blue.name, "blue", 128);
	blue.order = 3;
	ngx_queue_insert_tail(&colors.queue, &blue.queue);

	// 查找
	ngx_queue_t	*q;
	color_node_t	*n;
	for (q = ngx_queue_head(&colors.queue); q != ngx_queue_sentinel(&colors.queue); q = ngx_queue_next(q))
	{    
		n = ngx_queue_data(q, color_node_t, queue);
		printf("%s\t%d\n", n->name, n->order);
	}

	// 简单排序
	ngx_queue_sort(&colors.queue, sort_cb);

	for (q = ngx_queue_head(&colors.queue); q != ngx_queue_sentinel(&colors.queue); q = ngx_queue_next(q))
	{    
		n = ngx_queue_data(q, color_node_t, queue);
		printf("%s\t%d\n", n->name, n->order);
	}
	if (ngx_queue_empty(&colors.queue)) {
		printf("empty\n");
	}

	return 0;
}
