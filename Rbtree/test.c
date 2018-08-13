#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "ngx_rbtree.h"
#include "ngx_crc32.h"


typedef struct ngx_resolver_s {
	ngx_rbtree_t              name_rbtree;
	ngx_rbtree_node_t         name_sentinel;
} ngx_resolver_t;

ngx_resolver_t	g_rs;

typedef struct ngx_resolver_node_s {
	ngx_rbtree_node_t         node;
	u_char                    name[128];
	u_short                   nlen;
} ngx_resolver_node_t;

#define ngx_resolver_node(n)                                                 \
	(ngx_resolver_node_t *)                                                  \
	((u_char *) (n) - offsetof(ngx_resolver_node_t, node))

int	ngx_memn2cmp(u_char *s1, u_char *s2, size_t n1, size_t n2)
{
	size_t     n;
	int	m, z;

	if (n1 <= n2) {
		n = n1;
		z = -1;

	} else {
		n = n2;
		z = 1;
	}

	m = memcmp(s1, s2, n);

	if (m || n1 == n2) {
		return m;
	}

	return z;
}

static void ngx_resolver_rbtree_insert_value(ngx_rbtree_node_t *temp,
		ngx_rbtree_node_t *node, ngx_rbtree_node_t *sentinel)
{
	ngx_rbtree_node_t    **p;
	ngx_resolver_node_t   *rn, *rn_temp;

	// 优先比较哈希值，哈希相同比较字符串
	for ( ;; ) {

		if (node->key < temp->key) {

			p = &temp->left;

		} else if (node->key > temp->key) {

			p = &temp->right;

		} else { /* node->key == temp->key */

			rn = ngx_resolver_node(node);
			rn_temp = ngx_resolver_node(temp);

			p = (ngx_memn2cmp(rn->name, rn_temp->name, rn->nlen, rn_temp->nlen)
					< 0) ? &temp->left : &temp->right;
		}

		if (*p == sentinel) {
			break;
		}

		temp = *p;
	}

	*p = node;
	node->parent = temp;
	node->left = sentinel;
	node->right = sentinel;
	ngx_rbt_red(node);
}

void init(void)
{
	ngx_rbtree_init(&g_rs.name_rbtree, &g_rs.name_sentinel,
			ngx_resolver_rbtree_insert_value);
}

void insert(const char *name)
{
	ngx_resolver_node_t *p = (ngx_resolver_node_t *)calloc(1, sizeof(ngx_resolver_node_t));
	strncpy(p->name, name, 128);
	p->nlen = strlen(p->name);
	p->node.key = ngx_crc32_short(p->name, p->nlen);
	ngx_rbtree_insert(&g_rs.name_rbtree, &p->node);
}

static ngx_resolver_node_t *
ngx_resolver_lookup_name(ngx_resolver_t *r, u_char *name, uint32_t hash)
{
    int	rc;
    ngx_rbtree_node_t    *node, *sentinel;
    ngx_resolver_node_t  *rn;

    node = r->name_rbtree.root;
    sentinel = r->name_rbtree.sentinel;

    while (node != sentinel) {

        if (hash < node->key) {
            node = node->left;
            continue;
        }

        if (hash > node->key) {
            node = node->right;
            continue;
        }

        /* hash == node->key */

        rn = ngx_resolver_node(node);

        rc = ngx_memn2cmp(name, rn->name, strlen(name), rn->nlen);

        if (rc == 0) {
            return rn;
        }

        node = (rc < 0) ? node->left : node->right;
    }

    /* not found */

    return NULL;
}

void deinit(ngx_rbtree_t *tree)
{
	ngx_resolver_node_t  *rn;

	while (tree->root != tree->sentinel) {

		rn = ngx_resolver_node(ngx_rbtree_min(tree->root, tree->sentinel));

		ngx_rbtree_delete(tree, &rn->node);

		free(rn);
	}
}

int main(int argc, char *argv[])
{
	ngx_crc32_table_init();
	// 初始化树
	init();

	// 插入
	insert("amazon");
	insert("baidu");
	insert("google");

	// 查找
	ngx_resolver_node_t *p = NULL;
	p = ngx_resolver_lookup_name(&g_rs, "baidu", ngx_crc32_short("baidu", strlen("baidu")));

	// 释放树
	deinit(&g_rs.name_rbtree);
	return 0;
}

