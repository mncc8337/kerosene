#include "system.h"

extern bool check_a20();
extern bool query_a20_support();
extern void enable_a20_keyboard_controller();
extern void enable_a20_fast_gate();

bool enable_a20() {
    if(check_a20()) return true;

    bool fast_gate_support = query_a20_support();

    if(fast_gate_support) {
        enable_a20_fast_gate();
        if(check_a20()) return true;
    }
    
    // if fast gate failed or is not supported
    enable_a20_keyboard_controller();
    return check_a20();
}
