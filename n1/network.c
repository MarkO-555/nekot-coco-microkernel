#include "n1/private.h"

byte logbuf[30];

word strlen(const char* s) {
    const char* p = s;
    while (*p) ++p;
    return p-s;
}

void WizSend(byte* addr, word size) {
    byte cc_value = N1IrqSaveAndDisable();

    tx_ptr_t t = WizReserveToSend(size);
    t = WizBytesToSend(t, addr, size);
    WizFinalizeSend(size);

    N1IrqRestore(cc_value);
}

// HACK
void N1NetworkLog(const char* s) {
    word n = strlen(s);
    logbuf[0] = CMD_LOG;
    Poke2(logbuf+1, n);
    Poke2(logbuf+3, 0);
    MemCopy(logbuf+5, (byte*)s, n);

    WizSend(logbuf, n+5);
}

MORE_DATA byte recv_head[5];
MORE_DATA byte recv_buf[64];

bool need_recv_payload;
bool need_to_start_task;
word task_to_start;

void ExecuteReceivedCommand() {
    byte* h = recv_head;
    byte* b = recv_buf;

    word n = Peek2(h+1);
    word p = Peek2(h+3);
// Console_Printf(" %d(%x,%x)", h[0], n, p);

    if (h[0] == CMD_DATA) {
        // If we ever send CMD_ECHO, expect CMD_DATA.
    } else if (h[0] == NEKOT_POKE) { // 66
            errnum e2 = WizRecvChunkTry((byte*)p, n);
            if (e2==NOTYET) return;
            if (e2) Fatal("ERR E",e2);
    } else if (h[0] == NEKOT_CALL) { // 67
            func fn = (func)p;
            fn();
    } else if (h[0] == NEKOT_LAUNCH) { // 68
            task_to_start = p;
            need_to_start_task = true;
    } else {
        Fatal("XRC", h[0]);
    }

    need_recv_payload = false;
}

void CheckReceived() {
    byte cc_value = N1IrqSaveAndDisable();

    if (!need_recv_payload) {
        byte err = WizRecvChunkTry(recv_head, 5);
        if (err==NOTYET) goto RESTORE;
        if (err) Fatal("RECV", err);
        need_recv_payload = true;
    }

    ExecuteReceivedCommand();

    if (need_to_start_task) {
// Console_Printf("NEED(%x).", task_to_start);
        need_to_start_task = false;
        StartTask(task_to_start);
        // Note StartTask never returns.
        // It will launch the task and allow IRQs.
    }

RESTORE:
    N1IrqRestore(cc_value);
}

void HelloMCP() {
    struct quint q = {CMD_HELLO_NEKOT, 0, 0};
    WizSend((byte*)&q, 5);
}

void Network_Init() {
    Wiznet_Init();
}
