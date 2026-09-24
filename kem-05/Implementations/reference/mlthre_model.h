#ifndef __MLTHRE_MODEL_H_INCLUDED__
#define __MLTHRE_MODEL_H_INCLUDED__

#include "types.h"

#define MLTHRE_MODEL_INPUT_DIM 11
#define MLTHRE_MODEL_HIDDEN1_DIM 32
#define MLTHRE_MODEL_HIDDEN2_DIM 16
#define MLTHRE_MODEL_OUTPUT_DIM 13

int32_t mlthre_model_predict_delta(IN const float features[MLTHRE_MODEL_INPUT_DIM]);

#endif
