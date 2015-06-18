#include "../inc/reg.h"

#include "../inc/debug.h"

#include "stdlib.h"

using "../inc/reg.h";

using "../inc/debug.h";

using "stdlib.h";

/*Indexes correspond to regXXX definitions
  Note rsp and rbp always used*/
reg* regs = 0;

void regInit (void) {
    regs = calloc(regMax, sizeof(reg));

    regs[0] = (reg) {1, {"undefined", "undefined", "undefined", "undefined"}, 0};
    regs[1] = (reg) {1, {"al", "ax", "eax", "rax"}, 0};
    regs[2] = (reg) {1, {"al", "bx", "ebx", "rbx"}, 0};
    regs[3] = (reg) {1, {"cl", "cx", "ecx", "rcx"}, 0};
    regs[4] = (reg) {1, {"dl", "dx", "edx", "rdx"}, 0};
    regs[5] = (reg) {2, {0, "si", "esi", "rsi"}, 0};
    regs[6] = (reg) {2, {0, "di", "edi", "rdi"}, 0};
    regs[7] = (reg) {8, {0, 0, 0, "r8"}, 0};
    regs[8] = (reg) {8, {0, 0, 0, "r9"}, 0};
    regs[9] = (reg) {8, {0, 0, 0, "r10"}, 0};
    regs[10] = (reg) {8, {0, 0, 0, "r11"}, 0};
    regs[11] = (reg) {8, {0, 0, 0, "r12"}, 0};
    regs[12] = (reg) {8, {0, 0, 0, "r13"}, 0};
    regs[13] = (reg) {8, {0, 0, 0, "r14"}, 0};
    regs[14] = (reg) {8, {0, 0, 0, "r15"}, 0};
    regs[15] = (reg) {2, {0, "bp", "ebp", "rbp"}, 0};
    regs[16] = (reg) {2, {0, "sp", "esp", "rsp"}, 0};
}

bool regIsUsed (regIndex r) {
    return regs[r].allocatedAs != 0;
}

const reg* regGet (regIndex r) {
    return &regs[r];
}

reg* regRequest (regIndex r, int size) {
    if (size == 0) {
        debugError("regRequest", "zero sized register requested, %s", regIndexGetName(r, 8));
        size = regs[r].size == 8 ? 8 : 4;
    }

    if (regs[r].allocatedAs == 0 && regs[r].size <= size) {
        regs[r].allocatedAs = size;
        return &regs[r];

    } else
        return 0;
}

void regFree (reg* r) {
    r->allocatedAs = (int) false;
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

static const char* regGetName (const reg* r, int size) {
    if (size == 1)
        return r->names[0];

    else if (size == 2)
        return r->names[1];

    else if (size == 4)
        return r->names[2];

    else if (size == 8)
        return r->names[3];

    else {
        debugErrorUnhandledInt("regGetName", "register size", size);
        return r->names[3];
    }
}

const char* regIndexGetName (regIndex r, int size) {
    return regGetName(&regs[r], size);
}

const char* regGetStr (const reg* r) {
    return regGetName(r, r->allocatedAs);
}
