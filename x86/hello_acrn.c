#include "vm.h"

int main(void)
{
	setup_vm();
	
	report(true, "***Hello ACRN***");
	return report_summary();
}
