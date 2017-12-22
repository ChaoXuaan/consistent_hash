/*
 * config.h
 *
 *  Created on: Dec 18, 2017
 *      Author: dmcl216
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#define SRVPORT 	35696
#define LISTEN_NUM  64
#define TESTIP  	"127.0.0.1"
#define LOCAL_HOST 	"192.168.0.105"
#define SRV_HOST   	"192.168.0.122"
#define SEED_HOST   "192.168.0.122"

// command
#define GOSSIP_HEADER "gossip"
#define G_HEADER_SIZE  6     // strlen(HEADER)
#define NODE_INSERT "node_insert"
#define NODE_DELETE "node_delete"
#define DATA_INSERT "data_insert"
#define DATA_QUERY  "data_query"
#define DATA_DELETE "data_delete"
#define DATA_UPDATE "data_update"
#define INSERT_MIGRATE "insert_migrate"
#define DELETE_MIGRATE "delete_migrate"

#endif /* CONFIG_H_ */
