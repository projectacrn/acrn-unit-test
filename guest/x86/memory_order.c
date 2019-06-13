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

void ap_main();

int main(int ac, char **av)
{
	int ret;

	ap_main();

	// report("invalid clwb", ud);
	// report("pcommit (%s)", ud == expected, expected ? "ABSENT" : "present");

	ret = report_summary();

	// while(1);
	return ret;
}
