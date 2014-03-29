#include "../inc/reg.h"

#include "../inc/debug.h"

/*Indexes correspond to regXXX definitions
  Note rsp and rbp always used*/
reg regs[regMax] = {
    {1, {"undefined", "undefined", "undefined", "undefined"}, 0},
    {1, {"ah", "ax", "eax", "rax"}, 0},
    {1, {"ah", "bx", "ebx", "rbx"}, 0},
    {1, {"ch", "cx", "ecx", "rcx"}, 0},
    {1, {"dh", "dx", "edx", "rdx"}, 0},
    {2, {0, "si", "esi", "rsi"}, 0},
    {2, {0, "di", "edi", "rdi"}, 0},
    {8, {0, 0, 0, "r8"}, 0},
    {8, {0, 0, 0, "r9"}, 0},
    {8, {0, 0, 0, "r10"}, 0},
    {8, {0, 0, 0, "r11"}, 0},
    {8, {0, 0, 0, "r12"}, 0},
    {8, {0, 0, 0, "r13"}, 0},
    {8, {0, 0, 0, "r14"}, 0},
    {8, {0, 0, 0, "r15"}, 0},
    {2, {0, "bp", "ebp", "rbp"}, 0},
    {2, {0, "sp", "esp", "rsp"}, 0}
};

bool regIsUsed (regIndex r) {
    return regs[r].allocatedAs != 0;
}

reg* regRequest (regIndex r, int size) {
    if (regs[r].allocatedAs == 0 && regs[r].size <= size) {
        regs[r].allocatedAs = size;
        return &regs[r];

    } else
        return 0;
}

void regFree (reg* r) {
    r->allocatedAs = false;
}

reg* regAlloc (int size) {
    if (size == 0)
        return 0;

    /*Bugger RAX. Functions put their rets in there, so its just a hassle*/
    for (regIndex r = regRBX; r <= regR15; r++)
        if (regRequest(r, size) != 0)
            return &regs[r];

    if (regIsUsed(regRAX))
        debugError("regAlloc", "no registers left");

    return regRequest(regRAX, size);
}

const char* regToStr (reg* r) {
    if (r->allocatedAs == 1)
        return r->names[0];

    else if (r->allocatedAs == 2)
        return r->names[1];

    else if (r->allocatedAs == 4)
        return r->names[2];

    else if (r->allocatedAs == 8)
        return r->names[3];

    else {
        debugErrorUnhandledInt("regToStr", "register size", r->allocatedAs);
        return r->names[3];
    }
}
