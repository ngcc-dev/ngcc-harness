`ifndef FEILIAN_SV
`define FEILIAN_SV

module feilian
    import feilian_pkg::*;
#(
    parameter int BLOCK_BITS  = feilian_pkg::BLOCK_BITS,
    parameter int DIGEST_BITS = feilian_pkg::DIGEST_BITS,
    parameter int MMIO_ADDR_WIDTH  = feilian_pkg::ADDR_WIDTH,
    parameter int MMIO_DATA_WIDTH  = feilian_pkg::DATA_WIDTH
)(
    input  logic                  clk,
    input  logic                  rst,
    input  logic                  wr_en,
    input  logic [MMIO_ADDR_WIDTH-1:0] wr_addr,
    input  logic [MMIO_DATA_WIDTH-1:0] wr_data,
    input  logic                  rd_en,
    input  logic [MMIO_ADDR_WIDTH-1:0] rd_addr,
    output logic [MMIO_DATA_WIDTH-1:0] rd_data
);
    typedef enum logic [1:0] {IDLE, WAIT_CORE, HOLD_OUT} state_t;

    state_t                 state;
    state_t                 state_nxt;
    logic [BLOCK_BITS-1:0]  msg_reg;
    logic [BLOCK_BITS-1:0]  msg_nxt;
    logic [7:0]             valid_bytes_reg;
    logic [7:0]             valid_bytes_nxt;
    logic                   last_reg;
    logic                   last_nxt;
    logic                   start_pulse;
    logic                   core_start;
    logic                   core_done;
    logic [DIGEST_BITS-1:0] digest;
    logic                   core_busy;

    assign start_pulse = wr_en && (wr_addr == ADDR_CTRL) && wr_data[0];
    assign core_start = (state == IDLE) && start_pulse;

    core u_core (
        .clk         (clk),
        .rst         (rst),
        .start       (core_start),
        .block       (msg_reg),
        .last        (last_reg),
        .valid_bytes (valid_bytes_reg),
        .done        (core_done),
        .digest      (digest),
        .busy        (core_busy)
    );

    always_comb begin
        state_nxt       = state;
        msg_nxt         = msg_reg;
        valid_bytes_nxt = valid_bytes_reg;
        last_nxt        = last_reg;

        if (wr_en && (state == IDLE)) begin
            if ((wr_addr >= ADDR_MSG_BASE) && (wr_addr < ADDR_MSG_BASE + WORDS)) begin
                msg_nxt[(wr_addr - ADDR_MSG_BASE)*WORD_BITS +: WORD_BITS] = wr_data;
            end else if (wr_addr == ADDR_BLOCK_INFO) begin
                valid_bytes_nxt = wr_data[7:0];
                last_nxt        = wr_data[8];
            end
        end

        case (state)
            IDLE: begin
                if (core_start) begin
                    state_nxt = WAIT_CORE;
                end
            end
            WAIT_CORE: begin
                if (core_done) begin
                    state_nxt = HOLD_OUT;
                end else if (!core_busy) begin
                    state_nxt = IDLE;
                end
            end
            HOLD_OUT: begin
                state_nxt = HOLD_OUT;
            end
            default: begin
                state_nxt = IDLE;
            end
        endcase
    end

    always_ff @(posedge clk) begin
        if (rst) begin
            state <= IDLE;
            msg_reg <= '0;
            valid_bytes_reg <= '0;
            last_reg <= 1'b0;
        end else begin
            state <= state_nxt;
            msg_reg <= msg_nxt;
            valid_bytes_reg <= valid_bytes_nxt;
            last_reg <= last_nxt;
        end
    end

    always_comb begin
        rd_data = '0;
        if (rd_en) begin
            case (1'b1)
                (rd_addr == ADDR_CTRL): begin
                    rd_data = '0;
                end
                (rd_addr == ADDR_STATUS): begin
                    rd_data[0] = (state == IDLE);
                    rd_data[1] = (state == WAIT_CORE);
                    rd_data[2] = (state == HOLD_OUT);
                    rd_data[3] = core_busy;
                end
                (rd_addr == ADDR_BLOCK_INFO): begin
                    rd_data[7:0] = valid_bytes_reg;
                    rd_data[8]   = last_reg;
                end
                ((rd_addr >= ADDR_MSG_BASE) && (rd_addr < ADDR_MSG_BASE + WORDS)): begin
                    rd_data = msg_reg[(rd_addr - ADDR_MSG_BASE)*WORD_BITS +: WORD_BITS];
                end
                ((rd_addr >= ADDR_DIGEST_BASE) && (rd_addr < ADDR_DIGEST_BASE + WORDS)): begin
                    rd_data = digest[(rd_addr - ADDR_DIGEST_BASE)*WORD_BITS +: WORD_BITS];
                end
                default: begin
                    rd_data = '0;
                end
            endcase
        end
    end
endmodule

`endif
