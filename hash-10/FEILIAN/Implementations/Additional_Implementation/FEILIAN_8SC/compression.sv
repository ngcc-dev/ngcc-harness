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
  logic   [ CHAIN_BITS-1:0] h_reg;
  logic   [ CHAIN_BITS-1:0] h_nxt;
  logic   [ BLOCK_BITS-1:0] m_reg;
  logic   [ BLOCK_BITS-1:0] m_nxt;
  logic   [DOMAIN_BITS-1:0] d_reg;
  logic   [DOMAIN_BITS-1:0] d_nxt;
  logic   [ CHAIN_BITS-1:0] v_reg;
  logic   [ CHAIN_BITS-1:0] v_nxt;
  logic   [ROUND_IDX_W-1:0] round_reg;
  logic   [ROUND_IDX_W-1:0] round_nxt;
  logic   [ROUND_OFFSET_W-1:0] round_offset_reg;
  logic   [ROUND_OFFSET_W-1:0] round_offset_nxt;
  logic   [ROUND_GROUP_BITS-1:0] round_group_reg;
  logic   [ROUND_GROUP_BITS-1:0] round_group_nxt;
  logic   [ CHAIN_BITS-1:0] round_v;
  logic                     inject_msg;
  logic   [ROUND_GROUP_BITS-1:0] round_group;

  round u_round (
      .v_in     (v_reg),
      .m        (m_reg),
      .d  (d_reg),
      .round_group(round_group),
      .inject_en(inject_msg),
      .v_out    (round_v)
  );

  assign done    = (state == OUTPUT);
  assign h_out        = v_reg;
  assign inject_msg   = (round_offset_reg == '0);
  assign round_group  = round_group_reg;

  always_comb begin
    state_nxt  = state;
    h_nxt     = h_reg;
    m_nxt      = m_reg;
    d_nxt = d_reg;
    v_nxt      = v_reg;
    round_nxt  = round_reg;
    round_offset_nxt = round_offset_reg;
    round_group_nxt  = round_group_reg;

    case (state)
      IDLE: begin
        if (start) begin
          h_nxt     = h_in;
          m_nxt      = m;
          d_nxt = d;
          v_nxt      = h_in;
          round_nxt  = '0;
          round_offset_nxt = '0;
          round_group_nxt  = '0;
          state_nxt  = RUN;
        end
      end
      RUN: begin
        if (round_reg == LAST_ROUND) begin
          v_nxt     = round_v ^ h_reg;
          state_nxt = OUTPUT;
        end else begin
          v_nxt     = round_v;
          round_nxt = round_reg + 1'b1;
          if (round_offset_reg == LAST_ROUND_OFFSET) begin
            round_offset_nxt = '0;
            round_group_nxt  = (round_group_reg == LAST_ROUND_GROUP) ? '0 : round_group_reg + 1'b1;
          end else begin
            round_offset_nxt = round_offset_reg + 1'b1;
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
    h_reg     <= h_nxt;
    m_reg      <= m_nxt;
    d_reg <= d_nxt;
    v_reg      <= v_nxt;
    round_reg  <= rst ? '0 : round_nxt;
    round_offset_reg <= rst ? '0 : round_offset_nxt;
    round_group_reg  <= rst ? '0 : round_group_nxt;
  end
endmodule

`endif
