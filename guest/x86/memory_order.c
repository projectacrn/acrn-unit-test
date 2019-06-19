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
#include "atomic.h"

#define USE_CPU_FENCE  1
#define MAX_RUNNING_CPU 3
#define NOP()       do { asm volatile ("nop\n\t" :::"memory"); } while(0)

void ap_main();
void test1();
void test2();
void (*test_cases[MAX_RUNNING_CPU])() = { NULL, test1, test2 };

atomic_t begin_sem1;
atomic_t begin_sem2;
atomic_t end_sem;

int X, Y;
int r1, r2;
int id;

int logical_processor_arbitration() {
    int id;

    static int __booting_id = 1;    // share with cpus_booted; AP from 1, 0 is BP
    static struct spinlock cpu_id_lock = {0};
    spin_lock(&cpu_id_lock);
        id = __booting_id;
        ++__booting_id;

	printf("arbitration id: %d\n", id);
    spin_unlock(&cpu_id_lock);
    return id;
}

int main(int ac, char **av)
{
	int ret;
	int detected = 0;
	atomic_set(&begin_sem1, 0);
	atomic_set(&begin_sem2, 0);
	atomic_set(&end_sem, 0);
	setup_idt();

	id = 0;

	for (int i = 1; ; ++i) {
		X = Y = 0;
		r1 = r2 = 1;

		atomic_inc(&begin_sem1);
		atomic_inc(&begin_sem2);

		while(atomic_read(&end_sem) != 2) NOP();
		atomic_set(&end_sem, 0);

		if (r1 == 0 && r2 == 0) {
			detected++;
			printf("%d reorders detected after %d iterations\n", detected, i);
		}

		if( i % 10000 == 0) printf("BSP: times %d\n", i);
	}

	ret = report_summary();

	while(1) { NOP(); }
	return ret;
}
void ap_main() {
	int local_id = logical_processor_arbitration();

	if (local_id >= MAX_RUNNING_CPU) {
		printf("<HALT *AP* > un-used processor id %d\n", local_id);
		while(1) { NOP(); }
	} else {
		printf("<Enter *AP* > processor id %d\n", local_id);
	}

	while(1)
		test_cases[local_id]();
}
void test1() {
	while(atomic_read(&begin_sem1) != 1) NOP();
	atomic_dec(&begin_sem1);

	X = 1;
#if USE_CPU_FENCE
	asm volatile("mfence" ::: "memory");  // prevent CPU ordering
#else
	asm volatile("" ::: "memory");  // prevent compiler ordering
#endif
	r1 = Y;

	atomic_inc(&end_sem);
}

void test2() {
	while(atomic_read(&begin_sem2) != 1) NOP();
	atomic_dec(&begin_sem2);

	Y = 1;
#if USE_CPU_FENCE
	asm volatile("mfence" ::: "memory"); // prevent CPU ordering
#else
	asm volatile("" ::: "memory");  // prevent compiler ordering
#endif
	r2 = X;

	atomic_inc(&end_sem);
}
