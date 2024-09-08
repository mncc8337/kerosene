#include "process.h"

static volatile process_t* current_process;
static volatile process_t* process_list;

void process_fork() {}
void process_exec() {}
void process_switch() {}
void process_init() {}
