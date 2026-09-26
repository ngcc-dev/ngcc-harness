`ifndef CORE_SV
`define CORE_SV

module core
    import feilian_pkg::*;
#(
    parameter int CHAIN_BITS  = feilian_pkg::CHAIN_BITS,
    parameter int BLOCK_BITS  = feilian_pkg::BLOCK_BITS,
    parameter int DIGEST_BITS = feilian_pkg::DIGEST_BITS
)(
    input  logic                   clk,
    input  logic                   rst,
    input  logic                   start,
    input  logic [BLOCK_BITS-1:0]  block,
    input  logic                   last,
    input  logic [7:0]             valid_bytes,
    output logic                   done,
    output logic [DIGEST_BITS-1:0] digest,
    output logic                   busy
);
    typedef enum logic [1:0] {IDLE, WAIT_CF, DONE} state_t;

    localparam int BLOCK_BITS_COUNT_WIDTH = $clog2(BLOCK_BITS + 1);

    state_t                            state;
    state_t                            state_nxt;
    logic [CHAIN_BITS-1:0]             h_reg;
    logic [CHAIN_BITS-1:0]             h_nxt;
    logic [127:0]                      hashed_bits_reg;
    logic [127:0]                      hashed_bits_nxt;
    logic                              active_last_reg;
    logic                              active_last_nxt;
    logic [7:0]                        active_valid_bytes_reg;
    logic [7:0]                        active_valid_bytes_nxt;
    logic [BLOCK_BITS-1:0]             cf_block;
    logic [255:0]                      cf_domain;
    logic                              cf_start;
    logic                              cf_done;
    logic [CHAIN_BITS-1:0]             cf_h_out;
    logic [BLOCK_BITS_COUNT_WIDTH-1:0] bits_added;

    function automatic logic [CHAIN_BITS-1:0] make_h_init();
        logic [CHAIN_BITS-1:0] h;
        begin
            h = '0;
            for (int i = 0; i < WORDS; i++) begin
                h[i*WORD_BITS +: WORD_BITS] = IV[i];
            end
            return h;
        end
    endfunction

    function automatic logic [BLOCK_BITS-1:0] mask_last_block(
        input logic [BLOCK_BITS-1:0] data,
        input logic [7:0]            valid_bytes,
        input logic                  is_last
    );
        logic [BLOCK_BITS-1:0] masked;
        begin
            masked = '0;
            if (!is_last) begin
                masked = data;
            end else begin
                for (int i = 0; i < BLOCK_BITS/8; i++) begin
                    if (i < valid_bytes) begin
                        masked[i*8 +: 8] = data[i*8 +: 8];
                    end
                end
            end
            return masked;
        end
    endfunction

    function automatic logic [BLOCK_BITS_COUNT_WIDTH-1:0] block_bits_to_add(
        input logic [7:0] valid_bytes,
        input logic       is_last
    );
        if (is_last) begin
            block_bits_to_add = {valid_bytes, 3'b000};
        end else begin
            block_bits_to_add = BLOCK_BITS_COUNT_WIDTH'(BLOCK_BITS);
        end
    endfunction

    function automatic logic [255:0] make_domain(
        input logic        is_last,
        input logic [127:0] hash_bits
    );
        make_domain = '0;
        make_domain[DOMAIN_VERSION_WORD*WORD_BITS +: WORD_BITS] = VERSION;
        make_domain[DOMAIN_FLAG_WORD*WORD_BITS +: WORD_BITS] =
            is_last ? 64'hffff_ffff_ffff_ffff : 64'h0;
        make_domain[2*WORD_BITS +: WORD_BITS] = hash_bits[127:64];
        make_domain[3*WORD_BITS +: WORD_BITS] = hash_bits[63:0];
    endfunction

    function automatic word_t byte_reverse64(input word_t x);
        byte_reverse64 = {
            x[ 7: 0], x[15: 8], x[23:16], x[31:24],
            x[39:32], x[47:40], x[55:48], x[63:56]
        };
    endfunction

    function automatic logic [DIGEST_BITS-1:0] format_digest(
        input logic [CHAIN_BITS-1:0] h
    );
        logic [DIGEST_BITS-1:0] digest;
        begin
            digest = '0;
            for (int i = 0; i < WORDS; i++) begin
                digest[(WORDS - 1 - i)*WORD_BITS +: WORD_BITS] =
                    byte_reverse64(h[i*WORD_BITS +: WORD_BITS]);
            end
            return digest;
        end
    endfunction

    localparam logic [CHAIN_BITS-1:0] H_INIT = make_h_init();

    logic [BLOCK_BITS_COUNT_WIDTH-1:0] bits_added_at_start;
    logic [127:0]                      hashed_bits_updated;

    assign bits_added_at_start = block_bits_to_add(valid_bytes, last);
    assign hashed_bits_updated = hashed_bits_reg + {{(128-BLOCK_BITS_COUNT_WIDTH){1'b0}}, bits_added_at_start};

    compression u_compression (
        .clk          (clk),
        .rst          (rst),
        .start     (cf_start),
        .h_in         (h_reg),
        .m            (cf_block),
        .d       (cf_domain),
        .done    (cf_done),
        .h_out        (cf_h_out)
    );

    assign done         = (state == DONE);
    assign digest       = format_digest(h_reg);
    assign busy         = (state == WAIT_CF);
    assign cf_block     = mask_last_block(block, valid_bytes, last);
    assign cf_domain    = make_domain(last, hashed_bits_updated);
    assign cf_start    = (state == IDLE) && start;
    assign bits_added     = block_bits_to_add(active_valid_bytes_reg, active_last_reg);

    always_comb begin
        state_nxt              = state;
        h_nxt                  = h_reg;
        hashed_bits_nxt        = hashed_bits_reg;
        active_last_nxt        = active_last_reg;
        active_valid_bytes_nxt = active_valid_bytes_reg;

        case (state)
            IDLE: begin
                if (start) begin
                    active_last_nxt        = last;
                    active_valid_bytes_nxt = valid_bytes;
                    state_nxt              = WAIT_CF;
                end
            end
            WAIT_CF: begin
                if (cf_done) begin
                    h_nxt           = cf_h_out;
                    hashed_bits_nxt = hashed_bits_reg + {{(128-BLOCK_BITS_COUNT_WIDTH){1'b0}}, bits_added};
                    if (active_last_reg) begin
                        state_nxt  = DONE;
                    end else begin
                        state_nxt = IDLE;
                    end
                end
            end
            DONE: begin
                state_nxt = DONE;
            end
            default: begin
                state_nxt = IDLE;
            end
        endcase
    end

    always_ff @(posedge clk) begin
        if (rst) begin
            state <= IDLE;
            h_reg <= H_INIT;
            hashed_bits_reg <= '0;
            active_last_reg <= 1'b0;
            active_valid_bytes_reg <= '0;
        end else begin
            state <= state_nxt;
            h_reg <= h_nxt;
            hashed_bits_reg <= hashed_bits_nxt;
            active_last_reg <= active_last_nxt;
            active_valid_bytes_reg <= active_valid_bytes_nxt;
        end
    end
endmodule

`endif
