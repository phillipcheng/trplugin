/* dummy
 */

#include "dc.h"

int ta_serv_init(void)
{
    fprintf(stderr, "Never call me, I am dummy");
	return 0;
}

void ta_serv_fini(void)
{
    fprintf(stderr, "Never call me, I am dummy");
	return;
}