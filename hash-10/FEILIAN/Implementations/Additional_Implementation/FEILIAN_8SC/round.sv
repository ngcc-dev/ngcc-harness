`ifndef ROUND_SV
`define ROUND_SV

module round
    import feilian_pkg::*;
(
    input  logic [STATE_BITS-1:0] v_in,
    input  logic [BLOCK_BITS-1:0] m,
    input  logic [DOMAIN_BITS-1:0]       d,
    input  logic [ROUND_GROUP_BITS-1:0] round_group,
    input  logic               inject_en,
    output logic [STATE_BITS-1:0] v_out
);
    function automatic state_t xor_message(
        input state_t words,
        input state_t msg,
        input int unsigned base
    );
        xor_message = words;
        for (int col = 0; col < 4; col++) begin
            xor_message[col]     = words[col]     ^ msg[base + col];
            xor_message[col + 8] = words[col + 8] ^ msg[base + 4 + col];
        end
    endfunction

    function automatic state_t shift_rows(input state_t words);
        shift_rows[0]  = words[0];
        shift_rows[1]  = words[1];
        shift_rows[2]  = words[2];
        shift_rows[3]  = words[3];
        shift_rows[4]  = words[5];
        shift_rows[5]  = words[6];
        shift_rows[6]  = words[7];
        shift_rows[7]  = words[4];
        shift_rows[8]  = words[10];
        shift_rows[9]  = words[11];
        shift_rows[10] = words[8];
        shift_rows[11] = words[9];
        shift_rows[12] = words[15];
        shift_rows[13] = words[12];
        shift_rows[14] = words[13];
        shift_rows[15] = words[14];
    endfunction

    function automatic state_t add_round_constants(
        input state_t words,
        input logic [ROUND_GROUP_BITS-1:0] round_group,
        input logic [255:0] d
    );
        int unsigned rc_index;

        add_round_constants = words;
        for (int i = 0; i < ROUND_CONSTANT_WORDS; i++) begin
            rc_index = (int'(round_group) + i) % ROUND_CONSTANT_WORDS;
            add_round_constants[4 + i] = words[4 + i] ^ ROUND_CONSTANTS[rc_index];
        end
        add_round_constants[12] = words[12] ^ d[DOMAIN_VERSION_WORD*WORD_BITS +: WORD_BITS];
        add_round_constants[13] = words[13] ^ d[DOMAIN_FLAG_WORD*WORD_BITS +: WORD_BITS];
        add_round_constants[14] = words[14] ^ d[2*WORD_BITS +: WORD_BITS];
        add_round_constants[15] = words[15] ^ d[3*WORD_BITS +: WORD_BITS];
    endfunction

    state_t v_words;
    state_t msg_words;
    state_t phase0_words;
    state_t phase0_sub_words;
    state_t phase1_words;
    state_t phase1_sub_words;
    state_t result_words;
    logic [3:0][WORD_BITS-1:0] phase0_g_in  [0:3];
    logic [3:0][WORD_BITS-1:0] phase0_g_out [0:3];
    logic [3:0][WORD_BITS-1:0] phase1_g_in  [0:3];
    logic [3:0][WORD_BITS-1:0] phase1_g_out [0:3];
    genvar g_col;

    generate
        for (g_col = 0; g_col < 4; g_col++) begin : gen_phase0_g
            subcolumn u_subcolumn (
                .state_i(phase0_g_in[g_col]),
                .state_o(phase0_g_out[g_col])
            );
        end

        for (g_col = 0; g_col < 4; g_col++) begin : gen_phase1_g
            subcolumn u_subcolumn (
                .state_i(phase1_g_in[g_col]),
                .state_o(phase1_g_out[g_col])
            );
        end
    endgenerate

    always_comb begin
        v_words = unpack_words(v_in);
        msg_words = unpack_words(m);

        for (int col = 0; col < 4; col++) begin
            phase0_g_in[col] = '0;
            phase1_g_in[col] = '0;
        end
        for (int i = 0; i < WORDS; i++) begin
            phase0_sub_words[i] = '0;
            phase1_sub_words[i] = '0;
        end

        phase0_words = v_words;
        if (inject_en) begin
            phase0_words = xor_message(phase0_words, msg_words, 0);
        end
        for (int col = 0; col < 4; col++) begin
            phase0_g_in[col][0] = phase0_words[col];
            phase0_g_in[col][1] = phase0_words[col + 4];
            phase0_g_in[col][2] = phase0_words[col + 8];
            phase0_g_in[col][3] = phase0_words[col + 12];

            phase0_sub_words[col]      = phase0_g_out[col][0];
            phase0_sub_words[col + 4]  = phase0_g_out[col][1];
            phase0_sub_words[col + 8]  = phase0_g_out[col][2];
            phase0_sub_words[col + 12] = phase0_g_out[col][3];
        end
        phase0_words = shift_rows(phase0_sub_words);

        phase1_words = phase0_words;
        if (inject_en) begin
            phase1_words = xor_message(phase1_words, msg_words, 8);
        end
        for (int col = 0; col < 4; col++) begin
            phase1_g_in[col][0] = phase1_words[col];
            phase1_g_in[col][1] = phase1_words[col + 4];
            phase1_g_in[col][2] = phase1_words[col + 8];
            phase1_g_in[col][3] = phase1_words[col + 12];

            phase1_sub_words[col]      = phase1_g_out[col][0];
            phase1_sub_words[col + 4]  = phase1_g_out[col][1];
            phase1_sub_words[col + 8]  = phase1_g_out[col][2];
            phase1_sub_words[col + 12] = phase1_g_out[col][3];
        end
        phase1_words = shift_rows(phase1_sub_words);

        result_words = add_round_constants(phase1_words, round_group, d);
        v_out = pack_state(result_words);
    end
endmodule

`endif
