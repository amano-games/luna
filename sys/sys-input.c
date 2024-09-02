#include "sys-input.h"
#include "sys-backend.h"
#include "lib/mathfunc.h"

int
sys_inp(void)
{
	int i = backend_inp();

	return i;
}

int
sys_key(int k)
{
	return backend_key(k);
}

void
sys_keys(u8 *dest)
{
	u8 *src = backend_keys();
	memcpy(dest, src, sizeof(u8) * SYS_KEYS_LEN);
}

f32
sys_crank(void)
{
	return backend_crank();
}

int
sys_crank_docked(void)
{
	return backend_crank_docked();
}
