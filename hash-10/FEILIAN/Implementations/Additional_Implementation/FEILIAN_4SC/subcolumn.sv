`ifndef SUBCOLUMN_SV
`define SUBCOLUMN_SV

module subcolumn
    import feilian_pkg::*;
(
    input  logic [3:0][WORD_BITS-1:0] state_i,
    output logic [3:0][WORD_BITS-1:0] state_o
);
    word_t a;
    word_t b;
    word_t c;
    word_t d;

    localparam int ROTR_D = 8;
    localparam int ROTR_B = 63;

    localparam int SIGMA0_R1 = 5;
    localparam int SIGMA0_R2 = 48;
    localparam int SIGMA1_R1 = 11;
    localparam int SIGMA1_R2 = 40;

    function automatic word_t rotr_d(input word_t x);
        rotr_d = {x[ROTR_D-1:0], x[WORD_BITS-1:ROTR_D]};
    endfunction

    function automatic word_t rotr_b(input word_t x);
        rotr_b = {x[ROTR_B-1:0], x[WORD_BITS-1:ROTR_B]};
    endfunction

    function automatic word_t sigma0(input word_t x);
        sigma0 = x
           ^ {x[SIGMA0_R1-1:0], x[WORD_BITS-1:SIGMA0_R1]}
           ^ {x[SIGMA0_R2-1:0], x[WORD_BITS-1:SIGMA0_R2]};
    endfunction

    function automatic word_t sigma1(input word_t x);
        sigma1 = x
            ^ {x[SIGMA1_R1-1:0], x[WORD_BITS-1:SIGMA1_R1]}
            ^ {x[SIGMA1_R2-1:0], x[WORD_BITS-1:SIGMA1_R2]};
    endfunction

    always_comb begin
        a = state_i[0] + state_i[1];
        c = state_i[2] + state_i[3];
        d = rotr_d(state_i[3] ^ a);
        c = c + sigma0(d);
        b = rotr_b(state_i[1] ^ c);
        a = a + sigma1(b);
    end

    assign state_o[0] = a;
    assign state_o[1] = b;
    assign state_o[2] = c;
    assign state_o[3] = d;
endmodule

`endif
