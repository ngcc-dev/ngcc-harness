`ifndef ROUND_SV
`define ROUND_SV

module round
    import feilian_pkg::*;
(
    input  logic [STATE_BITS-1:0] v_in,
    input  logic [DOMAIN_BITS-1:0] d,
    input  logic [ROUND_GROUP_BITS-1:0] round_group,
    input  logic add_constants,
    output logic [STATE_BITS-1:0] v_out
);
    logic [3:0][WORD_BITS-1:0] g_in  [0:3];
    logic [3:0][WORD_BITS-1:0] g_out [0:3];
    logic [STATE_BITS-1:0] step_out;
    word_t rc0;
    word_t rc1;
    word_t rc2;
    word_t rc3;
    genvar g_col;

    generate
        for (g_col = 0; g_col < 4; g_col++) begin : gen_g
            assign g_in[g_col][0] = v_in[(g_col     )*WORD_BITS +: WORD_BITS];
            assign g_in[g_col][1] = v_in[(g_col +  4)*WORD_BITS +: WORD_BITS];
            assign g_in[g_col][2] = v_in[(g_col +  8)*WORD_BITS +: WORD_BITS];
            assign g_in[g_col][3] = v_in[(g_col + 12)*WORD_BITS +: WORD_BITS];

            subcolumn u_subcolumn (
                .state_i(g_in[g_col]),
                .state_o(g_out[g_col])
            );
        end
    endgenerate

    assign step_out[ 0*WORD_BITS +: WORD_BITS] = g_out[0][0];
    assign step_out[ 1*WORD_BITS +: WORD_BITS] = g_out[1][0];
    assign step_out[ 2*WORD_BITS +: WORD_BITS] = g_out[2][0];
    assign step_out[ 3*WORD_BITS +: WORD_BITS] = g_out[3][0];
    assign step_out[ 4*WORD_BITS +: WORD_BITS] = g_out[1][1];
    assign step_out[ 5*WORD_BITS +: WORD_BITS] = g_out[2][1];
    assign step_out[ 6*WORD_BITS +: WORD_BITS] = g_out[3][1];
    assign step_out[ 7*WORD_BITS +: WORD_BITS] = g_out[0][1];
    assign step_out[ 8*WORD_BITS +: WORD_BITS] = g_out[2][2];
    assign step_out[ 9*WORD_BITS +: WORD_BITS] = g_out[3][2];
    assign step_out[10*WORD_BITS +: WORD_BITS] = g_out[0][2];
    assign step_out[11*WORD_BITS +: WORD_BITS] = g_out[1][2];
    assign step_out[12*WORD_BITS +: WORD_BITS] = g_out[3][3];
    assign step_out[13*WORD_BITS +: WORD_BITS] = g_out[0][3];
    assign step_out[14*WORD_BITS +: WORD_BITS] = g_out[1][3];
    assign step_out[15*WORD_BITS +: WORD_BITS] = g_out[2][3];

    always_comb begin
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

        v_out = step_out;
        if (add_constants) begin
            v_out[ 4*WORD_BITS +: WORD_BITS] = step_out[ 4*WORD_BITS +: WORD_BITS] ^ rc0;
            v_out[ 5*WORD_BITS +: WORD_BITS] = step_out[ 5*WORD_BITS +: WORD_BITS] ^ rc1;
            v_out[ 6*WORD_BITS +: WORD_BITS] = step_out[ 6*WORD_BITS +: WORD_BITS] ^ rc2;
            v_out[ 7*WORD_BITS +: WORD_BITS] = step_out[ 7*WORD_BITS +: WORD_BITS] ^ rc3;
            v_out[12*WORD_BITS +: WORD_BITS] =
                step_out[12*WORD_BITS +: WORD_BITS] ^ d[DOMAIN_VERSION_WORD*WORD_BITS +: WORD_BITS];
            v_out[13*WORD_BITS +: WORD_BITS] =
                step_out[13*WORD_BITS +: WORD_BITS] ^ d[DOMAIN_FLAG_WORD*WORD_BITS +: WORD_BITS];
            v_out[14*WORD_BITS +: WORD_BITS] =
                step_out[14*WORD_BITS +: WORD_BITS] ^ d[2*WORD_BITS +: WORD_BITS];
            v_out[15*WORD_BITS +: WORD_BITS] =
                step_out[15*WORD_BITS +: WORD_BITS] ^ d[3*WORD_BITS +: WORD_BITS];
        end
    end
endmodule

`endif
