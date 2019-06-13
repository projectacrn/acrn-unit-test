/*
 * Test for x86 cache and memory instructions
 *
 * Copyright (c) 2015 Red Hat Inc
 *
 * Authors:
 *  Eduardo Habkost <ehabkost@redhat.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include "libcflat.h"
#include "desc.h"
#include "processor.h"

#include "asm/spinlock.h"
#define CFG_HARD_CPU_NR 8
#define CFG_MASTER_ID   0

#define CFG_MEMORY_ORDER_STAT_CPU_WAIT 2
long func1(int id);
long func2(int id);




#define NOP()       do { asm volatile ("nop\n\t" :::"memory"); } while(0)
#define MFENCE()    do { asm volatile ("mfence\n\t" :::"memory"); } while(0)
#define HALT()  do { asm volatile ("hlt\n\t" "pause\n\t" :::"memory"); } while(1)

static struct spinlock console_slck = {0};
static inline void console_lock(void) { spin_lock(&console_slck); }
static inline void console_unlock(void) { spin_unlock(&console_slck); }
static inline void generic_lock(void) { spin_lock(&console_slck); }
static inline void generic_unlock(void) { spin_unlock(&console_slck); }

/*
static void delay(void) {
    volatile int tm = 0x1FFFF;
    while(--tm) {
        NOP();
    }
}
*/

int logical_processor_arbitration(int _id) {
    int id;

    static int __booting_id = 0;    // share with cpus_booted;
    static struct spinlock cpu_id_lock = {0};
    spin_lock(&cpu_id_lock);
        id = __booting_id;
        ++__booting_id;
    spin_unlock(&cpu_id_lock);
    return id;
}

typedef struct __sema_s __sema_t;
struct __sema_s {
    volatile int testing;
    volatile unsigned status;
    struct spinlock slock;
    volatile int val;
};


static __sema_t threads = { 0, 0, {0}, CFG_MEMORY_ORDER_STAT_CPU_WAIT };

static void sync_down(__sema_t *sema, volatile unsigned *st, volatile int *in, int id, int deadline) {
    const unsigned mask = (0x1 << deadline) - 1;

    int val;
    while(1) {
        spin_lock(&sema->slock);
        val = sema->val;

        MFENCE();
        if ((*st & mask) == mask) {

            if ((*in == 1) && (val == deadline)) {
                *in = 0;
                *st = 0;
                MFENCE();
                spin_unlock(&sema->slock);

                continue;
            }
        }

        if ( !(*st & (0x1 << id))) {
            // if (val > 0) {
            // if (*in == 0) {
                --sema->val;
                *st |= 0x1 << id;
                MFENCE();
                spin_unlock(&sema->slock);
                break;
            // }
            // }
        }

        spin_unlock(&sema->slock);
        NOP();
    }

    while(1) {
        NOP();
        NOP();
        NOP();
        spin_lock(&sema->slock);
            val = sema->val;
            MFENCE();
            if ((*st & mask) == mask) {
                if ((*in == 0) && (val == 0)) {
                    *in = 1;
                    MFENCE();
                }
            }

            if (*in == 1) {
                spin_unlock(&sema->slock);
                break;
            }
        spin_unlock(&sema->slock);
        NOP();
        NOP();
        NOP();
    }
}
static void sync_up(__sema_t *sema, volatile unsigned *st, int id, int idle) {
    spin_lock(&sema->slock);
    ++sema->val;
    spin_unlock(&sema->slock);
}
static inline void test_exec_enter(int id) {
    if ( (id >= 0) && (id < CFG_MEMORY_ORDER_STAT_CPU_WAIT))
        sync_down(&threads, &threads.status, &threads.testing, id, CFG_MEMORY_ORDER_STAT_CPU_WAIT);
}
static inline void test_exec_exit(int id) {
    if ( (id >= 0) && (id < CFG_MEMORY_ORDER_STAT_CPU_WAIT))
        sync_up(&threads, &threads.status, id, CFG_MEMORY_ORDER_STAT_CPU_WAIT);
}

#if 1

volatile long _x, _y;
volatile long _rx, _ry;


static long (*test_funcs[CFG_MEMORY_ORDER_STAT_CPU_WAIT])(int id) = {func1, func2 };
void test(int id, int times) {

    static volatile int __val = 0;
    register long __r;

    test_exec_enter(id);
    __r = test_funcs[id](id);
    test_exec_exit(id);


    generic_lock();

    if (id == 0) { _ry = __r; }
    if (id == 1) { _rx = __r; }

    MFENCE();

    if ( (id == 0) && (_ry != __r)) {
        printf("<fault core#%d> failure for un-consisten _ry val %ld, %ld\n", id, __r, _ry);
    }

    if ( (id == 1) && (_rx != __r)) {
        printf("<fault core#%d> failure for un-consisten _rx val %ld, %ld\n", id, __r, _rx);
    }

    // printf("........ <debug @core#%d> __r = %ld (_x %ld, _y %ld, _rx %ld, _ry %ld) ........\n", id, __r, _x, _y, _rx, _ry);

    {
        static volatile unsigned __cpu_cnt = 0;
        ++__cpu_cnt;
        MFENCE();

        if ( (__cpu_cnt & (CFG_MEMORY_ORDER_STAT_CPU_WAIT - 0x1)) == 0) {


        if ( (_rx == 0) && (_rx == _ry) ) {
            printf("<re-ordered @core#%d>: %08dth TEST Re-Ordered 8.2.3.4\n", id, times);
            if (__val == 0) {
                __val = 1;
                MFENCE();
            }
        }

        if (__val != 0) {

            printf("[FAILURE @core#%d] times = %d, (_x %ld, _y %ld, _rx %ld, _ry %ld)\n",
                id, times, _x, _y, _rx, _ry);

        }

        _x = _y = 0;
        _rx = _ry = 0;
        MFENCE();

        __val = 0;
        MFENCE();
        }
    }

    if ( (times % 10000000) == 0) {
        printf("[In Testing Core#%d] Loop %d\n", id, times);
    }
    generic_unlock();

    // report("Wether Re-Order val = %d", val == CFG_HARD_CPU_NR, val);
}

#else

volatile long val = 0;
long func1(int id) {
    asm volatile("incq %0\n\t": "+m"(val):: "memory");
    return 0;
}
long func2(int id) {
    // ++val;
    asm volatile("decq %0\n\t": "+m"(val):: "memory");
    return 0;
}

static long (*test_funcs[CFG_MEMORY_ORDER_STAT_CPU_WAIT])(int id) = {func1, func2 };
void test(int id, int times) {
    static volatile int __val = 0;
    register long __r;


    test_exec_enter(id);
    __r = test_funcs[id](id);
    test_exec_exit(id);


    generic_lock();

    if (__r != 0) {
        printf("    < fault %s @core#%d> times = %d, val = %d, __r = %ld\n",
            __val == 0 ? "PASS": "FAILURE", id, times, __val, __r);
    }

    if (val != 0) {
        __val = val;
        MFENCE();


        printf("[FAILURE @core#%d] times = %d, val = %d\n", id, times, __val);

        val = 0; // INIT
        __val = 0;
        MFENCE();
    }

    if ( (times % 1000) == 0)
        printf("[In Testing Core#%d] Loop %d\n", id, times);
    generic_unlock();

    // report("Wether Re-Order val = %d", val == CFG_HARD_CPU_NR, val);
}

#endif

void ap_main() {
    int id;
    int times[CFG_MEMORY_ORDER_STAT_CPU_WAIT] = {0};

    setup_idt();

    id = logical_processor_arbitration(CFG_MASTER_ID);

    generic_lock();
    if (id >= CFG_MEMORY_ORDER_STAT_CPU_WAIT) {
        printf("<HALT core %d> un-used processor id %d\n", id, id);
        generic_unlock();

        while(1) { NOP(); }
    } else {
        printf("<Enter core %d> Testing processor id %d\n", id, id);
    }
    generic_unlock();

    generic_lock();
    printf("Core id = %d\n", id);
    generic_unlock();

    while(1) {
        // if (times[id] >= 10000)
            // break;

        test(id, times[id]);
        ++times[id];
    }

    generic_lock();
    printf("<Exit core %d> Testing processor id %d all times %d\n", id, id, times[id]);
    generic_unlock();

    while(1) { NOP(); }
}


long func1(int id) {
    register long tmp_result;

    asm volatile (
            "xor %0, %0\n\t                 "
            "nop\n\t                        "
            "movq $1, %1\n\t                "
            "#; mfence\n\t                  "
            "movq %2, %0\n\t                "
            "nop\n\t                        "
            : "=&r"(tmp_result), "+m" (_x)
            : "m"(_y)
            : "memory");
    return tmp_result;
}
long func2(int id) {
    register long tmp_result;

    asm volatile (
            "xor  %0, %0\n\t                "
            "nop\n\t                        "
            "movq $1, %1\n\t                "
            "movq %2, %0\n\t                "
            "nop\n\t                        "
            : "=&r"(tmp_result), "+m" (_y)
            : "m"(_x)
            : "memory");
    return tmp_result;
}
