#ifndef _FS_SHELL_H
#define _FS_SHELL_H

#include "data_structure.h"
#include "global.h"
#include <vector>

bool node_char_info(const index_node node);

bool format_fs(); //格式化
bool init_fs();   //初始化，从磁盘加载系统信息
bool close_fs();  //退出文件系统

int blk_alloc();                  //磁盘块分配
bool blk_free(int blk_index);     //释放回收磁盘块
int inode_alloc();                //索引节点分配
bool inode_free(int inode_index); //释放回收索引节点

int has_item(char *item_name);                //当前目录是否有该文件
bool add_item(char *item_name, int node_idx); //在当前目录添加目录项
bool rm_item(int table_idx);                  //在当前目录删除目录项

bool touch(char file_name[]); //创建文件
std::vector<char *> ls();     //显示当前目录的所有文件
char *pwd();                  //显示当前目录
bool cd(char dir_name[]);     //切换工作目录
bool mkdir(char dir_name[]);  //创建目录

int rm(char file_name[]);   //删除文件
int rmdir(char dir_name[]); //删除文件夹

bool cp(char file_name[]);   //复制系统内文件
bool paste();               //粘贴

bool cmd();                                              //处理输入的命令
bool cmd_parser(char *cmd_str, int &argc, char *argv[]); //解析命令行

void help();                //显示帮助信息

void login();    //登录界面

#endif
