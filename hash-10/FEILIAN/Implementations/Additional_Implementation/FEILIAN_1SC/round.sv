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
    columns_t sub_col_with_cur;
    columns_t round_col_result;
    column_t cur_col_reg;
    column_t cur_col_nxt;
    column_t col_out;
    logic round_phase_reg;
    logic round_phase_nxt;
    logic [1:0] col_reg;
    logic [1:0] col_nxt;

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
                unique case (col)
                    2'd0: begin
                        column_with_message[0] = words[0] ^ msg[ 8*WORD_BITS +: WORD_BITS];
                        column_with_message[2] = words[2] ^ msg[12*WORD_BITS +: WORD_BITS];
                    end
                    2'd1: begin
                        column_with_message[0] = words[0] ^ msg[ 9*WORD_BITS +: WORD_BITS];
                        column_with_message[2] = words[2] ^ msg[13*WORD_BITS +: WORD_BITS];
                    end
                    2'd2: begin
                        column_with_message[0] = words[0] ^ msg[10*WORD_BITS +: WORD_BITS];
                        column_with_message[2] = words[2] ^ msg[14*WORD_BITS +: WORD_BITS];
                    end
                    default: begin
                        column_with_message[0] = words[0] ^ msg[11*WORD_BITS +: WORD_BITS];
                        column_with_message[2] = words[2] ^ msg[15*WORD_BITS +: WORD_BITS];
                    end
                endcase
            end else begin
                unique case (col)
                    2'd0: begin
                        column_with_message[0] = words[0] ^ msg[0*WORD_BITS +: WORD_BITS];
                        column_with_message[2] = words[2] ^ msg[4*WORD_BITS +: WORD_BITS];
                    end
                    2'd1: begin
                        column_with_message[0] = words[0] ^ msg[1*WORD_BITS +: WORD_BITS];
                        column_with_message[2] = words[2] ^ msg[5*WORD_BITS +: WORD_BITS];
                    end
                    2'd2: begin
                        column_with_message[0] = words[0] ^ msg[2*WORD_BITS +: WORD_BITS];
                        column_with_message[2] = words[2] ^ msg[6*WORD_BITS +: WORD_BITS];
                    end
                    default: begin
                        column_with_message[0] = words[0] ^ msg[3*WORD_BITS +: WORD_BITS];
                        column_with_message[2] = words[2] ^ msg[7*WORD_BITS +: WORD_BITS];
                    end
                endcase
            end
        end
    endfunction

    subcolumn u_subcolumn (
        .state_i (cur_col_reg),
        .state_o (col_out)
    );

    assign done  = (state == DONE);
    assign v_out = pack_columns(state_col_reg);

    always_comb begin
        sub_col_with_cur = sub_col_reg;
        unique case (col_reg)
            2'd0: sub_col_with_cur[0] = col_out;
            2'd1: sub_col_with_cur[1] = col_out;
            2'd2: sub_col_with_cur[2] = col_out;
            default: sub_col_with_cur[3] = col_out;
        endcase
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

        round_col_result[0][0] = sub_col_with_cur[0][0];
        round_col_result[1][0] = sub_col_with_cur[1][0];
        round_col_result[2][0] = sub_col_with_cur[2][0];
        round_col_result[3][0] = sub_col_with_cur[3][0];
        round_col_result[0][1] = sub_col_with_cur[1][1];
        round_col_result[1][1] = sub_col_with_cur[2][1];
        round_col_result[2][1] = sub_col_with_cur[3][1];
        round_col_result[3][1] = sub_col_with_cur[0][1];
        round_col_result[0][2] = sub_col_with_cur[2][2];
        round_col_result[1][2] = sub_col_with_cur[3][2];
        round_col_result[2][2] = sub_col_with_cur[0][2];
        round_col_result[3][2] = sub_col_with_cur[1][2];
        round_col_result[0][3] = sub_col_with_cur[3][3];
        round_col_result[1][3] = sub_col_with_cur[0][3];
        round_col_result[2][3] = sub_col_with_cur[1][3];
        round_col_result[3][3] = sub_col_with_cur[2][3];

        if (round_phase_reg) begin
            round_col_result[0][1] =
                round_col_result[0][1] ^ rc0;
            round_col_result[1][1] =
                round_col_result[1][1] ^ rc1;
            round_col_result[2][1] =
                round_col_result[2][1] ^ rc2;
            round_col_result[3][1] =
                round_col_result[3][1] ^ rc3;
            round_col_result[0][3] =
                round_col_result[0][3] ^ d[DOMAIN_VERSION_WORD*WORD_BITS +: WORD_BITS];
            round_col_result[1][3] =
                round_col_result[1][3] ^ d[DOMAIN_FLAG_WORD*WORD_BITS +: WORD_BITS];
            round_col_result[2][3] =
                round_col_result[2][3] ^ d[2*WORD_BITS +: WORD_BITS];
            round_col_result[3][3] =
                round_col_result[3][3] ^ d[3*WORD_BITS +: WORD_BITS];
        end
    end

    always_comb begin
        state_nxt       = state;
        state_col_nxt   = state_col_reg;
        sub_col_nxt     = sub_col_reg;
        cur_col_nxt     = cur_col_reg;
        round_phase_nxt = round_phase_reg;
        col_nxt         = col_reg;

        if ((state == IDLE || state == DONE) && start) begin
            for (int col = 0; col < 4; col++) begin
                for (int row = 0; row < 4; row++) begin
                    state_col_nxt[col][row] = v_in[(col + 4*row)*WORD_BITS +: WORD_BITS];
                    sub_col_nxt[col][row] = '0;
                end
            end
            cur_col_nxt[0]   = v_in[ 0*WORD_BITS +: WORD_BITS] ^
                               (inject_en ? m[0*WORD_BITS +: WORD_BITS] : '0);
            cur_col_nxt[1]   = v_in[ 4*WORD_BITS +: WORD_BITS];
            cur_col_nxt[2]   = v_in[ 8*WORD_BITS +: WORD_BITS] ^
                               (inject_en ? m[4*WORD_BITS +: WORD_BITS] : '0);
            cur_col_nxt[3]   = v_in[12*WORD_BITS +: WORD_BITS];
            round_phase_nxt  = 1'b0;
            col_nxt          = '0;
            state_nxt        = RUN;
        end else if (state == RUN) begin
            sub_col_nxt = sub_col_with_cur;
            unique case (col_reg)
                2'd0: begin
                    cur_col_nxt = column_with_message(state_col_reg[1], 2'd1,
                                                      inject_en, round_phase_reg, m);
                    col_nxt = 2'd1;
                end
                2'd1: begin
                    cur_col_nxt = column_with_message(state_col_reg[2], 2'd2,
                                                      inject_en, round_phase_reg, m);
                    col_nxt = 2'd2;
                end
                2'd2: begin
                    cur_col_nxt = column_with_message(state_col_reg[3], 2'd3,
                                                      inject_en, round_phase_reg, m);
                    col_nxt = 2'd3;
                end
                default: begin
                    state_col_nxt = round_col_result;
                    for (int col = 0; col < 4; col++) begin
                        for (int row = 0; row < 4; row++) begin
                            sub_col_nxt[col][row] = '0;
                        end
                    end
                    col_nxt = '0;

                    if (!round_phase_reg) begin
                        round_phase_nxt = 1'b1;
                        cur_col_nxt = column_with_message(round_col_result[0], 2'd0,
                                                          inject_en, 1'b1, m);
                    end else begin
                        round_phase_nxt = 1'b0;
                        state_nxt = DONE;
                    end
                end
            endcase
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
            cur_col_reg <= '0;
            round_phase_reg <= 1'b0;
            col_reg <= '0;
        end else begin
            state <= state_nxt;
            state_col_reg <= state_col_nxt;
            sub_col_reg <= sub_col_nxt;
            cur_col_reg <= cur_col_nxt;
            round_phase_reg <= round_phase_nxt;
            col_reg <= col_nxt;
        end
    end
endmodule

`endif
