#ifndef _DATA_STRUCTURE_H
#define _DATA_STRUCTURE_H

#include "config_sh.h"
#include <time.h> //for time_t

//超级块，记录文件系统基本信息，占用最开始的两个磁盘块
typedef struct super_block {
  int free_inode_num; //空闲索引节点数
  int free_blk_num;   //空闲磁盘块数

  int blk_map_index; //下一个存放页号堆栈的磁盘块地址
  int free_top;      //空闲块堆栈指针
  int free_blk_stack[BLOCK_STACK_SIZE]; //空闲块堆栈

  int root_index; //根目录节点索引
} super_block;

//索引节点，记录文件信息，每个inode占用一个磁盘块
typedef struct index_node {
  int item_num;                 // 0表示文件，其他表示目录项数
  char f_name[MAX_NAME_LENGTH]; //文件名或目录名

  long f_size; //文件大小

  int has_indirect;   //使用int便于对齐，方便调试
  int node_index[10]; //前8个一级，后2个二级，8kb+2MB
} index_node;

//文件目录项
typedef struct dir_item { //总共32字节，一个磁盘块存储1024/32=32个目录项
  char item_name[MAX_NAME_LENGTH]; //目录名或者文件名
  int inode_index;                 //目录项对应索引节点地址
} dir_item;

#endif
