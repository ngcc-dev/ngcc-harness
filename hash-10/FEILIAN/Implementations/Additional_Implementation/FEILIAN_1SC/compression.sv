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
    WAIT_ROUND,
    OUTPUT
  } state_t;

  localparam int ROUND_IDX_W = $clog2(ROUNDS);
  localparam int ROUND_OFFSET_W = $clog2(INJECT_INTERVAL);
  localparam logic [ROUND_IDX_W-1:0] LAST_ROUND = ROUNDS - 1;
  localparam logic [ROUND_OFFSET_W-1:0] LAST_ROUND_OFFSET = INJECT_INTERVAL - 1;
  localparam logic [ROUND_GROUP_BITS-1:0] LAST_ROUND_GROUP = ROUND_CONSTANT_WORDS - 1;

  state_t state;
  state_t state_nxt;
  logic [ROUND_IDX_W-1:0] round_reg;
  logic [ROUND_IDX_W-1:0] round_nxt;
  logic [ROUND_OFFSET_W-1:0] round_offset_reg;
  logic [ROUND_OFFSET_W-1:0] round_offset_nxt;
  logic [ROUND_GROUP_BITS-1:0] round_group_reg;
  logic [ROUND_GROUP_BITS-1:0] round_group_nxt;
  logic round_start;
  logic round_done;
  logic [CHAIN_BITS-1:0] round_in;
  logic [CHAIN_BITS-1:0] round_out;
  logic inject_msg;
  logic [ROUND_OFFSET_W-1:0] next_round_offset;
  logic [ROUND_GROUP_BITS-1:0] next_round_group;
  logic [ROUND_GROUP_BITS-1:0] active_round_group;

  round u_round (
      .clk         (clk),
      .rst         (rst),
      .start       (round_start),
      .v_in        (round_in),
      .m           (m),
      .d           (d),
      .round_group (active_round_group),
      .inject_en   (inject_msg),
      .done        (round_done),
      .v_out       (round_out)
  );

  assign done        = (state == OUTPUT);
  assign h_out       = round_out ^ h_in;
  assign round_start = ((state == IDLE) && start) ||
                       ((state == WAIT_ROUND) && round_done && (round_reg != LAST_ROUND));
  assign round_in    = ((state == IDLE) && start) ? h_in : round_out;
  assign next_round_offset = (round_offset_reg == LAST_ROUND_OFFSET) ?
      '0 : round_offset_reg + 1'b1;
  assign next_round_group = (round_offset_reg == LAST_ROUND_OFFSET) ?
      ((round_group_reg == LAST_ROUND_GROUP) ? '0 : round_group_reg + 1'b1) :
      round_group_reg;
  assign active_round_group = ((state == IDLE) && start) ? '0 :
      (((state == WAIT_ROUND) && round_done && (round_reg != LAST_ROUND)) ?
       next_round_group : round_group_reg);
  assign inject_msg = ((state == IDLE) && start) ? 1'b1 :
      (((state == WAIT_ROUND) && round_done && (round_reg != LAST_ROUND)) ?
       (next_round_offset == '0) : (round_offset_reg == '0));

  always_comb begin
    state_nxt        = state;
    round_nxt        = round_reg;
    round_offset_nxt = round_offset_reg;
    round_group_nxt  = round_group_reg;

    case (state)
      IDLE: begin
        if (start) begin
          round_nxt        = '0;
          round_offset_nxt = '0;
          round_group_nxt  = '0;
          state_nxt        = WAIT_ROUND;
        end
      end

      WAIT_ROUND: begin
        if (round_done) begin
          if (round_reg == LAST_ROUND) begin
            state_nxt = OUTPUT;
          end else begin
            round_nxt = round_reg + 1'b1;
            if (round_offset_reg == LAST_ROUND_OFFSET) begin
              round_offset_nxt = next_round_offset;
              round_group_nxt  = next_round_group;
            end else begin
              round_offset_nxt = next_round_offset;
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
    if (rst) begin
      state <= IDLE;
      round_reg <= '0;
      round_offset_reg <= '0;
      round_group_reg <= '0;
    end else begin
      state <= state_nxt;
      round_reg <= round_nxt;
      round_offset_reg <= round_offset_nxt;
      round_group_reg <= round_group_nxt;
    end
  end
endmodule

`endif
