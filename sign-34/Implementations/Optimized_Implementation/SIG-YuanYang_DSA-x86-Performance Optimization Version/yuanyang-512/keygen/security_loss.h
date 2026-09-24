#ifndef YUANYANG_512_SECURITY_LOSS_H
#define YUANYANG_512_SECURITY_LOSS_H

#include <stdint.h>

#include "yuanyang_params.h"

int yuanyang_security_loss_accept(
	const int8_t f[YUANYANG_D],
	const int8_t g[YUANYANG_D]);

#endif
