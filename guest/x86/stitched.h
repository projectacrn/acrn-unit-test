#ifndef __MAIN_H
#define __MAIN_H

#include "libcflat.h"

struct unit_test {
	const char *name;
	int (*fn)(int ac, char **av);
};

extern struct unit_test unit_tests[];

#endif
