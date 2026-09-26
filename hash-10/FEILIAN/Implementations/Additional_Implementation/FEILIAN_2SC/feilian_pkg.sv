`ifndef FEILIAN_PKG_SV
`define FEILIAN_PKG_SV

package feilian_pkg;
    localparam int MSG_BITS    = 1024;
    localparam int CHAIN_BITS  = 1024;
    localparam int STATE_BITS  = 1024;
    localparam int BLOCK_BITS  = 1024;
    localparam int DOMAIN_BITS = 256;
    localparam int DIGEST_BITS = 1024;
    localparam int ROUNDS      = 20;
    localparam int WORD_BITS   = 64;
    localparam int WORDS       = 16;
    localparam int GROUPS      = 5;
    localparam int INJECT_INTERVAL = ROUNDS / GROUPS;
    localparam int ROUND_CONSTANT_WORDS = 4;
    localparam int ROUND_GROUP_BITS = $clog2(ROUND_CONSTANT_WORDS);

    typedef logic [WORD_BITS-1:0] word_t;
    typedef word_t state_t [0:WORDS-1];

    localparam int ADDR_WIDTH = 8;
    localparam int DATA_WIDTH = 64;

    localparam logic [ADDR_WIDTH-1:0] ADDR_CTRL        = 8'h00;
    localparam logic [ADDR_WIDTH-1:0] ADDR_STATUS      = 8'h01;
    localparam logic [ADDR_WIDTH-1:0] ADDR_BLOCK_INFO  = 8'h02;
    localparam logic [ADDR_WIDTH-1:0] ADDR_MSG_BASE    = 8'h10;
    localparam logic [ADDR_WIDTH-1:0] ADDR_DIGEST_BASE = 8'h20;

    localparam word_t IV [0:15] = '{
        64'h243f6a8885a308d3, 64'h13198a2e03707344,
        64'ha4093822299f31d0, 64'h082efa98ec4e6c89,
        64'h452821e638d01377, 64'hbe5466cf34e90c6c,
        64'hc0ac29b7c97c50dd, 64'h3f84d5b5b5470917,
        64'h9216d5d98979fb1b, 64'hd1310ba698dfb5ac,
        64'h2ffd72dbd01adfb7, 64'hb8e1afed6a267e96,
        64'hba7c9045f12c7f99, 64'h24a19947b3916cf7,
        64'h0801f2e2858efc16, 64'h636920d871574e69
    };

    localparam word_t ROUND_CONSTANTS [0:ROUND_CONSTANT_WORDS-1] = '{
        64'h6a09e667f3bcc908,
        64'hbb67ae8584caa73b,
        64'h3c6ef372fe94f82b,
        64'ha54ff53a5f1d36f1
    };

    localparam word_t VERSION = 64'h400;

    localparam int DOMAIN_VERSION_WORD = 0;
    localparam int DOMAIN_FLAG_WORD    = 1;

    function automatic state_t unpack_words(input logic [WORDS*WORD_BITS-1:0] packed_words);
        for (int i = 0; i < WORDS; i++) begin
            unpack_words[i] = packed_words[i*WORD_BITS +: WORD_BITS];
        end
    endfunction

    function automatic logic [STATE_BITS-1:0] pack_state(input state_t words);
        pack_state = '0;
        for (int i = 0; i < WORDS; i++) begin
            pack_state[i*WORD_BITS +: WORD_BITS] = words[i];
        end
    endfunction

endpackage

`endif
