#ifndef _GLOBAL_H
#define _GLOBAL_H

#include "data_structure.h"
#include <stdio.h>
#include <stdlib.h>

//全局变量声明
extern super_block *sp_blk;                // 超级块
extern FILE *fp_disk;                      // 虚拟磁盘文件指针
extern char inode_map[INODE_NUM];          // 索引节点位图
extern int cur_dir_index;                  // 当前目录索引
extern index_node cur_node;                //当前目录索引节点信息
extern dir_item cur_table[MAX_ITEM_NUM];   // 当前目录下的所有文件
extern index_node root_node;               //根目录索引节点信息
extern char cur_dir_name[MAX_PATH_LENGTH]; //当前目录名

#endif
