/*
 * chash.c
 *
 *  Created on: Dec 19, 2017
 *      Author: dmcl216
 */

#include "chash.h"
#include "config.h"
#include "messager.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

void chash_store_open(struct chash_store_s *t) {
    t->chash_store_init = chash_store_init;
    t->node_insert = chash_node_insert;
    t->node_delete = chash_node_delete;
    t->node_sort = chash_node_sort;
    t->host_push = chash_host_push;
    t->host_delete = chash_host_delete;
    t->host_find = chash_host_find;
    t->get_pre_host = chash_pre_host;
    t->host_md5 = chash_host_md5;
    t->value_put = chash_value_put;
    t->value_get = chash_value_get;
    t->value_delete = chash_value_delete;
    t->value_update = chash_value_update;
    t->value_store_realloc = chash_value_realloc;
    t->value_sort = chash_value_sort;
    t->value_md5 = chash_value_md5;
    t->chash_store_destructor = chash_destructor;
    t->cnt_belongs2 = chash_cnt_belongs2;
    t->data_belongs2 = chash_data_belongs2;
    t->value_print = chash_value_printf;
}

void chash_store_init(struct gossiper_s *gs, struct chash_store_s *t) {
    /* 初始存放1024个值 */
    t->gossiper = gs;
    t->value_store = malloc(sizeof(struct value_store_s) * 1024);
    t->space = 1024;
    t->n_value = 0;
    t->n_host = 1;

    char *ip = malloc(sizeof(char) * (strlen(LOCAL_HOST) + 1));
    strcpy(ip, LOCAL_HOST);
    t->host_push(ip, t);
}

/**
 * 节点插入，
 * 1. 向seed发送[node_insert:localhost]
 * 2. seed更新gossip
 * 3. seed返回gossip,更新本地gossip，host
 * 4. 向pre节点发送[data_migrate]请求数据
 * 5. pre节点发送数据
 */
int  chash_node_insert(struct chash_store_s *t) {
    struct str_s *g_msg = t->gossiper->gossiper_cur_msg(t->gossiper);
    uint32_t g_len = g_msg->used - G_HEADER_SIZE;
    struct str_s *ss = malloc(sizeof(struct str_s));
    ss->data = malloc(strlen(NODE_INSERT) + g_len);
    ss->len = ss->used = strlen(NODE_INSERT) + g_len;
    /* node_insert+gossip_msg */
    strcpy(ss->data, NODE_INSERT);
    strncpy(ss->data + strlen(NODE_INSERT),
            g_msg->data + G_HEADER_SIZE, g_len);

    struct messager_s ms;
    messager_open(&ms);
    if (ms.messager_init(SEED_HOST, SRVPORT, &ms) < 0) {
        fprintf(stdout, "[error]chash_node_insert: start failure\n");
        return -1;
    }

    /* 发送node_insert:localhost消息 */
    ms.messager_send(*ss, &ms);
    free(g_msg->data);
    free(g_msg);
    free(ss->data);
    free(ss);

    /* 收到gossip消息 */
    struct str_s ss_back;
    ss_back.data = malloc(sizeof(char) * 4096);
    ss_back.len = 4096;
    ss_back.used = 0;
    ms->messager_recv(&ss_back, ss_back.len, ms);
    free(ss_back.data);
    ss_back->len = 0;
    ss_back.used = 0;
    ms.messager_close(&ms);
    /* 解析 */
    if (ss_back.used > G_HEADER_SIZE
            && strncmp(ss_back, GOSSIP_HEADER, G_HEADER_SIZE) == 0) {
        char *str = ss_back.data + G_HEADER_SIZE;
        host_handler(t, str);


        /* data migrate */
        char *pre_host = t->get_pre_host(LOCAL_HOST, t);
        if (strcmp(pre_host, LOCAL_HOST) == 0) {
            return 0;
        }

        /* 发送 insert_migrate+gossip 消息 */
        g_msg = t->gossiper->gossiper_cur_msg(t->gossiper);
        g_len = g_msg->used - G_HEADER_SIZE;
        ss = malloc(sizeof(struct str_s));
        ss->len = ss->used = strlen(INSERT_MIGRATE) + g_len;
        ss->data = malloc(ss->used * sizeof(char));
        strcpy(ss->data, INSERT_MIGRATE);
        strncpy(ss->data + strlen(INSERT_MIGRATE),
                        g_msg->data + G_HEADER_SIZE, g_len);

        if (ms.messager_init(pre_host, SRVPORT, &ms) < 0) {
            fprintf(stdout, "[error]chash_node_insert: data migrate failure\n");
            return -1;
        }

        ms.messager_send(ss, &ms);

        /* 接受迁移数据 */
        struct str_s ss_back;
        ss_back.data = malloc(sizeof(char) * 4096);
        ss_back.len = 4096;
        ss_back.used = 0;
        ms->messager_recv(&ss_back, ss_back.len, &ms);
        if (ss_back.used == 0) {
            fprintf(stdout, "[info]chash_node_insert: has no data to migrate\n");
            return 0;
        }
        /* 收到的是字符串，转换成int数组 */
        int *arr = malloc(sizeof(char) * ss_back.used);
        assert (sizeof(char) * ss_back.used % sizeof(int) == 0);
        int size = sizeof(char) * ss_back.used / sizeof(int);
        memcpy(arr, ss_back, sizeof(char)*ss_back.used);
        int i = 0;
        for (i = 0; i < size; i++) {
            t->value_put(arr[i], t);
        }
        ms.messager_close(&ms);
        return 0;

    } else {
        fprintf(stdout,
                "[error]chash_node_insert: receive wrong socket message from server-%s\n",
                ss_back.data);
        return -1;
    }
}

/**
 * 节点删除
 * 1. 移除节点[node_delete:localhost]
 * 2. 迁移数据
 */
int  chash_node_delete(struct chash_store_s *t) {
    char node_delete[256] = NODE_DELETE;
    strcat(node_delete, LOCAL_HOST);
    struct str_s ss_delete = {node_delete, strlen(node_delete), strlen(node_delete)};
    return 0;
}

/**
 * 由于节点插入时是按序插入，所以sort函数暂不实现
 */
void chash_node_sort(struct chash_store_s *t) {
    return ;
}

/**
 * 根据md5值按序插入节点
 * @ip 待插入节点的ip
 * @t this指针
 * @return 0 on success, -1 on fail
 */
int chash_host_push(char *ip, struct chash_store_s *t) {
    struct chash_host host;
    host.ipv4 = ip;
    host.md5 = t->host_md5(ip, t);
    unsigned int i;
    for (i = 0; i < t->n_host; i++) {
        if (host.md5 == t->hosts[i].md5) {
            return 0;
        } else if (host.md5 < t->hosts[i].md5) {
            break;
        }
    }

    if (i == t->n_host && t->n_host >= NODE_MAX_NUM) {
        fprintf(stdout, "[error]chahs_host_push: over max num\n");
        return -1;
    }

    int j;
    for (j = t->n_host; j > i; j--) {
        t->hosts[j] = t->hosts[j-1];
    }
    t->hosts[j] = host;
    t->n_host++;
    return 0;
}

/**
 * 删除节点
 * @ip
 * @t
 * @return 0 on success, -1 on fail
 */
int chash_host_delete(char *ip, struct chash_store_s *t) {
    struct chash_host host;
    host.ipv4 = ip;
    host.md5 = t->host_md5(ip, t);
    unsigned int i;
    for (i = 0; i < t->n_host; i++) {
        if (host.md5 == t->hosts[i].md5) {
            break;
        }
    }

    unsigned int j;
    for (j = i; j < t->n_host; j++) {
        t->hosts[j] = t->hosts[j+1];
    }
    if (i < t->n_host)
        t->n_host--;

    return 0;
}

/**
 * 查找host
 * @ch 需要查找的Host
 * @t
 * @return index on success, -1 on fail
 */
int  chash_host_find(struct chash_host ch, struct chash_store_s *t) {
    struct chash_host host;
    host.ipv4 = ip;
    host.md5 = t->host_md5(ip, t);
    unsigned int i;
    for (i = 0; i < t->n_host; i++) {
        if (host.md5 == t->hosts[i].md5) {
            return i;
        }
    }

    return -1;
}

/**
 * 查找指定节点的前驱节点
 * @ip
 * @t
 * @return ip of pre node on success, NULL if ip does not exist
 */
char* chash_pre_host(char *ip, struct chash_store_s *t) {
    struct chash_host host;
    host.ipv4 = ip;
    host.md5 = t->host_md5(ip, t);
    unsigned int i;
    for (i = 0; i < t->n_host; i++) {
        if (host.md5 == t->hosts[i].md5) {
            break;
        }
    }

    if (i == t->n_host) {
        return NULL;
    } else {
        int idx = (i + t->n_host - 1) % t->n_host;
        return t->hosts[idx];
    }
}

long chash_host_md5(char *ip, struct chash_store_s *t);

/**
 * 扩容
 * @t
 * @return 0 on success, -1 on fail
 */
int chash_value_realloc(struct chash_store_s *t) {
    unsigned int new_size = t->space + t->space / 2;
    if (new_size > VALUE_MAX_NUM) {
        fprintf(stdout, "[error]chash_value_realloc: realloc failure\n");
        return -1;
    }
    struct value_store_s *new_store = malloc(
            sizeof(struct value_store_s) * new_size);
    memcpy(new_store, t->value_store, sizeof(struct value_store_s) * t->n_value);
    struct value_store_s *old_store = t->value_store;
    t->value_store = new_store;
    t->space = new_size;
    free(old_store);
    return 0;
}

/**
 * 数据插入：如果数据存在就更新，不存在插入
 * @v int型数据
 * @t
 * @return 0 on success, -1 on fail
 */
int chash_value_put(int v, struct chash_store_s *t) {
    struct value_store_s vs;
    vs.value = v;
    vs.md5 = t->value_md5(v, t);
    uint32_t b, i, j;

    /* 找到对应节点 */
    b = v_belongs2(v, t);

    /* 插入数据属于本节点 */
    if (strcmp(t->hosts[b].ipv4, LOCAL_HOST) == 0) {
        if (t->n_value >= t->space && t->value_store_realloc(t) < 0) {
            return -1;
        }

        for (i = 0; i < t->n_value; i++) {
            /* 更新 */
            if (vs.md5 == t->value_store[i].md5) {
                t->value_store[i].value = v;
                return 0;
            } else if (vs.md5 < t->value_store[i].md5) {
                break;
            }
        }

        for (j = t->n_value; j > i; j--) {
            t->value_store[j] = t->value_store[j - 1];
        }
        t->value_store[j] = vs;
        t->n_value++;
        fprintf(stdout, "value_put: value is inserted in %s\n", LOCAL_HOST);
        return 0;
    } else {
        /* 插入的数据不属于本节点，发送给对应节点 */
        char data_insert[64] = DATA_INSERT;
        memcpy(data_insert + strlen(DATA_INSERT), &v, sizeof(int));
        size_t len = strlen(DATA_INSERT) + sizeof(int) / sizeof(char);
        struct str_s ss = {data_insert, 64, len};
        struct messager_s ms;
        messager_open(&ms);
        if (ms.messager_init(t->hosts[b].ipv4, SRVPORT, &ms) < 0) {
            fprintf(stdout, "[error]chash_value_put: send value to %s failure.\n",
                        t->hosts[b].ipv4);
            return -1;
        }
        ms.messager_send(ss, &ms);

        char buf[1024];
        struct str_s ss_back = {buf, 1024, 0};
        ms.messager_recv(ss_back, ss_back.len, &ms);
        ms.messager_close(&ms);
        return 0;
    }
}

/**
 * 查找数据
 * @v
 * @t
 * @return index on success, -1 on failure
 */
int chash_value_get(int v, struct chash_store_s *t) {
    uint32_t i;
    for (i = 0; i < t->n_value; i++) {
        if (v == t->value_store[i].value) {
            return i;
        }
    }

    return -1;
}

/**
 * 由于只存v，这里v相当于key，所以不需要更新，更新是在put的时候做的，
 * 以后改成key-value形式再实现
 */
int chash_value_update(int oldV, int newV, struct chash_store_s *t) {
    return -1;
}

/**
 * 删除指定v
 * @v
 * @t
 * @return 0 on success, -1 on failure
 */
int chash_value_delete(int v, struct chash_store_s *t) {
    uint32_t b, i, j;
    b = v_belongs2(v, t);

    /* 数据属于本节点 */
    if (strcmp(t->hosts[b].ipv4, LOCAL_HOST) == 0) {
        for (i = 0; i < t->n_value; i++) {
            if (v == t->value_store[i].value) {
                break;
            }
        }
        for (j = i; j < t->n_value; j++) {
            t->value_store[j] = t->value_store[j - 1];
        }
        if (i < t->n_value) {
            t->n_value--;
        }
        fprintf(stdout, "value_delete: value is deleted in %s\n", LOCAL_HOST);
        return 0;
    } else {
        /* 数据不属于本节点，发送给对应节点 */
        char data_delete[64] = DATA_DELETE;
        memcpy(data_delete + strlen(DATA_DELETE), &v, sizeof(int));
        size_t len = strlen(DATA_DELETE) + sizeof(int) / sizeof(char);
        struct str_s ss = { data_delete, 64, len };
        struct messager_s ms;
        messager_open(&ms);
        if (ms.messager_init(t->hosts[b].ipv4, SRVPORT, &ms) < 0) {
            fprintf(stdout,
                    "[error]chash_value_delete: send value to %s failure.\n",
                    t->hosts[b].ipv4);
            return -1;
        }
        ms.messager_send(ss, &ms);

        char buf[1024];
        struct str_s ss_back = { buf, 1024, 0 };
        ms.messager_recv(ss_back, ss_back.len, &ms);
        ms.messager_close(&ms);
        return 0;
    }
}

/**
 * 由于插入数据时是按序插入，所以sort函数暂不实现
 */
void chash_value_sort(struct chash_store_s *t) {
    return ;
}
long chash_value_md5(int v, struct chash_store_s *t);

/**
 * 析构函数
 */
void chash_destructor(struct chash_store_s *t) {
    t->gossiper->gossiper_destructor(t->gossiper);
    free(t->value_store);
    free(t);
}

int host_handler(struct chash_store_s *t, char *s) {
    uint32_t skip = 0;
    int gap = sizeof(struct host_state_s);

    while (*(s + skip) != '\0') {
        struct host_state_s hs;
        memcpy(&hs, s+skip, gap);
        if (t->host_push(hs.host, t) < 0) {
            fprintf(stderr, "[error]host_handler: host push\n");
            return -1;
        }
        t->gossiper->gossiper_compare_update(hs, t->gossiper);
        skip += gap;
    }

    return 0;
}

/*
 * 当前节点的数据有多少属于host节点
 */
int  chash_cnt_belongs2(char *host, struct chash_store_s *t) {
    struct chash_host node;
    node.ipv4 = host;
    node.md5 = t->host_md5(host, t);

    int i, cnt = 0;
    for (i = 0; i < t->n_value; i++) {
        if (t->value_store[i].md5 >= node.md5) {
            cnt++;
        }
    }

    return cnt;
}

/*
 * 返回当前节点的数据属于host的数据
 */
int* chash_data_belongs2(char *host, struct chash_store_s *t) {
    int cnt = t->cnt_belongs2(host, t);
    long md5 = t->host_md5(host, t);
    int *vs = malloc(sizeof(int) * cnt);
    if (!vs) {
        return vs;
    }

    int i, j = 0;
    for (i = 0; i < t->n_value; i++) {
        if (t->value_store[i].md5 >= md5) {
            vs[j++] = t->value_store[i].value;
        }
    }

    return vs;
}

/*
 * 判断v属于哪个节点，返回节点下标
 */
int  v_belongs2(int v, struct chash_store_s *cs) {
    struct value_store_s vs;
    vs.value = v;
    vs.md5 = cs->value_md5(v, cs);
    uint32_t b, i, j;

    /* 找到对应节点 */
    for (b = 0; b < cs->n_host; b++) {
        if (cs->hosts[b].md5 > vs.md5) {
            break;
        }
    }

    if (b == cs->n_host || b == 0) {
        b = cs->n_host - 1;
    } else {
        b--;
    }

    return b;
}

void chash_value_printf(struct chash_store_s *t) {
    int i, carry = 10, cnt = 1;
    char *split_line = "---------------------------------------";
    fprintf(stdout, "%s\n", split_line);
    for (i = 0; i < t->n_value; i++) {
        if (cnt % carry == 0) {
            fprintf(stdout, "\n");
            cnt = 1;
        }
        fprintf(stdout, "%d ", t->value_store[i].value);
    }
    fprintf(stdout, "%s\n", split_line);
}
