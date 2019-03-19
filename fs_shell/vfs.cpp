#include "fs_shell.h"
#include <stdio.h>
#include <stdlib.h>



int main(void) {
  login();
  init_fs();
  cmd();
  close_fs();
  return 0;
}