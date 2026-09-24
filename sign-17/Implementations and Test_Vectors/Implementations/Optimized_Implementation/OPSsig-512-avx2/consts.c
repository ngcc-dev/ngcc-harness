#include <stdint.h>
#include "consts.h"

#define QINV ((int32_t)0xfd001001u)

const qdata_t qdata = {{
  Q, Q, Q, Q, Q, Q, Q, Q,
  QINV, QINV, QINV, QINV, QINV, QINV, QINV, QINV
}};
