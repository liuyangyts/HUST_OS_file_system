#include "fs_shell.h"
#include "config_sh.h"
#include "data_structure.h"
#include "global.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <fstream>

super_block *sp_blk;                //超级块
FILE *fp_disk;                      //虚拟磁盘文件指针
char inode_map[INODE_NUM];          //索引节点位图
index_node root_node;               //根目录索引节点信息
dir_item cur_table[MAX_ITEM_NUM];   //当前目录的所有文件
index_node cur_node;                //当前目录索引节点信息
int cur_dir_index;                  //当前目录索引
char cur_dir_name[MAX_PATH_LENGTH]; //当前目录名

index_node sub_node;                //临时文件节点，用于cp操作
int sub_node_num;                   //临时文件分配的索引节点号
//int sub_block_num;                  //临时文件分配的磁盘块号

//加载文件系统信息
//如果不存在则直接创建模拟磁盘
bool init_fs() {
  //以追加方式"r+"打开模拟磁盘文件
  printf("打开模拟磁盘文件%s\n", FILE_SYS_NAME);
  fp_disk = fopen(FILE_SYS_NAME, "r+");
  if (fp_disk == NULL) { //文件不存在
    printf("文件%s不存在，格式化磁盘\n", FILE_SYS_NAME);
    format_fs();
    printf("文件%s创建完成，请重新启动本系统\n",FILE_SYS_NAME);
    exit(-1);
  }
  printf("打开%s完成\n", FILE_SYS_NAME);
  //加载超级块
  printf("加载超级块\n");
  sp_blk = (super_block *)malloc(sizeof(super_block));
  if (sp_blk == NULL) {
    printf("error : allocate super block memory failed\n");
    exit(0);
    return false;
  }
  fseek(fp_disk, SUPER_START, SEEK_SET);
  fread(sp_blk, sizeof(super_block), 1, fp_disk);
  printf("超级块加载完成\n");
  //设置根目录
  printf("创建根目录\n");
  fseek(fp_disk, (sp_blk->root_index) * BLOCK_SIZE + INODE_DATA_START,
        SEEK_SET);
  fread(&root_node, sizeof(index_node), 1, fp_disk);
  printf("根目录创建完成\n");
  //设置当前目录为根目录
  if(strcmp(root_node.f_name,"")==0)
    printf("设置当前目录为根目录\n");
  else
    printf("设置当前目录为%s\n", root_node.f_name);
  cur_dir_index = sp_blk->root_index;
  memcpy(&cur_node, &root_node, sizeof(index_node));
  strcpy(cur_dir_name, root_node.f_name);
  //加载当前目录的目录项
  printf("加载当前目录\n");
  fseek(fp_disk, cur_node.node_index[0] * BLOCK_SIZE, SEEK_SET);
  fread(cur_table, sizeof(dir_item), 32, fp_disk);
  printf("当前目录加载完成\n");
  //加载索引节点位图
  printf("加载索引节点位图\n");
  fseek(fp_disk, INODE_MAP_START, SEEK_SET);
  fread(inode_map, sizeof(char), INODE_NUM, fp_disk);
  printf("索引节点位图加载完成\n");
  return true;
}

//退出文件系统
//释放超级块占用的资源
bool close_fs() {
  //将内存中的超级块信息写入磁盘
  printf("将内存中的超级块信息写入磁盘\n");
  fseek(fp_disk, SUPER_START, SEEK_SET);
  fwrite(sp_blk, sizeof(super_block), 1, fp_disk);
  printf("超级块写入成功\n");
  //将内存中的索引节点位图写入磁盘
  printf("将内存中的索引节点位图写入磁盘\n");
  fseek(fp_disk, INODE_MAP_START, SEEK_SET);
  fwrite(inode_map, sizeof(char), INODE_NUM, fp_disk);
  printf("索引节点位图写入成功\n");
  //释放资源
  printf("正在释放资源...\n");
  fclose(fp_disk);
  free(sp_blk);
  printf("关闭完成\n");
  return true;
}

//将一个100M的文件初始化为模拟磁盘
//设置当前目录为根目录
//根目录名字为空
bool format_fs() {
  char c_zero = 0;
  int i;
  fp_disk = fopen(FILE_SYS_NAME, "w");
  if (fp_disk == NULL) {
    int err_num = errno;
    printf("error : %d reason : %s\n", err_num, strerror(err_num));
    exit(err_num);
    return false;
  }

  //写入100M空间
  for (i = 0; i < DISK_SIZE; ++i) {
    fwrite(&c_zero, sizeof(char), 1, fp_disk);
  }
  printf("%d磁盘空间申请完成\n", DISK_SIZE);

  //初始化索引节点位图
  for (i = 0; i < INODE_NUM; ++i) {
    inode_map[i] = NOT_USED;
  }
  printf("索引节点位图初始化完成\n");

  //建立超级块，从后往前建立成组链接堆栈
  sp_blk = (super_block *)malloc(sizeof(super_block));
  if (sp_blk == NULL) {
    printf("error : super block allocation failed\n");
    return false;
  }
  sp_blk->blk_map_index = (INODE_DATA_START - BLOCK_SIZE) / BLOCK_SIZE;
  sp_blk->free_inode_num = INODE_NUM;
  sp_blk->free_blk_num = BLOCK_NUM - BLOCK_DATA_START / BLOCK_SIZE;
  sp_blk->free_top = 0;
  sp_blk->free_blk_stack[0] = -1; //最后一组的标识
  printf("建立空闲磁盘块号堆栈\n");
  for (i = BLOCK_NUM - 1; i >= (BLOCK_DATA_START) / BLOCK_SIZE; --i) {
    if (sp_blk->free_top == BLOCK_STACK_SIZE - 1) {
      fseek(fp_disk, (sp_blk->blk_map_index) * BLOCK_SIZE, SEEK_SET);
      fwrite(sp_blk->free_blk_stack, sizeof(int), BLOCK_STACK_SIZE, fp_disk);
      sp_blk->free_top = 0;
      sp_blk->free_blk_stack[sp_blk->free_top++] =
          (sp_blk->blk_map_index) / BLOCK_SIZE;
      sp_blk->blk_map_index -= 1;
    }
    sp_blk->free_blk_stack[++(sp_blk->free_top)] = i;
  }
  printf("超级块建立完成，空闲磁盘块号堆栈写入完成\n");

  //创建根目录
  printf("创建根目录\n");
  sp_blk->root_index = inode_alloc();
  root_node.f_size = BLOCK_SIZE;
  root_node.has_indirect = 0;
  strcpy(root_node.f_name, ""); //根目录名为空
  for (i = 0; i < 10; ++i) {
    root_node.node_index[i] = ERROR_INDEX;
  }
  root_node.node_index[0] = blk_alloc();
  //增加目录项"."，当前目录
  dir_item self_item;
  strcpy(self_item.item_name, ".");
  root_node.item_num = 1; //当前目录目录项.
  self_item.inode_index = sp_blk->root_index;
  //将目录项写入磁盘块数据区
  printf("根目录目录项写入磁盘块号：%d，16字节偏移量: %d\n",
         root_node.node_index[0], root_node.node_index[0] * BLOCK_SIZE / 16);
  fseek(fp_disk, root_node.node_index[0] * BLOCK_SIZE, SEEK_SET);
  fwrite(&self_item, sizeof(dir_item), 1, fp_disk);
  //将索引节点写入磁盘块索引节点区
  printf("根目录索引节点写入index：%d，16字节偏移量: %d\n", sp_blk->root_index,
         (sp_blk->root_index * BLOCK_SIZE + INODE_DATA_START) / 16);
  fseek(fp_disk, (sp_blk->root_index) * BLOCK_SIZE + INODE_DATA_START,
        SEEK_SET);
  fwrite(&root_node, sizeof(root_node), 1, fp_disk);
  //设置当前目录信息
  printf("设置当前目录为根目录\n");
  strcpy(cur_dir_name, ""); //根目录名字
  cur_dir_index = sp_blk->root_index;
  memcpy(&cur_node, &root_node, sizeof(index_node));
  memcpy(cur_table, &self_item, sizeof(dir_item));
  printf("根目录创建完成\n");

  printf("文件系统格式化成功\n");
  return true;
}

//分配一个空闲的索引节点
//返回索引节点号
int inode_alloc() {
  if (sp_blk->free_inode_num <= 0) {
    printf("error : no free index node!\n");
    exit(ERROR_ALLOC);
    return ERROR_ALLOC;
  }
  int i;
  for (i = 0; i < INODE_NUM; ++i) {
    if (inode_map[i] == NOT_USED) {
      //更新超级块
      sp_blk->free_inode_num--;
      //更新索引节点位图
      inode_map[i] = USED;
      printf("分配索引节点: %d\n", i);
      return i;
    }
  }
}

//分配一个空闲的磁盘块
//返回分配的磁盘块号
int blk_alloc() {
  if (sp_blk->free_blk_num <= 0) {
    printf("error : no free disk blk\n");
    exit(ERROR_ALLOC);
    return ERROR_ALLOC;
  }
  if (sp_blk->free_top > 0) { //从内存里取出磁盘号
    printf("从超级块内存里取出磁盘块号\n");
    sp_blk->free_blk_num--;
    printf("分配磁盘块号：%d\n", sp_blk->free_blk_stack[sp_blk->free_top]);
    printf("栈顶：%d\n", sp_blk->free_top);
    return sp_blk->free_blk_stack[sp_blk->free_top--];
  }
   else { //从磁盘中加载空闲栈
    printf("从磁盘加载空闲块号栈\n");
    fseek(fp_disk, sp_blk->free_blk_stack[0] * BLOCK_SIZE, SEEK_SET);
    fread(sp_blk->free_blk_stack, sizeof(int), BLOCK_STACK_SIZE, fp_disk);
    sp_blk->free_top = BLOCK_STACK_SIZE - 1;
    sp_blk->free_blk_num--;
    printf("分配磁盘块号：%d\n", sp_blk->free_blk_stack[sp_blk->free_top]);
    printf("栈顶：%d\n", sp_blk->free_top);
    return sp_blk->free_blk_stack[sp_blk->free_top];
  }
}

//回收已经分配的索引节点
//更新超级块
bool inode_free(int inode_index) {
  printf("回收索引节点: %d\n", inode_index);
  inode_map[inode_index] = NOT_USED;
  sp_blk->free_inode_num++;
  return true;
}

//回收已经分配出去的磁盘
//更新超级块
bool blk_free(int blk_index) {
  printf("回收磁盘块号: %d\n", blk_index);
  if (sp_blk->free_top == BLOCK_STACK_SIZE - 1) {
    //空闲块栈满，写入磁盘
    fseek(fp_disk, (sp_blk->blk_map_index) * BLOCK_SIZE, SEEK_SET);
    fwrite(sp_blk->free_blk_stack, sizeof(int), BLOCK_STACK_SIZE, fp_disk);
    //更新超级块
    sp_blk->free_top = 0;
    sp_blk->free_blk_stack[0] = sp_blk->blk_map_index;
    sp_blk->blk_map_index -= 1;
  }
  sp_blk->free_blk_stack[(sp_blk->free_top)++] = blk_index;
  sp_blk->free_blk_num++;
  return true;
}

//当前目录是否有该文件
//返回在目录项表格中的索引
int has_item(char *item_name) {
  for (int i = 0; i < cur_node.item_num; ++i) {
    if (strcmp(cur_table[i].item_name, item_name) == 0) {
      return i;
    }
  }
  return NOT_EXIST;
}

//当前目录添加目录项
//将添加得目录项写入内存
bool add_item(char *item_name, int node_idx) {
  printf("%s添加目录项%s\n", cur_node.f_name, item_name);
  strcpy(cur_table[cur_node.item_num].item_name, item_name);
  cur_table[cur_node.item_num].inode_index = node_idx;
  //当前目录目录项写入磁盘
  printf("将目录项写入磁盘块数据区：%d，16字节偏移量: %d\n",
         cur_node.node_index[0], cur_node.node_index[0] * BLOCK_SIZE / 16);
  fseek(fp_disk, cur_node.node_index[0] * BLOCK_SIZE, SEEK_SET);
  fwrite(cur_table, sizeof(dir_item), 32, fp_disk);
  //当前目录索引节点写入磁盘
  cur_node.item_num++;
  printf("%s目录项数目：%d\n", cur_node.f_name, cur_node.item_num);
  printf("父目录%s索引节点更新，写入index: %d，16字节偏移量: %d\n",
         cur_dir_name, cur_dir_index,
         (cur_dir_index * BLOCK_SIZE + INODE_DATA_START) / 16);
  fseek(fp_disk, cur_dir_index * BLOCK_SIZE + INODE_DATA_START, SEEK_SET);
  fwrite(&cur_node, sizeof(index_node), 1, fp_disk);
  return true;
}

bool rm_item(int table_idx) {
  printf("[debug in function rm_item] item_num before: %d\n",
         cur_node.item_num);
  //删除父目录目录项
  memcpy(&cur_table[table_idx], &cur_table[cur_node.item_num - 1],
         sizeof(dir_item));
  cur_node.item_num--;
  printf("[debug int function rm_item] item_num later: %d\n",
         cur_node.item_num);
  //更改后的目录项写入磁盘
  fseek(fp_disk, cur_node.node_index[0] * BLOCK_SIZE, SEEK_SET);
  fwrite(cur_table, sizeof(dir_item), 32, fp_disk);
  //更改后的索引节点写入索引节点区
  fseek(fp_disk, cur_dir_index * BLOCK_SIZE + INODE_DATA_START, SEEK_SET);
  fwrite(&cur_node, 1, sizeof(index_node), fp_disk);
  return true;
}

//在当前目录下创建新文件
//不支持重名文件
bool touch(char file_name[]) {
  int i;
  if (has_item(file_name) != NOT_EXIST) { //判断是否已有该文件
    printf("sorry, but the current directory has already had file %s\n",
           file_name);
    return false;
  }
  if (cur_node.item_num >= MAX_ITEM_NUM) { //目录满了
    printf("sorry, but this directory is full due to system design\n");
    return false;
  }
  //创建索引节点
  int node_idx = inode_alloc();
  index_node f_node;
  f_node.f_size = 0;
  f_node.item_num = 0;  //表示新建的是文件
  f_node.has_indirect = 0;
  
  for (i = 0; i < 10; ++i) {
    f_node.node_index[i] = ERROR_INDEX;
  }
  strcpy(f_node.f_name, file_name);
  //在父目录增加文件名
  add_item(file_name, node_idx);
  //将新文件索引节点写入inode data区
  printf("新文件索引节点写入index: %d，16字节偏移量: %d\n", node_idx,
         (node_idx * BLOCK_SIZE + INODE_DATA_START) / 16);
  fseek(fp_disk, node_idx * BLOCK_SIZE + INODE_DATA_START, SEEK_SET);
  fwrite(&f_node, sizeof(index_node), 1, fp_disk);
  return true;
}

//在当前目录下创建新目录项
bool mkdir(char dir_name[]) {
  if (has_item(dir_name) != NOT_EXIST) {
    printf("sorry, but the current directory has already had directory %s\n",
           dir_name);
    return false;
  }
  if (cur_node.item_num >= MAX_ITEM_NUM) {
    printf("sorry, but current directory is full due to system design\n");
    return false;
  }
  int i;
  printf("创建目录索引节点\n");
  int node_idx = inode_alloc();
  printf("分配磁盘空间\n");
  int blk_idx = blk_alloc();
  index_node d_node;
  strcpy(d_node.f_name, dir_name);
  d_node.f_size = BLOCK_SIZE;
  d_node.has_indirect = 0;
  for (i = 0; i < 10; ++i) {
    d_node.node_index[i] = ERROR_INDEX;
  }
  d_node.node_index[0] = blk_idx;
  dir_item two_item[2];
  //当前目录
  two_item[0].inode_index = node_idx;
  strcpy(two_item[0].item_name, ".");
  //父目录
  two_item[1].inode_index = cur_dir_index;
  strcpy(two_item[1].item_name, "..");
  d_node.item_num = 2;
  //在父目录增加目录项
  printf("添加目录项.和..\n");
  add_item(dir_name, node_idx);
  //将新目录索引节点写入inode_data
  printf("新创建目录写入索引节点: %d，16字节偏移量: %d\n", node_idx,
         (node_idx * BLOCK_SIZE + INODE_DATA_START) / 16);
  fseek(fp_disk, node_idx * BLOCK_SIZE + INODE_DATA_START, SEEK_SET);
  fwrite(&d_node, sizeof(index_node), 1, fp_disk);
  //将新目录目录项写入block_data
  printf("新创建目录目录项写入磁盘块号: %d，16字节偏移量: %d\n", blk_idx,
         blk_idx * BLOCK_SIZE / 16);
  fseek(fp_disk, blk_idx * BLOCK_SIZE, SEEK_SET);
  fwrite(two_item, sizeof(dir_item), 2, fp_disk);

  return true;
}

//返回当前目录下的所有文件名
std::vector<char *> ls() {
  std::vector<char *> file_list;
  for (int i = 0; i < cur_node.item_num; ++i) {
    file_list.push_back(cur_table[i].item_name);
  }
  return file_list;
}

//返回路径名
char *pwd() {
  char *rtn_pwd = (char *)malloc(sizeof(char) * MAX_PATH_LENGTH);
  if (strlen(cur_dir_name) == 0) { //根目录
    strcpy(rtn_pwd, "/");
  }
  else {  strcpy(rtn_pwd, cur_dir_name);
  }
  return rtn_pwd;
}

bool cd(char dir_name[]) {
  if (strcmp(dir_name, ".") == 0) {
    //进入当前目录，直接退出
    return true;
  }
  int i;
  for (i = 0; i < cur_node.item_num; ++i) {
    if (strcmp(cur_table[i].item_name, dir_name) == 0) {
      //找到该目录，切换
      printf("目录%s位于目录%s第%d项\n", dir_name, cur_dir_name, i);
      cur_dir_index = cur_table[i].inode_index;
      index_node d_node;
      //从磁盘索引节点数据区加载该索引节点信息
      printf("从目录表第%d项加载索引节点信息，16字节偏移量: %d\n",
             cur_table[i].inode_index,
             (cur_table[i].inode_index * BLOCK_SIZE + INODE_DATA_START) / 16);
      fseek(fp_disk, cur_table[i].inode_index * BLOCK_SIZE + INODE_DATA_START,
            SEEK_SET);
      fread(&d_node, sizeof(index_node), 1, fp_disk);
      printf("打开目录%s成功\n",d_node.f_name);
      //node_char_info(d_node); //打印索引节点字节信息
      //更新当前目录信息
      memcpy(&cur_node, &d_node, sizeof(index_node));
      //从磁盘加载目录项
      printf("从磁盘块号: %d加载%s目录项\n", d_node.node_index[0], dir_name);
      fseek(fp_disk, d_node.node_index[0] * BLOCK_SIZE, SEEK_SET);
      fread(cur_table, sizeof(dir_item), 32, fp_disk);
      if (strcmp(dir_name, "..") == 0) { //父目录
        //查找'/'最后出现的位置
        char *div_pointer = strrchr(cur_dir_name, '/');
        char *tmp_str = (char *)malloc(sizeof(char) * MAX_PATH_LENGTH);
        strcpy(tmp_str, cur_dir_name);
        memcpy(cur_dir_name, tmp_str, strlen(tmp_str) - strlen(div_pointer));
        cur_dir_name[strlen(tmp_str) - strlen(div_pointer)] = 0;
      } else { //子目录
      //在当前路径后加上/目录名
        strcpy(cur_dir_name, strcat(cur_dir_name, "/"));
        strcpy(cur_dir_name, strcat(cur_dir_name, dir_name));
      }
      printf("成功进入%s\n", d_node.f_name);
      return true;
    }
  }
  printf("error dir name!\n");
  return false;
}

//命令行命令解析
bool cmd() {
  while (true) {
    int i, j;
    printf("[root@LY F-S: %s]$ ", cur_node.f_name);
    char cmd_str[MAX_CMD_LENGTH];
    fgets(cmd_str, MAX_CMD_LENGTH, stdin);
    cmd_str[strlen(cmd_str) - 1] = 0; //去掉回车
    int argc;
    char *argv[MAX_ARG_NUM];
    //解析命令参数
    cmd_parser(cmd_str, argc, argv);
    //用strcasecmp忽略大小写差异
    if (strcasecmp(argv[0], "ls") == 0) {
      std::vector<char *> file_list = ls();
      for (j = 0; j < file_list.size(); ++j) {
        printf("%s\n", file_list[j]);
      }
      printf("file list size: %ld, item num: %d\n", file_list.size(),
             cur_node.item_num);
    } 
    else if (strcasecmp(argv[0], "pwd") == 0) {
      printf("%s\n", pwd());
    } 
    else if (strcasecmp(argv[0], "touch") == 0) {
      for (i = 1; i < argc; ++i)
        touch(argv[i]);
    } 
    else if (strcasecmp(argv[0], "mkdir") == 0) {
      for (i = 1; i < argc; ++i)
        mkdir(argv[i]);
    } 
    else if (strcasecmp(argv[0], "cd") == 0) {
      if (argc > 2) {
        printf("error argument number for cd\n");
      } else {
        cd(argv[1]);
      }
    } 
    else if (strcasecmp(argv[0], "exit") == 0) {
      return true;
    } 
    else if (strcasecmp(argv[0], "rm") == 0) {
      for (i = 1; i < argc; ++i) {
        int rtn_rm = rm(argv[i]);
        if (rtn_rm == NOT_EXIST) {
          printf("文件%s不存在，删除失败\n", argv[i]);
        } else if (rtn_rm == RM_A_DIR) {
          printf("%s是一个目录\n", argv[i]);
        } else {
          printf("删除文件%s成功\n", argv[i]);
        }
      }
    } 
    else if (strcasecmp(argv[0], "rmdir") == 0) {
      for (i = 1; i < argc; ++i) {
        int rtn_rmdir = rmdir(argv[i]);
        if (rtn_rmdir == NOT_EXIST) {
          printf("目录%s不存在，删除失败\n", argv[i]);
        } else if (rtn_rmdir == DIR_NOT_EMPTY) {
          printf("目录%s非空，删除失败\n", argv[i]);
        } else if (rtn_rmdir == ILLEGAL_RMDIR) {
          printf("删除非法目录%s，删除失败\n", argv[i]);
        } else {
          printf("删除目录%s成功\n", argv[i]);
        }
      }
    } 
    else if (strcasecmp(argv[0],"help")==0){
      help();
    }
    else if (strcasecmp(argv[0],"cp")==0){
      if(argc>2)
        printf("error: incorrect input\n");
      else
        cp(argv[1]);
    }
    else if (strcasecmp(argv[0],"paste")==0){
      if(argc==1){
        paste();
      }
      else{
        printf("error: incorrect input\n");
      }
    }
    else {
      printf("error : command not surported now!\n");
    }
  }
}

//解析用户输入的字符串，获得命令
bool cmd_parser(char *cmd_str, int &argc, char *argv[]) {
  argc = 0;
  char *token = strtok(cmd_str, " ");
  while (token != NULL) {
    argv[argc] = (char *)malloc(sizeof(char) * (strlen(token) + 1));
    strcpy(argv[argc++], token);
    token = strtok(NULL, " ");
  }
  return true;
}

//打印index_node字节信息
bool node_char_info(const index_node node) {
  unsigned int *pi = (unsigned int *)(&node);
  int len = sizeof(index_node) / sizeof(int);
  for (int i = 0; i < len; ++i) {
    printf("%x", *(pi + i));
  }
  printf("\n");
  return true;
}

int rm(char file_name[]) {
  int i, j;
  i = has_item(file_name);
  if (i == NOT_EXIST) { //当前目录不存在该文件
    return NOT_EXIST;
  } else {
    int node_idx = cur_table[i].inode_index;
    index_node f_node;
    //加载索引节点，从索引节点释放磁盘块信息
    fseek(fp_disk, BLOCK_SIZE * node_idx + INODE_DATA_START, SEEK_SET);
    fread(&f_node, 1, sizeof(index_node), fp_disk);
    int blk_idx;
    if (f_node.has_indirect) { // TODO:文件采用二级间接索引
    } else {                   //文件采用一级直接索引
      j = 0;
      while (f_node.f_size > 0) { //释放所有的磁盘块
        blk_idx = f_node.node_index[j];
        blk_free(blk_idx);
        j++;
        f_node.f_size -= BLOCK_SIZE;
      }
    } //文件对应的磁盘已经全部释放
    //释放文件对应的索引节点
    inode_free(node_idx);
    rm_item(i);
    return RM_SUCCESS;
  }
}

int rmdir(char dir_name[]) {
  int i;
  i = has_item(dir_name);
  if (i == NOT_EXIST) {
    return NOT_EXIST;
  } else if ((strcmp(dir_name, ".") == 0) || (strcmp(dir_name, "..") == 0)) {
    return ILLEGAL_RMDIR;
  } else {
    index_node d_node;
    int node_idx = cur_table[i].inode_index;
    fseek(fp_disk, node_idx * BLOCK_SIZE + INODE_DATA_START, SEEK_SET);
    fread(&d_node, 1, sizeof(index_node), fp_disk);
    if (d_node.item_num > 2) { //目录非空，目录项.和..
      return DIR_NOT_EMPTY;
    } else { //应该可以直接调用rm
      return rm(dir_name);
    }
  }
}

void help(){
  printf("┌——————————————————————————————————————┐\n");
  printf("|------welcome to my file system-------|\n");
  printf("|-------made by: CS1603 liuyang--------|\n");
  printf("|-------------QQ:2593178774------------|\n");
  printf("├——————————————————————————————————————┤\n");
  printf("|------------help message--------------|\n");
  printf("|ls:list all file and dir--------------|\n");
  printf("|pwd:show the path of current dir------|\n");
  printf("|touch: touch 'filename1' 'filename2'--|\n");
  printf("|mkdir: mkdir 'dirname1' 'dirname2'----|\n");
  printf("|cd: cd 'dirname'----------------------|\n");
  printf("|exit: stop and exit this file system--|\n");
  printf("|rm: rm 'filename1' 'filename2'--------|\n");
  printf("|rmdir: rmdir 'dirname1' 'dirname2'----|\n");
  printf("|cp: cp 'filename'---------------------|\n");
  printf("|paste: paste what you copyed----------|\n");
  printf("|help:show this message----------------|\n");
  printf("└——————————————————————————————————————┘\n");
}

//登录界面
void login(){
  char username[10];
  char password[20];
  while(1){
    printf("please input username and password\n");
    printf("Username(no more than 10 words):");
    fgets(username,10,stdin);
    printf("password(no more than 20 words):");
    fgets(password,20,stdin);
    username[strlen(username)-1]=0;
    password[strlen(password)-1]=0;
    if((strcasecmp(username,USER_NAME)==0)&&(strcasecmp(password,USER_PASSWORD)==0))
      return;
    else
    {
      printf("error: username or password not correct\n");
    }
  }
}

//将文件复制到临时节点上，暂时先不粘贴
bool cp(char file_name[]){
  int node_source = has_item(file_name);
  if(node_source==NOT_EXIST){
    printf("sorry, current dir don't have this file");
    return false;
  }
  int i;
  printf("分配临时文件索引节点\n");
  sub_node_num = inode_alloc();
  fseek(fp_disk,sub_node_num*BLOCK_SIZE+INODE_DATA_START,SEEK_SET);
  fread(&sub_node,sizeof(index_node),1,fp_disk);
  strcpy(sub_node.f_name,file_name);
  printf("复制成功\n");
  printf("name: %s size: %ld\n",sub_node.f_name,sub_node.f_size);
  // printf("分配磁盘块\n");
  // sub_block_num = blk_alloc();
  // fseek(fp_disk,sub_node.node_index[0]*BLOCK_SIZE+BLOCK_DATA_START,SEEK_SET);
  // fread
  return true;
}

//将文件粘贴到当前目录下
bool paste(){
  printf("正将文件粘贴到当前目录下...\n");
  add_item(sub_node.f_name,sub_node_num);
  printf("正在更新磁盘信息\n");
  fseek(fp_disk, sub_node_num * BLOCK_SIZE + INODE_DATA_START, SEEK_SET);
  fwrite(&sub_node, sizeof(index_node), 1, fp_disk);
  printf("粘贴成功\n");
  inode_free(sub_node_num);
  printf("回收临时索引节点成功\n");
  return true;
}