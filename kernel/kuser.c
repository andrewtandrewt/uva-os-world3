/* in-kernel launcher for user tasks, e.g. sh or snes

    "pseudo" user tasks, compiled into the kernel, but executed at EL0 and in
    their own VA. besides syscalls, CANNOT call any kernel functions -- otherwise
    will trigger memory protection error

    in general, the programming environment is very limited, b/c everything
    needs to be re-implemented (e.g. printf())

    So just as a proof-of-concept demonstrating EL and VM concepts. Beyond that,
    nothing much to be done here. The real fun stuffs have to be standalone user
    programs loaded via exec() and linked to real user libs.
 */

/* The tricky parts are:
    1. The symbols referenced must be compiled into this unit, between user_begin/user_end.
        - Cannot call a kernel function defined outside.
        - Cannot reference a constant string which may also be linked outside.
    2. All references to the symbols must be PC-relative, not absolute addresses.
        - This is challenging as we often don't have such fine control of the C compiler.
        - Using -O0 often works, but -O2 may break things.
        - Writing in ASM lacks control of where the linker places things, leading to linker complaints about unencodable offsets.
        - See stawged/testuser_launch.S and user-test.c for reference. */

#include "kuser_sys.h"

// cannot call kernel's strlen which may emit long jmp with absolute addr
static unsigned int user_strlen(const char *s)    
{
  int n;
  for(n = 0; s[n]; n++)
    ;
  return n;
}

static void print_to_console(char *msg) {
	call_sys_write(1 /*stdout*/, msg, user_strlen(msg)); 
}

void user_process_hello() {
    print_to_console("Hello world from a user taskn\r");
}

void user_process_mario() {
    print_to_console("It's me, Mario!\n\r");
    call_sys_exec(0 /*path, ignored*/, 0 /*argv, ignored*/); // exec0()-> [usr] nes0
}

/* -----------------   printer tasks ------------------ 
  fork, then print in inf loop */
static void loop1(char *str) {
	for (;;)
		print_to_console(str);
}

void user_process_printers() {    
    print_to_console("User process entry\n\r");

    int pid = call_sys_fork();
    if (pid < 0) {
        print_to_console("Error during fork\n\r");
        call_sys_exit(1);
        return;
    }
    print_to_console("fork() succeeds\n\r");

    if (pid == 0) {
        loop1("abcde");
    } else {
        loop1("12345");
    }
}

/* -----------------------------------------------------*/

// for testing. works but not in use
// accepting args which are populated by move_to_user_mode()
// x0-x7 (per arm64 calling convention)
//  unpopulated args have garbage values
void user_process0(unsigned long arg0,
                   unsigned long arg1,
                   unsigned long arg2,
                   unsigned long arg3,
                   unsigned long arg4,
                   unsigned long arg5,
                   unsigned long arg6,
                   unsigned long arg7) {
    print_to_console("User process entry\n\r");
    call_sys_exec(0 /*ignored*/, 0 /*ignored*/); // will call nes0
}