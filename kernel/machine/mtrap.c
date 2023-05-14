#include "kernel/riscv.h"
#include "kernel/process.h"
#include "spike_interface/spike_utils.h"

#include "spike_interface/spike_file.h"
#include <string.h>

char full_path[256];
char full_file[5120];
struct stat file_stat;
void error_displayer() {
  uint64 exp_addr = read_csr(mepc);
  for(int i=0; i<current->line_ind; i++) 
  {
    if(exp_addr < current->line[i].addr)
    {
      addr_line *exp = current->line + i - 1;
      int dirlen = strlen(current->dir[current->file[exp->file].dir]);   
      strcpy(full_path, current->dir[current->file[exp->file].dir]);
      full_path[dirlen] = '/';
      strcpy(full_path+dirlen+1, current->file[exp->file].file);   
      spike_file_t * _file_ = spike_file_open(full_path, O_RDONLY, 0);
      spike_file_stat(_file_, &file_stat);
      spike_file_read(_file_, full_file, file_stat.st_size);
      spike_file_close(_file_);
      int offset = 0, count = 0;
      while (offset < file_stat.st_size) 
      {
        int tmp = offset;
        while (tmp < file_stat.st_size && full_file[tmp] != '\n') tmp++;
        if (count == exp->line - 1) 
        {
          full_file[tmp] = '\0';
          sprint("Runtime error at %s:%d\n%s\n", full_path, exp->line, full_file + offset);
          break;
        } 
        else
        {
          count++;
          offset = tmp + 1;
        }
      }
      break;
    }
  }
}

static void handle_instruction_access_fault() {error_displayer(); panic("Instruction access fault!"); }

static void handle_load_access_fault() {error_displayer(); panic("Load access fault!"); }

static void handle_store_access_fault() {error_displayer(); panic("Store/AMO access fault!"); }

static void handle_illegal_instruction() {error_displayer(); panic("Illegal instruction!"); }

static void handle_misaligned_load() {error_displayer(); panic("Misaligned Load!"); }

static void handle_misaligned_store() {error_displayer(); panic("Misaligned AMO!"); }


// added @lab1_3
static void handle_timer() {
  int cpuid = 0;
  // setup the timer fired at next time (TIMER_INTERVAL from now)
  *(uint64*)CLINT_MTIMECMP(cpuid) = *(uint64*)CLINT_MTIMECMP(cpuid) + TIMER_INTERVAL;

  // setup a soft interrupt in sip (S-mode Interrupt Pending) to be handled in S-mode
  write_csr(sip, SIP_SSIP);
}

//
// handle_mtrap calls a handling function according to the type of a machine mode interrupt (trap).
//
void handle_mtrap() {
  uint64 mcause = read_csr(mcause);
  switch (mcause) {
    case CAUSE_MTIMER:
      handle_timer();
      break;
    case CAUSE_FETCH_ACCESS:
      handle_instruction_access_fault();
      break;
    case CAUSE_LOAD_ACCESS:
      handle_load_access_fault();
    case CAUSE_STORE_ACCESS:
      handle_store_access_fault();
      break;
    case CAUSE_ILLEGAL_INSTRUCTION:
      // TODO (lab1_2): call handle_illegal_instruction to implement illegal instruction
      // interception, and finish lab1_2.
      //panic( "call handle_illegal_instruction to accomplish illegal instruction interception for lab1_2.\n" );
      handle_illegal_instruction();
      break;
    case CAUSE_MISALIGNED_LOAD:
      handle_misaligned_load();
      break;
    case CAUSE_MISALIGNED_STORE:
      handle_misaligned_store();
      break;

    default:
      sprint("machine trap(): unexpected mscause %p\n", mcause);
      sprint("            mepc=%p mtval=%p\n", read_csr(mepc), read_csr(mtval));
      panic( "unexpected exception happened in M-mode.\n" );
      break;
  }
}
