#include "../inc/reg.h"

/*In use? Indexes correspond to regXXX definitions
  Note rsp and rbp always used*/
bool regs[] = {false, false, false, false, false, false, false, false,
               false, false, false, false, false, false, false, true,
               true};

char* regNames[] = {"register", "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8",
                    "r9", "r10", "r11", "r12", "r13", "r14", "r15", "rbp",
                    "rsp"};

bool regIsUsed (regClass reg) {
    return regs[reg];
}

regClass regRequest (regClass reg) {
    if (regs[reg] == false) {
        regs[reg] = true;
        return regUndefined;

    } else
        return reg;
}

void regFree (regClass reg) {
    if (reg != regRBP)
        regs[reg] = false;
}

void regFreeAll () {
    for (regClass i = regRAX; i < regMax; i++)
        regs[i] = false;
}

/**
 * Allocate a general register. Returns the register allocated if any.
 */
regClass regAllocGeneral () {
    for (regClass i = regRBX; i <= regR15; i++)	//Bugger RAX. Functions put their rets in there, so its just a hassle
        if (!regRequest(i))
            return i;

    return regRequest(regRAX);
}

char* regToStr (regClass Reg) {
    return regNames[Reg];
}
