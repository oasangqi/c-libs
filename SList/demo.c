#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

void list_splice_demo();

struct student {
	int num;
	char name[20];
	struct list_head list;
};

int main(void)
{
	struct student *list_node = NULL;
	struct list_head *pos = NULL,*n = NULL;
	struct student *pnode = NULL;
	struct student *tmp = NULL;
	int	i;

	// 定义并初始化一个链表头
	struct student head;
	INIT_LIST_HEAD(&head.list);

	// 创建节点加到链表，每次新节点都是加在head的后面，所以先加进来的节点就在链表里的最后
	for (i = 5; i > 0; i--) {
		list_node = (struct student *)malloc(sizeof(struct student));
		if (list_node == NULL) {
			printf("file,%s line,%d:malloc error!\n",__FILE__,__LINE__);
			exit(1);
		}

		list_node->num = i;
		snprintf(list_node->name, 20, "list_node%d", i);

		list_add(&list_node->list, &head.list);
		printf("node num %d has added to the list!\n", i);	
	}

	// 判断链表是否为空，即只有head
	if (list_empty(&head.list)) {
		printf("list is empty!\n");
		exit(1);
	} 

	// list_head结构的遍历，从head开始遍历
	printf("====== 遍历 list_head结构 ========\n");
	list_for_each(pos, &head.list) {
		// 用list_entry获取对应节点的student，并打印  
		pnode = list_entry(pos, struct student, list);
		printf("num:%d, name: %s\n", pnode->num, pnode->name);	
	}
	printf("=========== end ===============\n");

	// list_head结构从head开始遍历，中间有删除操作，比如删除第三个节点
	printf("===== 遍历list_head结构时删除node3 ====\n");
	list_for_each_safe(pos, n, &head.list) {
		// 用list_entry获取对应节点的student，并打印  
		pnode = list_entry(pos, struct student, list);
		if (pnode->num == 3) {
			list_del(pos);
			printf("node num %d has removed from the list!\n", pnode->num);
			free(pnode);
			continue;
		}
		printf("num:%d, name: %s\n", pnode->num, pnode->name);	
	}
	printf("=========== end ===============\n");

	// 遍历student结构，从head开始遍历
	printf("======= 遍历宿主结构 ============\n");
	list_for_each_entry(pnode, &head.list, list) {
		printf("num:%d, name: %s\n", pnode->num, pnode->name);	
	}
	printf("=========== end ===============\n");

	// 从head开始遍历student结构时，中间删除第四个节点
	printf("======== 遍历宿主结构时删除node4 ===========\n");
	list_for_each_entry_safe(pnode, tmp, &head.list, list) {
		if (pnode->num == 4) {
			list_del(&pnode->list);
			printf("node num %d has removed from the list!\n", pnode->num);
			free(pnode);
			continue;
		}
		printf("num:%d, name: %s\n", pnode->num, pnode->name);	
	}
	printf("=========== end ===============\n");

	// 删除所有节点（head不删除），释放资源
	list_for_each_safe(pos, n, &head.list) 
	{ 
		pnode = list_entry(pos, struct student, list); 
		list_del(pos); 
		free(pnode); 
	} 

	// 链表合并demo
	list_splice_demo();

	return 0;
}

void list_splice_demo()
{
	struct student *list_node = NULL;
	struct student *pnode = NULL;
	struct student *tmp = NULL;
	int	i;

	// 定义并初始化链表头
	struct student head1;
	INIT_LIST_HEAD(&head1.list);
	struct student head2;
	INIT_LIST_HEAD(&head2.list);

	// 创建节点加到链表head1
	for (i = 5; i > 0; i--) {
		list_node = (struct student *)malloc(sizeof(struct student));
		if (list_node == NULL) {
			printf("file,%s line,%d:malloc error!\n",__FILE__,__LINE__);
			exit(1);
		}

		list_node->num = i;
		snprintf(list_node->name, 20, "list_node%d", i);

		list_add(&list_node->list, &head1.list);
		printf("node num %d has added to the list!\n", i);	
	}
	// 创建节点加到链表head2
	for (i = 8; i > 5; i--) {
		list_node = (struct student *)malloc(sizeof(struct student));
		if (list_node == NULL) {
			printf("file,%s line,%d:malloc error!\n",__FILE__,__LINE__);
			exit(1);
		}

		list_node->num = i;
		snprintf(list_node->name, 20, "list_node%d", i);

		list_add(&list_node->list, &head2.list);
		printf("node num %d has added to the list!\n", i);	
	}

	// 判断链表head2是否为空，为空则不能合并head1
	if (list_empty(&head2.list)) {
		printf("list is empty!\n");
		exit(1);
	} 

	// 将head2链表合并添加到head1链表上
	list_splice_init(&head2.list, &head1.list);

	printf("======= 合并后的head1 ============\n");
	list_for_each_entry_safe(pnode, tmp, &head1.list, list) {
		printf("num:%d, name: %s\n", pnode->num, pnode->name);	
		list_del(&pnode->list);
		free(pnode);
		continue;
	}
	printf("=========== end ===============\n");
}
