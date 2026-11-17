`timescale 1ns / 1ps
`define  MAX_W        128      // 最大字宽（硬件固定128bit，动态适配小位宽）
`define  ROWS         24       // 固定行数
`define  RATE_ROWS    4        // 固定Rate行数
`define  CAP_ROWS     20       // 固定Cap行数
`define  MAX_NR       40       // 最大轮数（W=128时40轮）
`define  AXI_DATA_W   128       // AXI-Lite数据位宽
`define  AXI_ADDR_W   32        // AXI-Lite地址位宽

// W配置选择编码
`define  W_32         2'b00
`define  W_64         2'b01
`define  W_96         2'b10
`define  W_128        2'b11
module hash_core #(
    parameter W_MAX = `MAX_W,
    parameter ROWS  = `ROWS,
    parameter RATE  = `RATE_ROWS,
    parameter CAP   = `CAP_ROWS,
    parameter MAX_NR= `MAX_NR
) (
    input  wire                 clk,
    input  wire                 rst_n,
    // 配置接口
    input  wire [1:0]           w_sel,
    input  wire                 start,
    output wire                 core_ready,

    input  wire [63:0]          msg_len,
    input  wire [W_MAX*8-1:0]   data_in,
    input  wire                 padding_flag,
    output wire                 idle,
    output wire                 busy,
    output reg                  done,
    output wire [W_MAX*8-1:0]   data_out
);

// 状态定义
localparam IDLE     = 2'b00;
localparam PERMUTE  = 2'b01;
localparam SQUEEZ   = 2'b10;
localparam DONE     = 2'b11;

// 算法状态：24行x128bit
reg [W_MAX-1:0]     state [0:ROWS-1];
reg [W_MAX-1:0]     update_state [0:ROWS-1];
reg [W_MAX-1:0]     next_state_data_tmp [0:ROWS-1];
reg [W_MAX-1:0]     mi_s [0:ROWS-1];
reg [1:0]           curr_state;
reg [1:0]           next_state;
// 轮数计数器
reg [5:0]           round_cnt;
reg [5:0]           nr;          // 轮数
reg [W_MAX-1:0]     T [0:35];        // 优化轮函数临时变量(T0~T35)
reg                 last_msg_flag;

wire [W_MAX-1:0]    next_state_last_w;
wire [6:0]          sigma [0:ROWS-1];
reg  [W_MAX-1:0]    rc [0:MAX_NR-1];
reg  [W_MAX-1:0]    shift_out [0:ROWS-1];
wire [W_MAX-1:0]    shift_out_256 [0:ROWS-1];
wire [W_MAX-1:0]    shift_out_512 [0:ROWS-1];
wire [W_MAX-1:0]    shift_out_768 [0:ROWS-1];
wire [W_MAX-1:0]    shift_out_1024 [0:ROWS-1];
wire [W_MAX-1:0]     rc_256 [0:39];
wire [W_MAX-1:0]     rc_512 [0:39];
wire [W_MAX-1:0]     rc_768 [0:39];
wire [W_MAX-1:0]     rc_1024[0:39];


always @(*)begin
    case(w_sel)
        `W_32:  begin
            mi_s[0] = {96'd0,data_in[999:992], data_in[1007:1000], data_in[1015:1008], data_in[1023:1016]};
            mi_s[1] = {96'd0,data_in[967:960], data_in[975:968], data_in[983:976], data_in[991:984]};
            mi_s[2] = {96'd0,data_in[935:928], data_in[943:936], data_in[951:944], data_in[959:952]};
            mi_s[3] = {96'd0,data_in[903:896], data_in[911:904], data_in[919:912], data_in[927:920]};
            mi_s[4] = {96'd0,data_in[871:864], data_in[879:872], data_in[887:880], data_in[895:888]};
            mi_s[5] = {96'd0,data_in[839:832], data_in[847:840], data_in[855:848], data_in[863:856]};
            mi_s[6] = {96'd0,data_in[807:800], data_in[815:808], data_in[823:816], data_in[831:824]};
            mi_s[7] = {96'd0,data_in[775:768], data_in[783:776], data_in[791:784], data_in[799:792]};
            mi_s[8] = update_state[8];
            mi_s[9] = update_state[9];
            mi_s[10] = update_state[10];
            mi_s[11] = update_state[11];
            mi_s[12] = update_state[12];
            mi_s[13] = update_state[13];
            mi_s[14] = update_state[14];
            mi_s[15] = update_state[15];
            mi_s[16] = update_state[16];
            mi_s[17] = update_state[17];
            mi_s[18] = update_state[18];
            mi_s[19] = update_state[19];
            mi_s[20] = update_state[20];
            mi_s[21] = update_state[21];
            mi_s[22] = update_state[22];
            mi_s[23] = update_state[23];
        end
        `W_64:  begin
            mi_s[0] = {64'd0,data_in[967:960], data_in[975:968], data_in[983:976], data_in[991:984],
                             data_in[999:992], data_in[1007:1000], data_in[1015:1008], data_in[1023:1016]};
            mi_s[1] = {64'd0,data_in[903:896], data_in[911:904], data_in[919:912], data_in[927:920],
                             data_in[935:928], data_in[943:936], data_in[951:944], data_in[959:952]};
            mi_s[2] = {64'd0,data_in[839:832], data_in[847:840], data_in[855:848], data_in[863:856],
                             data_in[871:864], data_in[879:872], data_in[887:880], data_in[895:888]};
            mi_s[3] = {64'd0,data_in[775:768], data_in[783:776], data_in[791:784], data_in[799:792],
                             data_in[807:800], data_in[815:808], data_in[823:816], data_in[831:824]};
            mi_s[4] = {64'd0,data_in[711:704], data_in[719:712], data_in[727:720], data_in[735:728], 
                             data_in[743:736], data_in[751:744], data_in[759:752], data_in[767:760]};
            mi_s[5] = {64'd0,data_in[647:640], data_in[655:648], data_in[663:656], data_in[671:664], 
                             data_in[679:672], data_in[687:680], data_in[695:688], data_in[703:696]};
            mi_s[6] = {64'd0,data_in[583:576], data_in[591:584], data_in[599:592], data_in[607:600], 
                             data_in[615:608], data_in[623:616], data_in[631:624], data_in[639:632]};
            mi_s[7] = {64'd0,data_in[519:512], data_in[527:520], data_in[535:528], data_in[543:536],
                             data_in[551:544], data_in[559:552], data_in[567:560], data_in[575:568]};
            mi_s[8] = update_state[8];
            mi_s[9] = update_state[9];
            mi_s[10] = update_state[10];
            mi_s[11] = update_state[11];
            mi_s[12] = update_state[12];
            mi_s[13] = update_state[13];
            mi_s[14] = update_state[14];
            mi_s[15] = update_state[15];
            mi_s[16] = update_state[16];
            mi_s[17] = update_state[17];
            mi_s[18] = update_state[18];
            mi_s[19] = update_state[19];
            mi_s[20] = update_state[20];
            mi_s[21] = update_state[21];
            mi_s[22] = update_state[22];
            mi_s[23] = update_state[23];
        end
        `W_96:  begin
            mi_s[0] = {32'd0,data_in[935:928], data_in[943:936], data_in[951:944], data_in[959:952],
                             data_in[967:960], data_in[975:968], data_in[983:976], data_in[991:984],
                             data_in[999:992], data_in[1007:1000], data_in[1015:1008], data_in[1023:1016]};
            mi_s[1] = {32'd0,data_in[839:832], data_in[847:840], data_in[855:848], data_in[863:856], 
                             data_in[871:864], data_in[879:872], data_in[887:880], data_in[895:888], 
                             data_in[903:896], data_in[911:904], data_in[919:912], data_in[927:920]};
            mi_s[2] = {32'd0,data_in[743:736], data_in[751:744], data_in[759:752], data_in[767:760], 
                             data_in[775:768], data_in[783:776], data_in[791:784], data_in[799:792], 
                             data_in[807:800], data_in[815:808], data_in[823:816], data_in[831:824]};
            mi_s[3] = {32'd0,data_in[647:640], data_in[655:648], data_in[663:656], data_in[671:664], 
                             data_in[679:672], data_in[687:680], data_in[695:688], data_in[703:696], 
                             data_in[711:704], data_in[719:712], data_in[727:720], data_in[735:728]};
            mi_s[4] = {32'd0,data_in[551:544], data_in[559:552], data_in[567:560], data_in[575:568], 
                             data_in[583:576], data_in[591:584], data_in[599:592], data_in[607:600], 
                             data_in[615:608], data_in[623:616], data_in[631:624], data_in[639:632]};
            mi_s[5] = {32'd0,data_in[455:448], data_in[463:456], data_in[471:464], data_in[479:472], 
                             data_in[487:480], data_in[495:488], data_in[503:496], data_in[511:504],
                             data_in[519:512], data_in[527:520], data_in[535:528], data_in[543:536]};
            mi_s[6] = {32'd0,data_in[359:352], data_in[367:360], data_in[375:368], data_in[383:376],
                             data_in[391:384], data_in[399:392], data_in[407:400], data_in[415:408], 
                             data_in[423:416], data_in[431:424], data_in[439:432], data_in[447:440]};
            mi_s[7] = {32'd0,data_in[263:256], data_in[271:264], data_in[279:272], data_in[287:280], 
                             data_in[295:288], data_in[303:296], data_in[311:304], data_in[319:312], 
                             data_in[327:320], data_in[335:328], data_in[343:336], data_in[351:344]};
            mi_s[8] = update_state[8];
            mi_s[9] = update_state[9];
            mi_s[10] = update_state[10];
            mi_s[11] = update_state[11];
            mi_s[12] = update_state[12];
            mi_s[13] = update_state[13];
            mi_s[14] = update_state[14];
            mi_s[15] = update_state[15];
            mi_s[16] = update_state[16];
            mi_s[17] = update_state[17];
            mi_s[18] = update_state[18];
            mi_s[19] = update_state[19];
            mi_s[20] = update_state[20];
            mi_s[21] = update_state[21];
            mi_s[22] = update_state[22];
            mi_s[23] = update_state[23];
        end
        default:begin
            mi_s[0] = {data_in[903:896], data_in[911:904], data_in[919:912], data_in[927:920], 
                       data_in[935:928], data_in[943:936], data_in[951:944], data_in[959:952],
                       data_in[967:960], data_in[975:968], data_in[983:976], data_in[991:984], 
                       data_in[999:992], data_in[1007:1000], data_in[1015:1008], data_in[1023:1016]};
            mi_s[1] = {data_in[775:768], data_in[783:776], data_in[791:784], data_in[799:792],
                       data_in[807:800], data_in[815:808], data_in[823:816], data_in[831:824], 
                       data_in[839:832], data_in[847:840], data_in[855:848], data_in[863:856], 
                       data_in[871:864], data_in[879:872], data_in[887:880], data_in[895:888]};
            mi_s[2] = {data_in[647:640], data_in[655:648], data_in[663:656], data_in[671:664], 
                       data_in[679:672], data_in[687:680], data_in[695:688], data_in[703:696], 
                       data_in[711:704], data_in[719:712], data_in[727:720], data_in[735:728], 
                       data_in[743:736], data_in[751:744], data_in[759:752], data_in[767:760]};
            mi_s[3] = {data_in[519:512], data_in[527:520], data_in[535:528], data_in[543:536],
                       data_in[551:544], data_in[559:552], data_in[567:560], data_in[575:568], 
                       data_in[583:576], data_in[591:584], data_in[599:592], data_in[607:600], 
                       data_in[615:608], data_in[623:616], data_in[631:624], data_in[639:632]};
            mi_s[4] = {data_in[391:384], data_in[399:392], data_in[407:400], data_in[415:408], 
                       data_in[423:416], data_in[431:424], data_in[439:432], data_in[447:440],
                       data_in[455:448], data_in[463:456], data_in[471:464], data_in[479:472], 
                       data_in[487:480], data_in[495:488], data_in[503:496], data_in[511:504]};
            mi_s[5] = {data_in[263:256], data_in[271:264], data_in[279:272], data_in[287:280], 
                       data_in[295:288], data_in[303:296], data_in[311:304], data_in[319:312], 
                       data_in[327:320], data_in[335:328], data_in[343:336], data_in[351:344], 
                       data_in[359:352], data_in[367:360], data_in[375:368], data_in[383:376]};
            mi_s[6] = {data_in[135:128], data_in[143:136], data_in[151:144], data_in[159:152], 
                       data_in[167:160], data_in[175:168], data_in[183:176], data_in[191:184], 
                       data_in[199:192], data_in[207:200], data_in[215:208], data_in[223:216], 
                       data_in[231:224], data_in[239:232], data_in[247:240], data_in[255:248]};
            mi_s[7] = {data_in[7:0], data_in[15:8], data_in[23:16], data_in[31:24], 
                       data_in[39:32], data_in[47:40], data_in[55:48], data_in[63:56],
                       data_in[71:64], data_in[79:72], data_in[87:80], data_in[95:88], 
                       data_in[103:96], data_in[111:104], data_in[119:112], data_in[127:120]};
            mi_s[8] = update_state[8];
            mi_s[9] = update_state[9];
            mi_s[10] = update_state[10];
            mi_s[11] = update_state[11];
            mi_s[12] = update_state[12];
            mi_s[13] = update_state[13];
            mi_s[14] = update_state[14];
            mi_s[15] = update_state[15];
            mi_s[16] = update_state[16];
            mi_s[17] = update_state[17];
            mi_s[18] = update_state[18];
            mi_s[19] = update_state[19];
            mi_s[20] = update_state[20];
            mi_s[21] = update_state[21];
            mi_s[22] = update_state[22];
            mi_s[23] = update_state[23];
        end
    endcase
end

assign idle  = (curr_state == IDLE);
assign busy  = (curr_state != IDLE);
assign core_ready = (curr_state == PERMUTE) && (round_cnt == nr-1);
always @(posedge clk ) begin
    if(!rst_n) begin
        done <= 1'd0;
    end
    else if (curr_state == SQUEEZ && round_cnt == nr-1)begin
        done <= 1'd1;
    end
end
// 动态参数：轮数/位宽
always @(*) begin
    case(w_sel)
        `W_32:  begin nr = 6'd16; end
        `W_64:  begin nr = 6'd24; end
        `W_96:  begin nr = 6'd32; end
        default:begin nr = 6'd40; end
    endcase
end

    barrel_shifter_256 u_shift_256_0(
        .din(next_state_data_tmp[0]),
        .shift_num(7'd3),
        .dout(shift_out_256[0])
    );
    barrel_shifter_256 u_shift_256_1(
        .din(next_state_data_tmp[1]),
        .shift_num(7'd16),
        .dout(shift_out_256[1])
    );
    barrel_shifter_256 u_shift_256_2(
        .din(next_state_data_tmp[2]),
        .shift_num(7'd13),
        .dout(shift_out_256[2])
    );
    barrel_shifter_256 u_shift_256_3(
        .din(next_state_data_tmp[3]),
        .shift_num(7'd26),
        .dout(shift_out_256[3])
    );
    barrel_shifter_256 u_shift_256_4(
        .din(next_state_data_tmp[4]),
        .shift_num(7'd8),
        .dout(shift_out_256[4])
    );
    barrel_shifter_256 u_shift_256_5(
        .din(next_state_data_tmp[5]),
        .shift_num(7'd29),
        .dout(shift_out_256[5])
    );
    barrel_shifter_256 u_shift_256_6(
        .din(next_state_data_tmp[6]),
        .shift_num(7'd2),
        .dout(shift_out_256[6])
    );
    barrel_shifter_256 u_shift_256_7(
        .din(next_state_data_tmp[7]),
        .shift_num(7'd23),
        .dout(shift_out_256[7])
    );
    barrel_shifter_256 u_shift_256_8(
        .din(next_state_data_tmp[8]),
        .shift_num(7'd21),
        .dout(shift_out_256[8])
    );
    barrel_shifter_256 u_shift_256_9(
        .din(next_state_data_tmp[9]),
        .shift_num(7'd18),
        .dout(shift_out_256[9])
    );
    barrel_shifter_256 u_shift_256_10(
        .din(next_state_data_tmp[10]),
        .shift_num(7'd31),
        .dout(shift_out_256[10])
    );
    barrel_shifter_256 u_shift_256_11(
        .din(next_state_data_tmp[11]),
        .shift_num(7'd28),
        .dout(shift_out_256[11])
    );
    barrel_shifter_256 u_shift_256_12(
        .din(next_state_data_tmp[12]),
        .shift_num(7'd10),
        .dout(shift_out_256[12])
    );
    barrel_shifter_256 u_shift_256_13(
        .din(next_state_data_tmp[13]),
        .shift_num(7'd15),
        .dout(shift_out_256[13])
    );
    barrel_shifter_256 u_shift_256_14(
        .din(next_state_data_tmp[14]),
        .shift_num(7'd4),
        .dout(shift_out_256[14])
    );
    barrel_shifter_256 u_shift_256_15(
        .din(next_state_data_tmp[15]),
        .shift_num(7'd9),
        .dout(shift_out_256[15])
    );
    barrel_shifter_256 u_shift_256_16(
        .din(next_state_data_tmp[16]),
        .shift_num(7'd7),
        .dout(shift_out_256[16])
    );
    barrel_shifter_256 u_shift_256_17(
        .din(next_state_data_tmp[17]),
        .shift_num(7'd20),
        .dout(shift_out_256[17])
    );
    barrel_shifter_256 u_shift_256_18(
        .din(next_state_data_tmp[18]),
        .shift_num(7'd17),
        .dout(shift_out_256[18])
    );
    barrel_shifter_256 u_shift_256_19(
        .din(next_state_data_tmp[19]),
        .shift_num(7'd30),
        .dout(shift_out_256[19])
    );
    barrel_shifter_256 u_shift_256_20(
        .din(next_state_data_tmp[20]),
        .shift_num(7'd12),
        .dout(shift_out_256[20])
    );
    barrel_shifter_256 u_shift_256_21(
        .din(next_state_data_tmp[21]),
        .shift_num(7'd1),
        .dout(shift_out_256[21])
    );
    barrel_shifter_256 u_shift_256_22(
        .din(next_state_data_tmp[22]),
        .shift_num(7'd6),
        .dout(shift_out_256[22])
    );
    barrel_shifter_256 u_shift_256_23(
        .din(next_state_data_tmp[23]),
        .shift_num(7'd27),
        .dout(shift_out_256[23])
    );

    barrel_shifter_512 u_shift_512_0(
        .din(next_state_data_tmp[0]),
        .shift_num(7'd2),
        .dout(shift_out_512[0])
    );
    barrel_shifter_512 u_shift_512_1(
        .din(next_state_data_tmp[1]),
        .shift_num(7'd25),
        .dout(shift_out_512[1])
    );
    barrel_shifter_512 u_shift_512_2(
        .din(next_state_data_tmp[2]),
        .shift_num(7'd20),
        .dout(shift_out_512[2])
    );
    barrel_shifter_512 u_shift_512_3(
        .din(next_state_data_tmp[3]),
        .shift_num(7'd51),
        .dout(shift_out_512[3])
    );
    barrel_shifter_512 u_shift_512_4(
        .din(next_state_data_tmp[4]),
        .shift_num(7'd15),
        .dout(shift_out_512[4])
    );
    barrel_shifter_512 u_shift_512_5(
        .din(next_state_data_tmp[5]),
        .shift_num(7'd40),
        .dout(shift_out_512[5])
    );
    barrel_shifter_512 u_shift_512_6(
        .din(next_state_data_tmp[6]),
        .shift_num(7'd37),
        .dout(shift_out_512[6])
    );
    barrel_shifter_512 u_shift_512_7(
        .din(next_state_data_tmp[7]),
        .shift_num(7'd6),
        .dout(shift_out_512[7])
    );
    barrel_shifter_512 u_shift_512_8(
        .din(next_state_data_tmp[8]),
        .shift_num(7'd36),
        .dout(shift_out_512[8])
    );
    barrel_shifter_512 u_shift_512_9(
        .din(next_state_data_tmp[9]),
        .shift_num(7'd63),
        .dout(shift_out_512[9])
    );
    barrel_shifter_512 u_shift_512_10(
        .din(next_state_data_tmp[10]),
        .shift_num(7'd62),
        .dout(shift_out_512[10])
    );
    barrel_shifter_512 u_shift_512_11(
        .din(next_state_data_tmp[11]),
        .shift_num(7'd33),
        .dout(shift_out_512[11])
    );
    barrel_shifter_512 u_shift_512_12(
        .din(next_state_data_tmp[12]),
        .shift_num(7'd1),
        .dout(shift_out_512[12])
    );
    barrel_shifter_512 u_shift_512_13(
        .din(next_state_data_tmp[13]),
        .shift_num(7'd30),
        .dout(shift_out_512[13])
    );
    barrel_shifter_512 u_shift_512_14(
        .din(next_state_data_tmp[14]),
        .shift_num(7'd31),
        .dout(shift_out_512[14])
    );
    barrel_shifter_512 u_shift_512_15(
        .din(next_state_data_tmp[15]),
        .shift_num(7'd4),
        .dout(shift_out_512[15])
    );
    barrel_shifter_512 u_shift_512_16(
        .din(next_state_data_tmp[16]),
        .shift_num(7'd38),
        .dout(shift_out_512[16])
    );
    barrel_shifter_512 u_shift_512_17(
        .din(next_state_data_tmp[17]),
        .shift_num(7'd5),
        .dout(shift_out_512[17])
    );
    barrel_shifter_512 u_shift_512_18(
        .din(next_state_data_tmp[18]),
        .shift_num(7'd8),
        .dout(shift_out_512[18])
    );
    barrel_shifter_512 u_shift_512_19(
        .din(next_state_data_tmp[19]),
        .shift_num(7'd47),
        .dout(shift_out_512[19])
    );
    barrel_shifter_512 u_shift_512_20(
        .din(next_state_data_tmp[20]),
        .shift_num(7'd19),
        .dout(shift_out_512[20])
    );
    barrel_shifter_512 u_shift_512_21(
        .din(next_state_data_tmp[21]),
        .shift_num(7'd52),
        .dout(shift_out_512[21])
    );
    barrel_shifter_512 u_shift_512_22(
        .din(next_state_data_tmp[22]),
        .shift_num(7'd57),
        .dout(shift_out_512[22])
    );
    barrel_shifter_512 u_shift_512_23(
        .din(next_state_data_tmp[23]),
        .shift_num(7'd34),
        .dout(shift_out_512[23])
    );

    barrel_shifter_768 u_shift_768_0(
        .din(next_state_data_tmp[0]),
        .shift_num(7'd2),
        .dout(shift_out_768[0])
    );
    barrel_shifter_768 u_shift_768_1(
        .din(next_state_data_tmp[1]),
        .shift_num(7'd11),
        .dout(shift_out_768[1])
    );
    barrel_shifter_768 u_shift_768_2(
        .din(next_state_data_tmp[2]),
        .shift_num(7'd28),
        .dout(shift_out_768[2])
    );
    barrel_shifter_768 u_shift_768_3(
        .din(next_state_data_tmp[3]),
        .shift_num(7'd53),
        .dout(shift_out_768[3])
    );
    barrel_shifter_768 u_shift_768_4(
        .din(next_state_data_tmp[4]),
        .shift_num(7'd19),
        .dout(shift_out_768[4])
    );
    barrel_shifter_768 u_shift_768_5(
        .din(next_state_data_tmp[5]),
        .shift_num(7'd34),
        .dout(shift_out_768[5])
    );
    barrel_shifter_768 u_shift_768_6(
        .din(next_state_data_tmp[6]),
        .shift_num(7'd57),
        .dout(shift_out_768[6])
    );
    barrel_shifter_768 u_shift_768_7(
        .din(next_state_data_tmp[7]),
        .shift_num(7'd88),
        .dout(shift_out_768[7])
    );
    barrel_shifter_768 u_shift_768_8(
        .din(next_state_data_tmp[8]),
        .shift_num(7'd52),
        .dout(shift_out_768[8])
    );
    barrel_shifter_768 u_shift_768_9(
        .din(next_state_data_tmp[9]),
        .shift_num(7'd73),
        .dout(shift_out_768[9])
    );
    barrel_shifter_768 u_shift_768_10(
        .din(next_state_data_tmp[10]),
        .shift_num(7'd6),
        .dout(shift_out_768[10])
    );
    barrel_shifter_768 u_shift_768_11(
        .din(next_state_data_tmp[11]),
        .shift_num(7'd43),
        .dout(shift_out_768[11])
    );
    barrel_shifter_768 u_shift_768_12(
        .din(next_state_data_tmp[12]),
        .shift_num(7'd5),
        .dout(shift_out_768[12])
    );
    barrel_shifter_768 u_shift_768_13(
        .din(next_state_data_tmp[13]),
        .shift_num(7'd32),
        .dout(shift_out_768[13])
    );
    barrel_shifter_768 u_shift_768_14(
        .din(next_state_data_tmp[14]),
        .shift_num(7'd67),
        .dout(shift_out_768[14])
    );
    barrel_shifter_768 u_shift_768_15(
        .din(next_state_data_tmp[15]),
        .shift_num(7'd14),
        .dout(shift_out_768[15])
    );
    barrel_shifter_768 u_shift_768_16(
        .din(next_state_data_tmp[16]),
        .shift_num(7'd70),
        .dout(shift_out_768[16])
    );
    barrel_shifter_768 u_shift_768_17(
        .din(next_state_data_tmp[17]),
        .shift_num(7'd7),
        .dout(shift_out_768[17])
    );
    barrel_shifter_768 u_shift_768_18(
        .din(next_state_data_tmp[18]),
        .shift_num(7'd48),
        .dout(shift_out_768[18])
    );
    barrel_shifter_768 u_shift_768_19(
        .din(next_state_data_tmp[19]),
        .shift_num(7'd1),
        .dout(shift_out_768[19])
    );
    barrel_shifter_768 u_shift_768_20(
        .din(next_state_data_tmp[20]),
        .shift_num(7'd55),
        .dout(shift_out_768[20])
    );
    barrel_shifter_768 u_shift_768_21(
        .din(next_state_data_tmp[21]),
        .shift_num(7'd94),
        .dout(shift_out_768[21])
    );
    barrel_shifter_768 u_shift_768_22(
        .din(next_state_data_tmp[22]),
        .shift_num(7'd45),
        .dout(shift_out_768[22])
    );
    barrel_shifter_768 u_shift_768_23(
        .din(next_state_data_tmp[23]),
        .shift_num(7'd4),
        .dout(shift_out_768[23])
    );

    barrel_shifter_1024 u_shift_1024_0(
        .din(next_state_data_tmp[0]),
        .shift_num(7'd1),
        .dout(shift_out_1024[0])
    );
    barrel_shifter_1024 u_shift_1024_1(
        .din(next_state_data_tmp[1]),
        .shift_num(7'd16),
        .dout(shift_out_1024[1])
    );
    barrel_shifter_1024 u_shift_1024_2(
        .din(next_state_data_tmp[2]),
        .shift_num(7'd55),
        .dout(shift_out_1024[2])
    );
    barrel_shifter_1024 u_shift_1024_3(
        .din(next_state_data_tmp[3]),
        .shift_num(7'd118),
        .dout(shift_out_1024[3])
    );
    barrel_shifter_1024 u_shift_1024_4(
        .din(next_state_data_tmp[4]),
        .shift_num(7'd10),
        .dout(shift_out_1024[4])
    );
    barrel_shifter_1024 u_shift_1024_5(
        .din(next_state_data_tmp[5]),
        .shift_num(7'd29),
        .dout(shift_out_1024[5])
    );
    barrel_shifter_1024 u_shift_1024_6(
        .din(next_state_data_tmp[6]),
        .shift_num(7'd72),
        .dout(shift_out_1024[6])
    );
    barrel_shifter_1024 u_shift_1024_7(
        .din(next_state_data_tmp[7]),
        .shift_num(7'd11),
        .dout(shift_out_1024[7])
    );
    barrel_shifter_1024 u_shift_1024_8(
        .din(next_state_data_tmp[8]),
        .shift_num(7'd35),
        .dout(shift_out_1024[8])
    );
    barrel_shifter_1024 u_shift_1024_9(
        .din(next_state_data_tmp[9]),
        .shift_num(7'd58),
        .dout(shift_out_1024[9])
    );
    barrel_shifter_1024 u_shift_1024_10(
        .din(next_state_data_tmp[10]),
        .shift_num(7'd105),
        .dout(shift_out_1024[10])
    );
    barrel_shifter_1024 u_shift_1024_11(
        .din(next_state_data_tmp[11]),
        .shift_num(7'd48),
        .dout(shift_out_1024[11])
    );
    barrel_shifter_1024 u_shift_1024_12(
        .din(next_state_data_tmp[12]),
        .shift_num(7'd76),
        .dout(shift_out_1024[12])
    );
    barrel_shifter_1024 u_shift_1024_13(
        .din(next_state_data_tmp[13]),
        .shift_num(7'd103),
        .dout(shift_out_1024[13])
    );
    barrel_shifter_1024 u_shift_1024_14(
        .din(next_state_data_tmp[14]),
        .shift_num(7'd26),
        .dout(shift_out_1024[14])
    );
    barrel_shifter_1024 u_shift_1024_15(
        .din(next_state_data_tmp[15]),
        .shift_num(7'd101),
        .dout(shift_out_1024[15])
    );
    barrel_shifter_1024 u_shift_1024_16(
        .din(next_state_data_tmp[16]),
        .shift_num(7'd5),
        .dout(shift_out_1024[16])
    );
    barrel_shifter_1024 u_shift_1024_17(
        .din(next_state_data_tmp[17]),
        .shift_num(7'd36),
        .dout(shift_out_1024[17])
    );
    barrel_shifter_1024 u_shift_1024_18(
        .din(next_state_data_tmp[18]),
        .shift_num(7'd91),
        .dout(shift_out_1024[18])
    );
    barrel_shifter_1024 u_shift_1024_19(
        .din(next_state_data_tmp[19]),
        .shift_num(7'd42),
        .dout(shift_out_1024[19])
    );
    barrel_shifter_1024 u_shift_1024_20(
        .din(next_state_data_tmp[20]),
        .shift_num(7'd78),
        .dout(shift_out_1024[20])
    );
    barrel_shifter_1024 u_shift_1024_21(
        .din(next_state_data_tmp[21]),
        .shift_num(7'd113),
        .dout(shift_out_1024[21])
    );
    barrel_shifter_1024 u_shift_1024_22(
        .din(next_state_data_tmp[22]),
        .shift_num(7'd44),
        .dout(shift_out_1024[22])
    );
    barrel_shifter_1024 u_shift_1024_23(
        .din(next_state_data_tmp[23]),
        .shift_num(7'd127),
        .dout(shift_out_1024[23])
    );

integer q; 
always @(*) begin
    for (q = 0; q < 24; q = q + 1) begin
        case (w_sel)
            2'b00: shift_out[q] = shift_out_256[q];
            2'b01: shift_out[q] = shift_out_512[q];
            2'b10: shift_out[q] = shift_out_768[q];
            2'b11: shift_out[q] = shift_out_1024[q];
            default: ;
        endcase
    end
end

always @(*) begin
    // 非线性层：4列独立运算
    // 列0: 0,4,8,12,16,20
    T[0]  = ~(state[0] ^ state[8] ^ (state[4] & state[12]));
    T[4]  = ~(state[4] ^ state[12] ^ (state[8] & state[16]));
    T[8]  = state[8] ^ state[16] ^ (state[12] & state[20]);
    T[12] = ~(state[12] ^ state[20] ^ (state[16] & state[0]));
    T[16] = ~(state[16] ^ state[0] ^ (state[20] & state[4]));
    T[20] = state[20] ^ state[4] ^ (state[0] & state[8]);

    // 列1:1,5,9,13,17,21
    T[1]  = ~(state[1] ^ state[9] ^ (state[5] & state[13]));
    T[5]  = ~(state[5] ^ state[13] ^ (state[9] & state[17]));
    T[9]  = state[9] ^ state[17] ^ (state[13] & state[21]);
    T[13] = ~(state[13] ^ state[21] ^ (state[17] & state[1]));
    T[17] = ~(state[17] ^ state[1] ^ (state[21] & state[5]));
    T[21] = state[21] ^ state[5] ^ (state[1] & state[9]);

    // 列2:2,6,10,14,18,22
    T[2]  = ~(state[2] ^ state[10] ^ (state[6] & state[14]));
    T[6]  = ~(state[6] ^ state[14] ^ (state[10] & state[18]));
    T[10] = state[10] ^ state[18] ^ (state[14] & state[22]);
    T[14] = ~(state[14] ^ state[22] ^ (state[18] & state[2]));
    T[18] = ~(state[18] ^ state[2] ^ (state[22] & state[6]));
    T[22] = state[22] ^ state[6] ^ (state[2] & state[10]);

    // 列3:3,7,11,15,19,23
    T[3]  = ~(state[3] ^ state[11] ^ (state[7] & state[15]));
    T[7]  = ~(state[7] ^ state[15] ^ (state[11] & state[19]));
    T[11] = state[11] ^ state[19] ^ (state[15] & state[23]);
    T[15] = ~(state[15] ^ state[23] ^ (state[19] & state[3]));
    T[19] = ~(state[19] ^ state[3] ^ (state[23] & state[7]));
    T[23] = state[23] ^ state[7] ^ (state[3] & state[11]);

    // 行族0
    T[24] = T[4] ^ T[9]; T[25] = T[18] ^ T[23];
    next_state_data_tmp[0] = T[25] ^ T[9]; next_state_data_tmp[1] = T[25] ^ T[4];
    next_state_data_tmp[2] = T[24] ^ T[23]; next_state_data_tmp[3] = T[24] ^ T[18];

    // 行族1
/*
    T[24] = T[8] ^ T[17]; T[25] = T[14] ^ T[3];
    next_state_data[4] = T[25] ^ T[17]; next_state_data[5] = T[25] ^ T[8];
    next_state_data[6] = T[24] ^ T[3];  next_state_data[7] = T[24] ^ T[14];
*/
    T[26] = T[8] ^ T[1]; T[27] = T[22] ^ T[15];
    next_state_data_tmp[4] = T[27] ^ T[1]; next_state_data_tmp[5] = T[27] ^ T[8];
    next_state_data_tmp[6] = T[26] ^ T[15];  next_state_data_tmp[7] = T[26] ^ T[22];

    // 行族2
/*
    T[24] = T[12] ^ T[5]; T[25] = T[22] ^ T[19];
    next_state_data[8] = T[25] ^ T[5]; next_state_data[9] = T[25] ^ T[12];
    next_state_data[10]= T[24] ^ T[19];next_state_data[11]= T[24] ^ T[22];
*/
    T[28] = T[12] ^ T[17]; T[29] = T[6] ^ T[3];
    next_state_data_tmp[8] = T[29] ^ T[17]; next_state_data_tmp[9] = T[29] ^ T[12];
    next_state_data_tmp[10]= T[28] ^ T[3];next_state_data_tmp[11]= T[28] ^ T[6];

    // 行族3
/*
    T[24] = T[20] ^ T[1]; T[25] = T[10] ^ T[7];
    next_state_data[12]= T[25] ^ T[1]; next_state_data[13]= T[25] ^ T[20];
    next_state_data[14]= T[24] ^ T[7]; next_state_data[15]= T[24] ^ T[10];
*/
    T[30] = T[20] ^ T[5]; T[31] = T[10] ^ T[19];
    next_state_data_tmp[12]= T[31] ^ T[5]; next_state_data_tmp[13]= T[31] ^ T[20];
    next_state_data_tmp[14]= T[30] ^ T[19]; next_state_data_tmp[15]= T[30] ^ T[10];

    // 行族4
/*
    T[24] = T[0] ^ T[21]; T[25] = T[6] ^ T[15];
    next_state_data[16]= T[25] ^ T[21];next_state_data[17]= T[25] ^ T[0];
    next_state_data[18]= T[24] ^ T[15];next_state_data[19]= T[24] ^ T[6];
*/
    T[32] = T[0] ^ T[21]; T[33] = T[14] ^ T[11];
    next_state_data_tmp[16]= T[33] ^ T[21];next_state_data_tmp[17]= T[33] ^ T[0];
    next_state_data_tmp[18]= T[32] ^ T[11];next_state_data_tmp[19]= T[32] ^ T[14];

    // 行族5
/*
    T[24] = T[16] ^ T[13]; T[25] = T[2] ^ T[11];
    next_state_data[20]= T[25] ^ T[13];next_state_data[21]= T[25] ^ T[16];
    next_state_data[22]= T[24] ^ T[11];next_state_data[23]= T[24] ^ T[2];
    next_state_data[23] =  shift_out[23] ^ rc[round_cnt];
*/
    T[34] = T[16] ^ T[13]; T[35] = T[2] ^ T[7];
    next_state_data_tmp[20]= T[35] ^ T[13];next_state_data_tmp[21]= T[35] ^ T[16];
    next_state_data_tmp[22]= T[34] ^ T[7];next_state_data_tmp[23]= T[34] ^ T[2];

   // next_state_data[23] =  shift_out[23] ^ rc[round_cnt];

end
    // RC常数
    // W=32 RC[16]
 assign  rc_256[0]  = 128'h000000000000000000000000B7E15162;
 assign  rc_256[1]  = 128'h0000000000000000000000008AED2A6A;
 assign  rc_256[2]  = 128'h000000000000000000000000BF715880;
 assign  rc_256[3]  = 128'h0000000000000000000000009CF4F3C7;
 assign  rc_256[4]  = 128'h00000000000000000000000062E7160F;
 assign  rc_256[5]  = 128'h00000000000000000000000038B4DA56;
 assign  rc_256[6]  = 128'h000000000000000000000000A784D904;
 assign  rc_256[7]  = 128'h0000000000000000000000005190CFEF;
 assign  rc_256[8]  = 128'h000000000000000000000000324E7738;
 assign  rc_256[9]  = 128'h000000000000000000000000926CFBE5;
 assign  rc_256[10] = 128'h000000000000000000000000F4BF8D8D;
 assign  rc_256[11] = 128'h0000000000000000000000008C31D763;
 assign  rc_256[12] = 128'h000000000000000000000000DA06C80A;
 assign  rc_256[13] = 128'h000000000000000000000000BB1185EB;
 assign  rc_256[14] = 128'h0000000000000000000000004F7C7B57;
 assign  rc_256[15] = 128'h00000000000000000000000057F59584;
 assign  rc_256[16] = 128'h0;
 assign  rc_256[17] = 128'h0;
 assign  rc_256[18] = 128'h0;
 assign  rc_256[19] = 128'h0;
 assign  rc_256[20] = 128'h0;
 assign  rc_256[21] = 128'h0;
 assign  rc_256[22] = 128'h0;
 assign  rc_256[23] = 128'h0;
 assign  rc_256[24] = 128'h0;
 assign  rc_256[25] = 128'h0;
 assign  rc_256[26] = 128'h0;
 assign  rc_256[27] = 128'h0;
 assign  rc_256[28] = 128'h0;
 assign  rc_256[29] = 128'h0;
 assign  rc_256[30] = 128'h0;
 assign  rc_256[31] = 128'h0;
 assign  rc_256[32] = 128'h0;
 assign  rc_256[33] = 128'h0;
 assign  rc_256[34] = 128'h0;
 assign  rc_256[35] = 128'h0;
 assign  rc_256[36] = 128'h0;
 assign  rc_256[37] = 128'h0;
 assign  rc_256[38] = 128'h0;
 assign  rc_256[39] = 128'h0;

    // ==========================================
    // W=64 RC 常数 [24个] 64bit
    // ==========================================
 assign  rc_512[0]  = 128'h0000000000000000B7E151628AED2A6A;
 assign  rc_512[1]  = 128'h0000000000000000BF7158809CF4F3C7;
 assign  rc_512[2]  = 128'h000000000000000062E7160F38B4DA56;
 assign  rc_512[3]  = 128'h0000000000000000A784D9045190CFEF;
 assign  rc_512[4]  = 128'h0000000000000000324E7738926CFBE5;
 assign  rc_512[5]  = 128'h0000000000000000F4BF8D8D8C31D763;
 assign  rc_512[6]  = 128'h0000000000000000DA06C80ABB1185EB;
 assign  rc_512[7]  = 128'h00000000000000004F7C7B5757F59584;
 assign  rc_512[8]  = 128'h000000000000000090CFD47D7C19BB42;
 assign  rc_512[9]  = 128'h0000000000000000158D9554F7B46BCE;
 assign  rc_512[10] = 128'h0000000000000000D55C4D79FD5F24D6;
 assign  rc_512[11] = 128'h0000000000000000613C31C3839A2DDF;
 assign  rc_512[12] = 128'h00000000000000008A9A276BCFBFA1C8;
 assign  rc_512[13] = 128'h000000000000000077C56284DAB79CD4;
 assign  rc_512[14] = 128'h0000000000000000C2B3293D20E9E5EA;
 assign  rc_512[15] = 128'h0000000000000000F02AC60ACC93ED87;
 assign  rc_512[16] = 128'h00000000000000004422A52ECB238FEE;
 assign  rc_512[17] = 128'h0000000000000000E5AB6ADD835FD1A0;
 assign  rc_512[18] = 128'h0000000000000000753D0A8F78E537D2;
 assign  rc_512[19] = 128'h0000000000000000B95BB79D8DCAEC64;
 assign  rc_512[20] = 128'h00000000000000002C1E9F23B829B5C2;
 assign  rc_512[21] = 128'h0000000000000000780BF38737DF8BB3;
 assign  rc_512[22] = 128'h000000000000000000D01334A0D0BD86;
 assign  rc_512[23] = 128'h000000000000000045CBFA73A6160FFE;
 assign  rc_512[24] = 128'h0;
 assign  rc_512[25] = 128'h0;
 assign  rc_512[26] = 128'h0;
 assign  rc_512[27] = 128'h0;
 assign  rc_512[28] = 128'h0;
 assign  rc_512[29] = 128'h0;
 assign  rc_512[30] = 128'h0;
 assign  rc_512[31] = 128'h0;
 assign  rc_512[32] = 128'h0;
 assign  rc_512[33] = 128'h0;
 assign  rc_512[34] = 128'h0;
 assign  rc_512[35] = 128'h0;
 assign  rc_512[36] = 128'h0;
 assign  rc_512[37] = 128'h0;
 assign  rc_512[38] = 128'h0;
 assign  rc_512[39] = 128'h0;

    // ==========================================
    // W=96 RC 常数 [32个] 96bit = {32bit, 64bit}
    // ==========================================
 assign  rc_768[0]  = 128'h00000000B7E151628AED2A6ABF715880;
 assign  rc_768[1]  = 128'h000000009CF4F3C762E7160F38B4DA56;
 assign  rc_768[2]  = 128'h00000000A784D9045190CFEF324E7738;
 assign  rc_768[3]  = 128'h00000000926CFBE5F4BF8D8D8C31D763;
 assign  rc_768[4]  = 128'h00000000DA06C80ABB1185EB4F7C7B57;
 assign  rc_768[5]  = 128'h0000000057F5958490CFD47D7C19BB42;
 assign  rc_768[6]  = 128'h00000000158D9554F7B46BCED55C4D79;
 assign  rc_768[7]  = 128'h00000000FD5F24D6613C31C3839A2DDF;
 assign  rc_768[8]  = 128'h000000008A9A276BCFBFA1C877C56284;
 assign  rc_768[9]  = 128'h00000000DAB79CD4C2B3293D20E9E5EA;
 assign  rc_768[10] = 128'h00000000F02AC60ACC93ED874422A52E;
 assign  rc_768[11] = 128'h00000000CB238FEEE5AB6ADD835FD1A0;
 assign  rc_768[12] = 128'h00000000753D0A8F78E537D2B95BB79D;
 assign  rc_768[13] = 128'h000000008DCAEC642C1E9F23B829B5C2;
 assign  rc_768[14] = 128'h00000000780BF38737DF8BB300D01334;
 assign  rc_768[15] = 128'h00000000A0D0BD8645CBFA73A6160FFE;
 assign  rc_768[16] = 128'h00000000393C48CBBBCA060F0FF8EC6D;
 assign  rc_768[17] = 128'h0000000031BEB5CCEED7F2F0BB088017;
 assign  rc_768[18] = 128'h00000000163BC60DF45A0ECB1BCD289B;
 assign  rc_768[19] = 128'h0000000006CBBFEA21AD08E1847F3F73;
 assign  rc_768[20] = 128'h0000000078D56CED94640D6EF0D3D37B;
 assign  rc_768[21] = 128'h00000000E67008E186D1BF275B9B241D;
 assign  rc_768[22] = 128'h00000000EB64749A47DFDFB96632C3EB;
 assign  rc_768[23] = 128'h00000000061B6472BBF84C26144E49C2;
 assign  rc_768[24] = 128'h00000000D04C324EF10DE513D3F5114B;
 assign  rc_768[25] = 128'h000000008B5D374D93CB8879C7D52FFD;
 assign  rc_768[26] = 128'h0000000072BA0AAE7277DA7BA1B4AF14;
 assign  rc_768[27] = 128'h0000000088D8E836AF14865E6C37AB68;
 assign  rc_768[28] = 128'h0000000076FE690B571121382AF341AF;
 assign  rc_768[29] = 128'h00000000E94F77BCF06C83B8FF5675F0;
 assign  rc_768[30] = 128'h00000000979074AD9A787BC5B9BD4B0C;
 assign  rc_768[31] = 128'h000000005937D3EDE4C3A79396215EDA;
 assign  rc_768[32] = 128'h0;
 assign  rc_768[33] = 128'h0;
 assign  rc_768[34] = 128'h0;
 assign  rc_768[35] = 128'h0;
 assign  rc_768[36] = 128'h0;
 assign  rc_768[37] = 128'h0;
 assign  rc_768[38] = 128'h0;
 assign  rc_768[39] = 128'h0;

    // ==========================================
    // W=128 RC 常数 [40个] 完整128bit
    // ==========================================
 assign  rc_1024[0]  = 128'hB7E151628AED2A6ABF7158809CF4F3C7;
 assign  rc_1024[1]  = 128'h62E7160F38B4DA56A784D9045190CFEF;
 assign  rc_1024[2]  = 128'h324E7738926CFBE5F4BF8D8D8C31D763;
 assign  rc_1024[3]  = 128'hDA06C80ABB1185EB4F7C7B5757F59584;
 assign  rc_1024[4]  = 128'h90CFD47D7C19BB42158D9554F7B46BCE;
 assign  rc_1024[5]  = 128'hD55C4D79FD5F24D6613C31C3839A2DDF;
 assign  rc_1024[6]  = 128'h8A9A276BCFBFA1C877C56284DAB79CD4;
 assign  rc_1024[7]  = 128'hC2B3293D20E9E5EAF02AC60ACC93ED87;
 assign  rc_1024[8]  = 128'h4422A52ECB238FEEE5AB6ADD835FD1A0;
 assign  rc_1024[9]  = 128'h753D0A8F78E537D2B95BB79D8DCAEC64;
 assign  rc_1024[10] = 128'h2C1E9F23B829B5C2780BF38737DF8BB3;
 assign  rc_1024[11] = 128'h00D01334A0D0BD8645CBFA73A6160FFE;
 assign  rc_1024[12] = 128'h393C48CBBBCA060F0FF8EC6D31BEB5CC;
 assign  rc_1024[13] = 128'hEED7F2F0BB088017163BC60DF45A0ECB;
 assign  rc_1024[14] = 128'h1BCD289B06CBBFEA21AD08E1847F3F73;
 assign  rc_1024[15] = 128'h78D56CED94640D6EF0D3D37BE67008E1;
 assign  rc_1024[16] = 128'h86D1BF275B9B241DEB64749A47DFDFB9;
 assign  rc_1024[17] = 128'h6632C3EB061B6472BBF84C26144E49C2;
 assign  rc_1024[18] = 128'hD04C324EF10DE513D3F5114B8B5D374D;
 assign  rc_1024[19] = 128'h93CB8879C7D52FFD72BA0AAE7277DA7B;
 assign  rc_1024[20] = 128'hA1B4AF1488D8E836AF14865E6C37AB68;
 assign  rc_1024[21] = 128'h76FE690B571121382AF341AFE94F77BC;
 assign  rc_1024[22] = 128'hF06C83B8FF5675F0979074AD9A787BC5;
 assign  rc_1024[23] = 128'hB9BD4B0C5937D3EDE4C3A79396215EDA;
 assign  rc_1024[24] = 128'hB1F57D0B5A7DB461DD8F3C75540D0012;
 assign  rc_1024[25] = 128'h1FD56E95F8C731E9C4D7221BBED0C62B;
 assign  rc_1024[26] = 128'hB5A87804B679A0CAA41D802A4604C311;
 assign  rc_1024[27] = 128'hB71DE3E5C6B400E024A6668CCF2E2DE8;
 assign  rc_1024[28] = 128'h6876E4F5C50000F0A93B3AA7E6342B30;
 assign  rc_1024[29] = 128'h2A0A47373B25F73E3B26D569FE2291AD;
 assign  rc_1024[30] = 128'h36D6A147D1060B871A2801F978376408;
 assign  rc_1024[31] = 128'h2FF592D9140DB1E9399DF4B0E14CA8E8;
 assign  rc_1024[32] = 128'h8EE9110B2BD4FA98EED150CA6DD89322;
 assign  rc_1024[33] = 128'h45EF7592C703F532CE3A30CD31C070EB;
 assign  rc_1024[34] = 128'h36B4195FF33FB1C66C7D70F93918107C;
 assign  rc_1024[35] = 128'hE2051FED33F6D1DE9491C7DEA6A5A442;
 assign  rc_1024[36] = 128'hE154C8BB6D8D0362803BC248D414478C;
 assign  rc_1024[37] = 128'h2AFB07FFE78E89B9FECA7E3060C08F0D;
 assign  rc_1024[38] = 128'h61F8E36801DF66D1D8F9392E52CAEF06;
 assign  rc_1024[39] = 128'h53199479DF2BE64BBAAB008CA8A06FDA;

integer p; 
always @(*) begin
    for (p = 0; p < 40; p = p + 1) begin
        case (w_sel)
            2'b00: rc[p] = rc_256[p];
            2'b01: rc[p] = rc_512[p];
            2'b10: rc[p] = rc_768[p];
            2'b11: rc[p] = rc_1024[p];
            default: ;
        endcase
    end
end

assign next_state_last_w = shift_out[23] ^ rc[round_cnt];

integer j,k,n,m,u,v;
always @(posedge clk ) begin
    if(!rst_n) begin
        curr_state <= IDLE;
    end
    else begin
        curr_state <= next_state;
    end
end
always @(posedge clk ) begin
    if(!rst_n) begin
        round_cnt     <= 6'd0;
        last_msg_flag <= 1'b0;
        for(j=0; j<ROWS; j=j+1)begin
            state[j] <= {W_MAX{1'b0}};
        end
        for(m=0; m<ROWS; m=m+1)begin
            update_state[m] <= {W_MAX{1'b0}};
        end
    end else begin
        case(curr_state)
            IDLE: begin
                if(w_sel == 2'b00)begin
                    state[23] <= msg_len[63:32];
                    state[22] <= msg_len[31:0];
                    update_state[23] <= msg_len[63:32];
                    update_state[22] <= msg_len[31:0];
                end
                else begin
                    state[23] <= msg_len;
                    update_state[23] <= msg_len;
                end
            end
            PERMUTE: begin
                if(round_cnt == 6'd0)begin
                    for(u=0; u<ROWS; u=u+1)begin
                        update_state[u] <= state[u];
                    end
                end
                if(round_cnt == nr-1)begin
                    //s to the next state, ^Mi
                    for(k=0; k<ROWS-1; k=k+1)begin
                        state[k] <= shift_out[k] ^ mi_s[k];
                    end
                    state[23] <= next_state_last_w ^ mi_s[23];
                end
                else begin
                    //s to the next state, in round
                    for(k=0; k<ROWS-1; k=k+1)begin
                        state[k] <= shift_out[k];
                    end
                    state[23] <= shift_out[23] ^ rc[round_cnt];
                end
                // 轮数迭代
                if(round_cnt < nr-1) begin
                    round_cnt <= round_cnt + 1'b1;
                end
                else begin
                    round_cnt <= 6'd0;
                end
                if(padding_flag == 1'b1 && round_cnt == nr-2)begin
                    last_msg_flag <= 1'b1;
                end
            end
            SQUEEZ:begin
                if(round_cnt == 6'd0)begin
                    for(v=0; v<ROWS; v=v+1)begin
                        update_state[v] <= state[v];
                    end
                end
                for(n=0; n<ROWS-1; n=n+1)begin
                    state[n] <= shift_out[n];
                end
                state[23] <= shift_out[23] ^ rc[round_cnt];
                // 轮数迭代
                if(round_cnt < nr-1) begin
                    round_cnt <= round_cnt + 1'b1;
                end
                else begin
                    round_cnt <= 6'd0;
                end
            end
        endcase
    end
end

always @(*) begin
    case(curr_state)
        IDLE: begin
            if(start) begin
                next_state = PERMUTE;
            end
            else begin
                next_state = IDLE;
            end
        end
        PERMUTE: begin
            if(last_msg_flag) begin
                next_state = SQUEEZ;
            end
            else begin
                next_state = PERMUTE;
            end
        end
        SQUEEZ: begin
            if(round_cnt == nr-1)begin
                next_state = DONE;
            end
            else begin
                next_state = SQUEEZ;
            end
        end
        DONE: begin 
            next_state = DONE;
        end
        default : next_state = IDLE;
    endcase
end

// 输出：Rate部分作为哈希结果
assign data_out = w_sel == `W_32 ? {state[0][7  :0], state[0][15 :8], state[0][23:16], state[0][31:24],
                                    state[1][7  :0], state[1][15 :8], state[1][23:16], state[1][31:24],
                                    state[2][7  :0], state[2][15 :8], state[2][23:16], state[2][31:24],
                                    state[3][7  :0], state[3][15 :8], state[3][23:16], state[3][31:24],
                                    state[4][7  :0], state[4][15 :8], state[4][23:16], state[4][31:24],
                                    state[5][7  :0], state[5][15 :8], state[5][23:16], state[5][31:24],
                                    state[6][7  :0], state[6][15 :8], state[6][23:16], state[6][31:24],
                                    state[7][7  :0], state[7][15 :8], state[7][23:16], state[7][31:24]} :

                  w_sel == `W_64 ? {state[0][7  :0], state[0][15 :8], state[0][23:16], state[0][31:24],
                                    state[0][39:32], state[0][47:40], state[0][55:48], state[0][63:56],

                                    state[1][7  :0], state[1][15 :8], state[1][23:16], state[1][31:24],
                                    state[1][39:32], state[1][47:40], state[1][55:48], state[1][63:56],

                                    state[2][7  :0], state[2][15 :8], state[2][23:16], state[2][31:24],
                                    state[2][39:32], state[2][47:40], state[2][55:48], state[2][63:56],

                                    state[3][7  :0], state[3][15 :8], state[3][23:16], state[3][31:24],
                                    state[3][39:32], state[3][47:40], state[3][55:48], state[3][63:56],

                                    state[4][7  :0], state[4][15 :8], state[4][23:16], state[4][31:24],
                                    state[4][39:32], state[4][47:40], state[4][55:48], state[4][63:56],

                                    state[5][7  :0], state[5][15 :8], state[5][23:16], state[5][31:24],
                                    state[5][39:32], state[5][47:40], state[5][55:48], state[5][63:56],

                                    state[6][7  :0], state[6][15 :8], state[6][23:16], state[6][31:24],
                                    state[6][39:32], state[6][47:40], state[6][55:48], state[6][63:56],

                                    state[7][7  :0], state[7][15 :8], state[7][23:16], state[7][31:24],
                                    state[7][39:32], state[7][47:40], state[7][55:48], state[7][63:56]} :

                  w_sel == `W_96 ? {state[0][7  :0], state[0][15 :8], state[0][23:16], state[0][31:24],
                                    state[0][39:32], state[0][47:40], state[0][55:48], state[0][63:56],
                                    state[0][71:64], state[0][79:72], state[0][87:80], state[0][95:88],

                                    state[1][7  :0], state[1][15 :8], state[1][23:16], state[1][31:24],
                                    state[1][39:32], state[1][47:40], state[1][55:48], state[1][63:56],
                                    state[1][71:64], state[1][79:72], state[1][87:80], state[1][95:88],

                                    state[2][7  :0], state[2][15 :8], state[2][23:16], state[2][31:24],
                                    state[2][39:32], state[2][47:40], state[2][55:48], state[2][63:56],
                                    state[2][71:64], state[2][79:72], state[2][87:80], state[2][95:88],

                                    state[3][7  :0], state[3][15 :8], state[3][23:16], state[3][31:24],
                                    state[3][39:32], state[3][47:40], state[3][55:48], state[3][63:56],
                                    state[3][71:64], state[3][79:72], state[3][87:80], state[3][95:88],

                                    state[4][7  :0], state[4][15 :8], state[4][23:16], state[4][31:24],
                                    state[4][39:32], state[4][47:40], state[4][55:48], state[4][63:56],
                                    state[4][71:64], state[4][79:72], state[4][87:80], state[4][95:88],

                                    state[5][7  :0], state[5][15 :8], state[5][23:16], state[5][31:24],
                                    state[5][39:32], state[5][47:40], state[5][55:48], state[5][63:56],
                                    state[5][71:64], state[5][79:72], state[5][87:80], state[5][95:88],

                                    state[6][7  :0], state[6][15 :8], state[6][23:16], state[6][31:24],
                                    state[6][39:32], state[6][47:40], state[6][55:48], state[6][63:56],
                                    state[6][71:64], state[6][79:72], state[6][87:80], state[6][95:88],

                                    state[7][7  :0], state[7][15 :8], state[7][23:16], state[7][31:24],
                                    state[7][39:32], state[7][47:40], state[7][55:48], state[7][63:56],
                                    state[7][71:64], state[7][79:72], state[7][87:80], state[7][95:88]} :
                                    
                                   {state[0][7  :0], state[0][15 :8], state[0][23:16], state[0][31:24],
                                    state[0][39:32], state[0][47:40], state[0][55:48], state[0][63:56],
                                    state[0][71:64], state[0][79:72], state[0][87:80], state[0][95:88],
                                    state[0][103:96], state[0][111:104], state[0][119:112], state[0][127:120],

                                    state[1][7  :0], state[1][15 :8], state[1][23:16], state[1][31:24],
                                    state[1][39:32], state[1][47:40], state[1][55:48], state[1][63:56],
                                    state[1][71:64], state[1][79:72], state[1][87:80], state[1][95:88],
                                    state[1][103:96], state[1][111:104], state[1][119:112], state[1][127:120],

                                    state[2][7  :0], state[2][15 :8], state[2][23:16], state[2][31:24],
                                    state[2][39:32], state[2][47:40], state[2][55:48], state[2][63:56],
                                    state[2][71:64], state[2][79:72], state[2][87:80], state[2][95:88],
                                    state[2][103:96], state[2][111:104], state[2][119:112], state[2][127:120],

                                    state[3][7  :0], state[3][15 :8], state[3][23:16], state[3][31:24],
                                    state[3][39:32], state[3][47:40], state[3][55:48], state[3][63:56],
                                    state[3][71:64], state[3][79:72], state[3][87:80], state[3][95:88],
                                    state[3][103:96], state[3][111:104], state[3][119:112], state[3][127:120],

                                    state[4][7  :0], state[4][15 :8], state[4][23:16], state[4][31:24],
                                    state[4][39:32], state[4][47:40], state[4][55:48], state[4][63:56],
                                    state[4][71:64], state[4][79:72], state[4][87:80], state[4][95:88],
                                    state[4][103:96], state[4][111:104], state[4][119:112], state[4][127:120],

                                    state[5][7  :0], state[5][15 :8], state[5][23:16], state[5][31:24],
                                    state[5][39:32], state[5][47:40], state[5][55:48], state[5][63:56],
                                    state[5][71:64], state[5][79:72], state[5][87:80], state[5][95:88],
                                    state[5][103:96], state[5][111:104], state[5][119:112], state[5][127:120],

                                    state[6][7  :0], state[6][15 :8], state[6][23:16], state[6][31:24],
                                    state[6][39:32], state[6][47:40], state[6][55:48], state[6][63:56],
                                    state[6][71:64], state[6][79:72], state[6][87:80], state[6][95:88],
                                    state[6][103:96], state[6][111:104], state[6][119:112], state[6][127:120],

                                    state[7][7  :0], state[7][15 :8], state[7][23:16], state[7][31:24],
                                    state[7][39:32], state[7][47:40], state[7][55:48], state[7][63:56],
                                    state[7][71:64], state[7][79:72], state[7][87:80], state[7][95:88],
                                    state[7][103:96], state[7][111:104], state[7][119:112], state[7][127:120]} ;

endmodule

