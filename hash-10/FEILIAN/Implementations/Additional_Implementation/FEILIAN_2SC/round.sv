`ifndef ROUND_SV
`define ROUND_SV

module round
    import feilian_pkg::*;
(
    input  logic clk,
    input  logic rst,
    input  logic start,
    input  logic [STATE_BITS-1:0] v_in,
    input  logic [BLOCK_BITS-1:0] m,
    input  logic [DOMAIN_BITS-1:0] d,
    input  logic [ROUND_GROUP_BITS-1:0] round_group,
    input  logic inject_en,
    output logic done,
    output logic [STATE_BITS-1:0] v_out
);
    typedef enum logic [1:0] {
        IDLE,
        RUN,
        DONE
    } state_t;

    typedef logic [3:0][WORD_BITS-1:0] column_t;
    typedef column_t columns_t [0:3];

    state_t state;
    state_t state_nxt;
    columns_t state_col_reg;
    columns_t state_col_nxt;
    columns_t sub_col_reg;
    columns_t sub_col_nxt;
    columns_t sub_col_with_pair;
    columns_t layer_result;
    column_t sc_in0;
    column_t sc_in1;
    column_t sc_out0;
    column_t sc_out1;
    logic round_phase_reg;
    logic round_phase_nxt;
    logic pair_reg;
    logic pair_nxt;

    function automatic logic [STATE_BITS-1:0] pack_columns(input columns_t columns);
        pack_columns = '0;
        for (int col = 0; col < 4; col++) begin
            for (int row = 0; row < 4; row++) begin
                pack_columns[(col + 4*row)*WORD_BITS +: WORD_BITS] = columns[col][row];
            end
        end
    endfunction

    function automatic column_t column_with_message(
        input column_t words,
        input logic [1:0] col,
        input logic inject,
        input logic phase,
        input logic [BLOCK_BITS-1:0] msg
    );
        column_with_message = words;
        if (inject) begin
            if (phase) begin
                column_with_message[0] =
                    words[0] ^ msg[(8 + col)*WORD_BITS +: WORD_BITS];
                column_with_message[2] =
                    words[2] ^ msg[(12 + col)*WORD_BITS +: WORD_BITS];
            end else begin
                column_with_message[0] =
                    words[0] ^ msg[col*WORD_BITS +: WORD_BITS];
                column_with_message[2] =
                    words[2] ^ msg[(4 + col)*WORD_BITS +: WORD_BITS];
            end
        end
    endfunction

    subcolumn u_subcolumn0 (
        .state_i (sc_in0),
        .state_o (sc_out0)
    );

    subcolumn u_subcolumn1 (
        .state_i (sc_in1),
        .state_o (sc_out1)
    );

    assign done  = (state == DONE);
    assign v_out = pack_columns(state_col_reg);

    always_comb begin
        if (pair_reg) begin
            sc_in0 = column_with_message(state_col_reg[2], 2'd2,
                                         inject_en, round_phase_reg, m);
            sc_in1 = column_with_message(state_col_reg[3], 2'd3,
                                         inject_en, round_phase_reg, m);
        end else begin
            sc_in0 = column_with_message(state_col_reg[0], 2'd0,
                                         inject_en, round_phase_reg, m);
            sc_in1 = column_with_message(state_col_reg[1], 2'd1,
                                         inject_en, round_phase_reg, m);
        end
    end

    always_comb begin
        sub_col_with_pair = sub_col_reg;
        if (pair_reg) begin
            sub_col_with_pair[2] = sc_out0;
            sub_col_with_pair[3] = sc_out1;
        end else begin
            sub_col_with_pair[0] = sc_out0;
            sub_col_with_pair[1] = sc_out1;
        end
    end

    always_comb begin
        word_t rc0;
        word_t rc1;
        word_t rc2;
        word_t rc3;

        unique case (round_group)
            2'd1: begin
                rc0 = ROUND_CONSTANTS[1];
                rc1 = ROUND_CONSTANTS[2];
                rc2 = ROUND_CONSTANTS[3];
                rc3 = ROUND_CONSTANTS[0];
            end
            2'd2: begin
                rc0 = ROUND_CONSTANTS[2];
                rc1 = ROUND_CONSTANTS[3];
                rc2 = ROUND_CONSTANTS[0];
                rc3 = ROUND_CONSTANTS[1];
            end
            2'd3: begin
                rc0 = ROUND_CONSTANTS[3];
                rc1 = ROUND_CONSTANTS[0];
                rc2 = ROUND_CONSTANTS[1];
                rc3 = ROUND_CONSTANTS[2];
            end
            default: begin
                rc0 = ROUND_CONSTANTS[0];
                rc1 = ROUND_CONSTANTS[1];
                rc2 = ROUND_CONSTANTS[2];
                rc3 = ROUND_CONSTANTS[3];
            end
        endcase

        layer_result[0][0] = sub_col_with_pair[0][0];
        layer_result[1][0] = sub_col_with_pair[1][0];
        layer_result[2][0] = sub_col_with_pair[2][0];
        layer_result[3][0] = sub_col_with_pair[3][0];
        layer_result[0][1] = sub_col_with_pair[1][1];
        layer_result[1][1] = sub_col_with_pair[2][1];
        layer_result[2][1] = sub_col_with_pair[3][1];
        layer_result[3][1] = sub_col_with_pair[0][1];
        layer_result[0][2] = sub_col_with_pair[2][2];
        layer_result[1][2] = sub_col_with_pair[3][2];
        layer_result[2][2] = sub_col_with_pair[0][2];
        layer_result[3][2] = sub_col_with_pair[1][2];
        layer_result[0][3] = sub_col_with_pair[3][3];
        layer_result[1][3] = sub_col_with_pair[0][3];
        layer_result[2][3] = sub_col_with_pair[1][3];
        layer_result[3][3] = sub_col_with_pair[2][3];

        if (round_phase_reg) begin
            layer_result[0][1] = layer_result[0][1] ^ rc0;
            layer_result[1][1] = layer_result[1][1] ^ rc1;
            layer_result[2][1] = layer_result[2][1] ^ rc2;
            layer_result[3][1] = layer_result[3][1] ^ rc3;
            layer_result[0][3] =
                layer_result[0][3] ^ d[DOMAIN_VERSION_WORD*WORD_BITS +: WORD_BITS];
            layer_result[1][3] =
                layer_result[1][3] ^ d[DOMAIN_FLAG_WORD*WORD_BITS +: WORD_BITS];
            layer_result[2][3] =
                layer_result[2][3] ^ d[2*WORD_BITS +: WORD_BITS];
            layer_result[3][3] =
                layer_result[3][3] ^ d[3*WORD_BITS +: WORD_BITS];
        end
    end

    always_comb begin
        state_nxt       = state;
        state_col_nxt   = state_col_reg;
        sub_col_nxt     = sub_col_reg;
        round_phase_nxt = round_phase_reg;
        pair_nxt        = pair_reg;

        if ((state == IDLE || state == DONE) && start) begin
            for (int col = 0; col < 4; col++) begin
                for (int row = 0; row < 4; row++) begin
                    state_col_nxt[col][row] = v_in[(col + 4*row)*WORD_BITS +: WORD_BITS];
                    sub_col_nxt[col][row] = '0;
                end
            end
            round_phase_nxt = 1'b0;
            pair_nxt        = 1'b0;
            state_nxt       = RUN;
        end else if (state == RUN) begin
            sub_col_nxt = sub_col_with_pair;
            if (!pair_reg) begin
                pair_nxt = 1'b1;
            end else begin
                state_col_nxt = layer_result;
                for (int col = 0; col < 4; col++) begin
                    for (int row = 0; row < 4; row++) begin
                        sub_col_nxt[col][row] = '0;
                    end
                end
                pair_nxt = 1'b0;

                if (!round_phase_reg) begin
                    round_phase_nxt = 1'b1;
                end else begin
                    round_phase_nxt = 1'b0;
                    state_nxt = DONE;
                end
            end
        end
    end

    always_ff @(posedge clk) begin
        if (rst) begin
            state <= IDLE;
            for (int col = 0; col < 4; col++) begin
                for (int row = 0; row < 4; row++) begin
                    state_col_reg[col][row] <= '0;
                    sub_col_reg[col][row] <= '0;
                end
            end
            round_phase_reg <= 1'b0;
            pair_reg <= 1'b0;
        end else begin
            state <= state_nxt;
            state_col_reg <= state_col_nxt;
            sub_col_reg <= sub_col_nxt;
            round_phase_reg <= round_phase_nxt;
            pair_reg <= pair_nxt;
        end
    end
endmodule

`endif
