#ifndef _CONFIG_H
#define _CONFIG_H

#define USER_NAME "root"
#define USER_PASSWORD "123456"

//#define DISK_SIZE (100 * 1024 * 1024) //磁盘大小，100M，100*1024个磁盘块
#define DISK_SIZE (4 * 1024 * 1024) //适当改小，便于调试

#define BLOCK_SIZE 1024        //磁盘块大小，1kb
#define INODE_NUM 1024         //索引节点个数，文件个数
#define BLOCK_NUM (100 * 1024) // 100M = 100 * 1024 KB

#define FILE_MAX_SIZE (8 * 1024 + 2 * 1024 * 1024) //文件最大长度(字节)

#define INODE_MAP_SIZE 1024 //索引节点位图，字节为单位，凑齐1个磁盘块
#define BLOCK_MAP_SIZE (100 * 1024) //磁盘块位图，字节为单位

//磁盘分布
#define SUPER_START 0 //占用2个磁盘块，存放系统引导信息
#define INODE_MAP_START (2 * 1024) //占用1个磁盘块，存放索引节点使用情况
#define BLOCK_MAP_START (3 * 1024) //占用1024个磁盘块，存放空闲磁盘块号栈
#define INODE_DATA_START (1027 * 1024) //占用1024个磁盘块，存放索引节点信息
#define BLOCK_DATA_START (2051 * 1024) //占用剩余的磁盘块，存放文件以及目录项

//空闲块采用堆栈成组链接管理
#define BLOCK_STACK_SIZE 256 //每一组栈的大小

//其它定义
#define FILE_SYS_NAME "liuyang.fs"
#define MAX_NAME_LENGTH 28 //提醒用户不要超过，配合当前目录使用
#define MAX_PATH_LENGTH 256 //路径名最长字符长度
#define ERROR_ALLOC (-1) //空间不足，分配失败
#define USED 1
#define NOT_USED 0
#define ERROR_INDEX (-1)
#define MAX_ITEM_NUM 32 //一个目录含有的最多文件
#define DIR_FULL (-1)
#define MAX_CMD_LENGTH 100
#define MAX_ARG_NUM 10 //参数最多数量
#define NOT_EXIST (-1)
#define DIR_NOT_EMPTY (-2)
#define ILLEGAL_RMDIR (-3)
#define RM_SUCCESS (1)
#define RM_A_DIR (-4)

#endif
