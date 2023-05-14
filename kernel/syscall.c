/*
 * contains the implementation of all syscalls.
 */

#include <stdint.h>
#include <errno.h>

#include "util/types.h"
#include "syscall.h"
#include "string.h"
#include "process.h"
#include "util/functions.h"
#include "elf.h"

#include "spike_interface/spike_utils.h"

extern elf_symbol symbols[64];
extern char sym_names[64][32];
extern int sym_count;

//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  sprint(buf);
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  sprint("User exit with code:%d.\n", code);
  // in lab1, PKE considers only one app (one process). 
  // therefore, shutdown the system when the app calls exit()
  shutdown(code);
}


int func_name_printer(uint64 ret_addr) {
  for(int i=0;i<sym_count;i++){
    //sprint("%d %d %d\n", ret_addr, symbols[i].st_value, symbols[i].st_size);
    if(ret_addr >= symbols[i].st_value && ret_addr < symbols[i].st_value+symbols[i].st_size){
      sprint("%s\n",sym_names[i]);
      if(strcmp(sym_names[i],"main")==0) return 0;
      return 1;
    }
  }
  return 1;
}


ssize_t sys_backtrace(uint64 depth) {
  //sprint("this is sys_backtrace\n");
  uint64 trace_sp = current->trapframe->regs.sp + 32;
  uint64 trace_ra = trace_sp + 8;
  uint64 n = 0;
  for(; n<depth; n++) {
    if(func_name_printer(*(uint64*)trace_ra) == 0) return n;
    trace_ra += 16;
  }
  return n;
}

//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
  switch (a0) {
    case SYS_user_print:
      return sys_user_print((const char*)a1, a2);
    case SYS_user_exit:
      return sys_user_exit(a1);
    case SYS_user_backtrace:
      return sys_backtrace(a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
