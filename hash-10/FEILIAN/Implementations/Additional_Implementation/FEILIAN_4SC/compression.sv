`ifndef COMPRESSION_SV
`define COMPRESSION_SV

module compression
  import feilian_pkg::*;
#(
    parameter int ROUNDS = feilian_pkg::ROUNDS,
    parameter int CHAIN_BITS = feilian_pkg::CHAIN_BITS,
    parameter int BLOCK_BITS = feilian_pkg::BLOCK_BITS,
    parameter int DOMAIN_BITS = feilian_pkg::DOMAIN_BITS
) (
    input  logic                   clk,
    input  logic                   rst,
    input  logic                   start,
    input  logic [ CHAIN_BITS-1:0] h_in,
    input  logic [ BLOCK_BITS-1:0] m,
    input  logic [DOMAIN_BITS-1:0] d,
    output logic                   done,
    output logic [ CHAIN_BITS-1:0] h_out
);
  typedef enum logic [1:0] {
    IDLE,
    RUN,
    OUTPUT
  } state_t;
  localparam int ROUND_IDX_W = $clog2(ROUNDS);
  localparam int ROUND_OFFSET_W = $clog2(INJECT_INTERVAL);
  localparam logic [ROUND_IDX_W-1:0] LAST_ROUND = ROUNDS - 1;
  localparam logic [ROUND_OFFSET_W-1:0] LAST_ROUND_OFFSET = INJECT_INTERVAL - 1;
  localparam logic [ROUND_GROUP_BITS-1:0] LAST_ROUND_GROUP = ROUND_CONSTANT_WORDS - 1;

  state_t                   state;
  state_t                   state_nxt;
  logic   [ CHAIN_BITS-1:0] v_reg;
  logic   [ CHAIN_BITS-1:0] v_nxt;
  logic   [ROUND_IDX_W-1:0] round_reg;
  logic   [ROUND_IDX_W-1:0] round_nxt;
  logic   [ROUND_OFFSET_W-1:0] round_offset_reg;
  logic   [ROUND_OFFSET_W-1:0] round_offset_nxt;
  logic   [ROUND_GROUP_BITS-1:0] round_group_reg;
  logic   [ROUND_GROUP_BITS-1:0] round_group_nxt;
  logic                     round_phase_reg;
  logic                     round_phase_nxt;
  logic   [ CHAIN_BITS-1:0] round_in;
  logic   [ CHAIN_BITS-1:0] round_out;
  logic                     inject_msg;

  function automatic logic [CHAIN_BITS-1:0] xor_message(
      input logic [CHAIN_BITS-1:0] words,
      input logic [BLOCK_BITS-1:0] msg,
      input int unsigned base
  );
    xor_message = words;
    for (int col = 0; col < 4; col++) begin
      xor_message[col*WORD_BITS +: WORD_BITS] =
          words[col*WORD_BITS +: WORD_BITS] ^ msg[(base + col)*WORD_BITS +: WORD_BITS];
      xor_message[(col + 8)*WORD_BITS +: WORD_BITS] =
          words[(col + 8)*WORD_BITS +: WORD_BITS] ^ msg[(base + 4 + col)*WORD_BITS +: WORD_BITS];
    end
  endfunction

  round u_round (
      .v_in          (round_in),
      .d             (d),
      .round_group   (round_group_reg),
      .add_constants (round_phase_reg),
      .v_out         (round_out)
  );

  assign done    = (state == OUTPUT);
  assign h_out        = v_reg;
  assign inject_msg   = (round_offset_reg == '0);
  assign round_in = inject_msg ?
      xor_message(v_reg, m, round_phase_reg ? 8 : 0) : v_reg;

  always_comb begin
    state_nxt  = state;
    v_nxt      = v_reg;
    round_nxt  = round_reg;
    round_offset_nxt = round_offset_reg;
    round_group_nxt  = round_group_reg;
    round_phase_nxt  = round_phase_reg;

    case (state)
      IDLE: begin
        if (start) begin
          v_nxt      = h_in;
          round_nxt  = '0;
          round_offset_nxt = '0;
          round_group_nxt  = '0;
          round_phase_nxt  = 1'b0;
          state_nxt  = RUN;
        end
      end
      RUN: begin
        if (!round_phase_reg) begin
          v_nxt = round_out;
          round_phase_nxt = 1'b1;
        end else begin
          round_phase_nxt = 1'b0;
          if (round_reg == LAST_ROUND) begin
            v_nxt     = round_out ^ h_in;
            state_nxt = OUTPUT;
          end else begin
            v_nxt     = round_out;
            round_nxt = round_reg + 1'b1;
            if (round_offset_reg == LAST_ROUND_OFFSET) begin
              round_offset_nxt = '0;
              round_group_nxt  = (round_group_reg == LAST_ROUND_GROUP) ? '0 : round_group_reg + 1'b1;
            end else begin
              round_offset_nxt = round_offset_reg + 1'b1;
            end
          end
        end
      end
      OUTPUT: begin
        state_nxt = IDLE;
      end
      default: begin
        state_nxt = IDLE;
      end
    endcase
  end

  always_ff @(posedge clk) begin
    state      <= rst ? IDLE : state_nxt;
    v_reg      <= v_nxt;
    round_reg  <= rst ? '0 : round_nxt;
    round_offset_reg <= rst ? '0 : round_offset_nxt;
    round_group_reg  <= rst ? '0 : round_group_nxt;
    round_phase_reg  <= rst ? 1'b0 : round_phase_nxt;
  end
endmodule

`endif
