#!/usr/bin/env python3
import argparse
import json
from pathlib import Path


Q_SHIFT = 14
Q_SCALE = 1 << Q_SHIFT
FEATURE_SCALE = 256
NORM_SHIFT = 28
INV_STD_SCALE = 1 << (NORM_SHIFT - 8)


def q14(value):
    return int(round(float(value) * Q_SCALE))


def mean_q(value):
    return int(round(float(value) * FEATURE_SCALE))


def inv_std_q(value):
    std = float(value)
    if std == 0.0:
        std = 1.0
    return int(round((1.0 / std) * INV_STD_SCALE))


def format_1d(values, indent="  "):
    return ",\n".join(f"{indent}{int(value)}" for value in values)


def format_2d(values, indent="  "):
    rows = []
    for row in values:
        rows.append("{\n" + format_1d(row, indent + "  ") + "\n" + indent + "}")
    return ",\n".join(indent + row for row in rows)


def quantize_2d(values):
    return [[q14(value) for value in row] for row in values]


def write_header(path, input_dim, hidden1, hidden2, output_dim):
    path.write_text(
        f"""#ifndef __MLTHRE_MODEL_H_INCLUDED__
#define __MLTHRE_MODEL_H_INCLUDED__

#include \"types.h\"

#define MLTHRE_MODEL_INPUT_DIM {input_dim}
#define MLTHRE_MODEL_HIDDEN1_DIM {hidden1}
#define MLTHRE_MODEL_HIDDEN2_DIM {hidden2}
#define MLTHRE_MODEL_OUTPUT_DIM {output_dim}
#define MLTHRE_MODEL_Q_SHIFT {Q_SHIFT}
#define MLTHRE_MODEL_Q_SCALE {Q_SCALE}
#define MLTHRE_MODEL_FEATURE_SCALE {FEATURE_SCALE}

int32_t mlthre_model_predict_delta(IN const int32_t features[MLTHRE_MODEL_INPUT_DIM]);

#endif
"""
    )


def write_source(path, payload):
    weights = payload["weights"]
    labels = [int(value) for value in payload["labels"]]
    means = [mean_q(value) for value in payload["feature_means"]]
    inv_stds = [inv_std_q(value) for value in payload["feature_stds"]]
    w1 = quantize_2d(weights["w1"])
    b1 = [q14(value) for value in weights["b1"]]
    w2 = quantize_2d(weights["w2"])
    b2 = [q14(value) for value in weights["b2"]]
    w3 = quantize_2d(weights["w3"])
    b3 = [q14(value) for value in weights["b3"]]

    path.write_text(
        f"""#include \"mlthre_model.h\"
#include \"utilities.h\"

#include <stdint.h>

#define MLTHRE_MODEL_NORM_SHIFT {NORM_SHIFT}

static const int32_t feature_means_q[MLTHRE_MODEL_INPUT_DIM] = {{
{format_1d(means)}
}};

static const int32_t inv_feature_stds_q[MLTHRE_MODEL_INPUT_DIM] = {{
{format_1d(inv_stds)}
}};

static const int32_t output_labels[MLTHRE_MODEL_OUTPUT_DIM] = {{
{format_1d(labels)}
}};

static const int32_t w1[MLTHRE_MODEL_HIDDEN1_DIM][MLTHRE_MODEL_INPUT_DIM] = {{
{format_2d(w1)}
}};

static const int32_t b1[MLTHRE_MODEL_HIDDEN1_DIM] = {{
{format_1d(b1)}
}};

static const int32_t w2[MLTHRE_MODEL_HIDDEN2_DIM][MLTHRE_MODEL_HIDDEN1_DIM] = {{
{format_2d(w2)}
}};

static const int32_t b2[MLTHRE_MODEL_HIDDEN2_DIM] = {{
{format_1d(b2)}
}};

static const int32_t w3[MLTHRE_MODEL_OUTPUT_DIM][MLTHRE_MODEL_HIDDEN2_DIM] = {{
{format_2d(w3)}
}};

static const int32_t b3[MLTHRE_MODEL_OUTPUT_DIM] = {{
{format_1d(b3)}
}};

static int32_t q_down(IN const int64_t x, IN const uint32_t shift)
{{
  return (int32_t)(x >> shift);
}}

static int32_t ct_relu_i32(IN const int32_t x)
{{
  const uint32_t mask = ((uint32_t)x >> 31U) - 1U;
  return (int32_t)((uint32_t)x & mask);
}}

static uint32_t ct_gt_i32_mask(IN const int32_t x, IN const int32_t y)
{{
  const uint32_t ux = ((uint32_t)x) ^ 0x80000000U;
  const uint32_t uy = ((uint32_t)y) ^ 0x80000000U;
  return ~secure_l32_mask(uy, ux);
}}

static int32_t ct_select_i32(IN const uint32_t mask,
                             IN const int32_t old_value,
                             IN const int32_t new_value)
{{
  return (int32_t)((u32_barrier(~mask) & (uint32_t)old_value) |
                   (u32_barrier(mask) & (uint32_t)new_value));
}}

int32_t mlthre_model_predict_delta(IN const int32_t features[MLTHRE_MODEL_INPUT_DIM])
{{
  int32_t x[MLTHRE_MODEL_INPUT_DIM];
  int32_t h1[MLTHRE_MODEL_HIDDEN1_DIM];
  int32_t h2[MLTHRE_MODEL_HIDDEN2_DIM];
  int32_t best_logit = INT32_MIN;
  int32_t best_label = 0;

  for(uint32_t i = 0; i < MLTHRE_MODEL_INPUT_DIM; i++) {{
    const int64_t centered = (int64_t)features[i] - feature_means_q[i];
    x[i] = q_down(centered * inv_feature_stds_q[i],
                  MLTHRE_MODEL_NORM_SHIFT - MLTHRE_MODEL_Q_SHIFT);
  }}

  for(uint32_t i = 0; i < MLTHRE_MODEL_HIDDEN1_DIM; i++) {{
    int64_t acc = b1[i];
    for(uint32_t j = 0; j < MLTHRE_MODEL_INPUT_DIM; j++) {{
      acc += q_down((int64_t)w1[i][j] * x[j], MLTHRE_MODEL_Q_SHIFT);
    }}
    h1[i] = ct_relu_i32((int32_t)acc);
  }}

  for(uint32_t i = 0; i < MLTHRE_MODEL_HIDDEN2_DIM; i++) {{
    int64_t acc = b2[i];
    for(uint32_t j = 0; j < MLTHRE_MODEL_HIDDEN1_DIM; j++) {{
      acc += q_down((int64_t)w2[i][j] * h1[j], MLTHRE_MODEL_Q_SHIFT);
    }}
    h2[i] = ct_relu_i32((int32_t)acc);
  }}

  for(uint32_t i = 0; i < MLTHRE_MODEL_OUTPUT_DIM; i++) {{
    int64_t acc = b3[i];
    for(uint32_t j = 0; j < MLTHRE_MODEL_HIDDEN2_DIM; j++) {{
      acc += q_down((int64_t)w3[i][j] * h2[j], MLTHRE_MODEL_Q_SHIFT);
    }}

    const int32_t logit = (int32_t)acc;
    const uint32_t gt_mask = ct_gt_i32_mask(logit, best_logit);
    best_logit = ct_select_i32(gt_mask, best_logit, logit);
    best_label = ct_select_i32(gt_mask, best_label, output_labels[i]);
  }}

  return best_label;
}}
"""
    )


def main():
    parser = argparse.ArgumentParser(
        description="Export MLThre MLP JSON to fixed-point constant-trip C."
    )
    parser.add_argument("--model", type=Path, required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    args = parser.parse_args()

    with args.model.open() as fp:
        payload = json.load(fp)

    weights = payload["weights"]
    input_dim = len(payload["feature_columns"])
    hidden1 = len(weights["b1"])
    hidden2 = len(weights["b2"])
    output_dim = len(payload["labels"])

    args.out_dir.mkdir(parents=True, exist_ok=True)
    write_header(args.out_dir / "mlthre_model.h", input_dim, hidden1, hidden2, output_dim)
    write_source(args.out_dir / "mlthre_model.c", payload)


if __name__ == "__main__":
    main()
