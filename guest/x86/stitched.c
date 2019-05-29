#include "stitched.h"

int main(int ac, char **av)
{
	struct unit_test *ut;

	if (ac > 0) {
		ut = unit_tests;
		while (ut->fn != NULL) {
			if (strcmp(av[0], ut->name) == 0) {
				printf("Run test %s\n", av[0]);
				return ut->fn(ac, av);
			} else {
				ut++;
			}
		}

		printf("No test found for %s\n", av[0]);
	}

	/* Print the list of available tests */
	printf("Available tests:\n");

	ut = unit_tests;
	while (ut->fn != NULL) {
		printf("\t%s\n", ut->name);
		ut++;
	}

	printf("End of available tests\n");

	return 0;
}
