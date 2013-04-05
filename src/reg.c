#include "../inc/reg.h"

/*In use? Indexes correspond to regXXX definitions
  Note rsp and rbp always used*/
bool regs[] = {false, false, false, false, false, false, false, false,
               false, false, false, false, false, false, false, true,
               true};

char* regNames[] = {"register", "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8",
                    "r9", "r10", "r11", "r12", "r13", "r14", "r15", "rbp",
                    "rsp"};

bool regIsUsed (regTag reg) {
    return regs[reg];
}

regTag regRequest (regTag reg) {
    if (regs[reg] == false) {
        regs[reg] = true;
        return regUndefined;

    } else
        return reg;
}

void regFree (regTag reg) {
    if (reg != regRBP)
        regs[reg] = false;
}

void regFreeAll () {
    for (regTag i = regRAX; i < regMax; i++)
        regs[i] = false;
}

/**
 * Allocate a general register. Returns the register allocated if any.
 */
regTag regAllocGeneral () {
    for (regTag i = regRBX; i <= regR15; i++)	//Bugger RAX. Functions put their rets in there, so its just a hassle
        if (!regRequest(i))
            return i;

    return regRequest(regRAX);
}

char* regToStr (regTag Reg) {
    return regNames[Reg];
}
