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
wire [W_MAX-1:0]    rc [0:MAX_NR-1];
wire [W_MAX-1:0]    shift_out [0:ROWS-1];

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

const_rom u_rom(
    .w_sel(w_sel),
    .sigma_out0(sigma[0]), .sigma_out1(sigma[1]), .sigma_out2(sigma[2]),
    .sigma_out3(sigma[3]), .sigma_out4(sigma[4]), .sigma_out5(sigma[5]),
    .sigma_out6(sigma[6]), .sigma_out7(sigma[7]), .sigma_out8(sigma[8]),
    .sigma_out9(sigma[9]), .sigma_out10(sigma[10]), .sigma_out11(sigma[11]),
    .sigma_out12(sigma[12]), .sigma_out13(sigma[13]), .sigma_out14(sigma[14]),
    .sigma_out15(sigma[15]), .sigma_out16(sigma[16]), .sigma_out17(sigma[17]),
    .sigma_out18(sigma[18]), .sigma_out19(sigma[19]), .sigma_out20(sigma[20]),
    .sigma_out21(sigma[21]), .sigma_out22(sigma[22]), .sigma_out23(sigma[23]),
    .rc_out0(rc[0]), .rc_out1(rc[1]), .rc_out2(rc[2]), .rc_out3(rc[3]),
    .rc_out4(rc[4]), .rc_out5(rc[5]), .rc_out6(rc[6]), .rc_out7(rc[7]),
    .rc_out8(rc[8]), .rc_out9(rc[9]), .rc_out10(rc[10]), .rc_out11(rc[11]),
    .rc_out12(rc[12]), .rc_out13(rc[13]), .rc_out14(rc[14]), .rc_out15(rc[15]),
    .rc_out16(rc[16]), .rc_out17(rc[17]), .rc_out18(rc[18]), .rc_out19(rc[19]),
    .rc_out20(rc[20]), .rc_out21(rc[21]), .rc_out22(rc[22]), .rc_out23(rc[23]),
    .rc_out24(rc[24]), .rc_out25(rc[25]), .rc_out26(rc[26]), .rc_out27(rc[27]),
    .rc_out28(rc[28]), .rc_out29(rc[29]), .rc_out30(rc[30]), .rc_out31(rc[31]),
    .rc_out32(rc[32]), .rc_out33(rc[33]), .rc_out34(rc[34]), .rc_out35(rc[35]),
    .rc_out36(rc[36]), .rc_out37(rc[37]), .rc_out38(rc[38]), .rc_out39(rc[39])
);

genvar i;
generate for(i=0; i<ROWS; i=i+1) begin : shift_loop
    barrel_shifter u_shift(
        .w_sel(w_sel),
        .din(next_state_data_tmp[i]),
        .shift_num(sigma[i]),
        .dout(shift_out[i])
    );
end endgenerate

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

