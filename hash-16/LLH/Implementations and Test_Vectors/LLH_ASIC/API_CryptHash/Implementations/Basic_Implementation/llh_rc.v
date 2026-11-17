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
// 只读存储器：存储SIGMA移位表、RC轮常数，按W_SEL寻址
module const_rom #(
    parameter SIGMA_W  = 7,    // 移位位宽(0~127)
    parameter RC_W    = `MAX_W,
    parameter SIGMA_D = 24,
    parameter MAX_RC  = `MAX_NR
) (
    input       [1:0]           w_sel,
    output reg  [SIGMA_W-1:0]   sigma_out0,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out1,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out2,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out3,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out4,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out5,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out6,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out7,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out8,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out9,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out10,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out11,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out12,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out13,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out14,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out15,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out16,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out17,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out18,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out19,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out20,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out21,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out22,   // SIGMA移位值
    output reg  [SIGMA_W-1:0]   sigma_out23,   // SIGMA移位值
    output reg  [RC_W-1:0]      rc_out0,       // RC轮常数
    output reg  [RC_W-1:0]      rc_out1,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out2,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out3,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out4,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out5,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out6,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out7,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out8,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out9,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out10,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out11,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out12,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out13,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out14,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out15,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out16,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out17,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out18,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out19,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out20,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out21,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out22,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out23,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out24,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out25,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out26,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out27,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out28,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out29,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out30,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out31,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out32,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out33,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out34,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out35,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out36,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out37,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out38,      // RC轮常数
    output reg  [RC_W-1:0]      rc_out39       // RC轮常数
);

wire [SIGMA_W-1:0] sigma_32 [0:SIGMA_D-1];
wire [SIGMA_W-1:0] sigma_64 [0:SIGMA_D-1];
wire [SIGMA_W-1:0] sigma_96 [0:SIGMA_D-1];
wire [SIGMA_W-1:0] sigma_128[0:SIGMA_D-1];

wire [RC_W-1:0] rc_32 [0:15];
wire [RC_W-1:0] rc_64 [0:23];
wire [RC_W-1:0] rc_96 [0:31];
wire [RC_W-1:0] rc_128[0:39];

    // W=32 SIGMA
 assign   sigma_32[0] = 7'd3;
 assign   sigma_32[1] = 7'd16;
 assign   sigma_32[2] = 7'd13;
 assign   sigma_32[3] = 7'd26;
 assign   sigma_32[4] = 7'd8;
 assign   sigma_32[5] = 7'd29;
 assign   sigma_32[6] = 7'd2;
 assign   sigma_32[7] = 7'd23;
 assign   sigma_32[8] = 7'd21;
 assign   sigma_32[9] = 7'd18;
 assign   sigma_32[10] = 7'd31;
 assign   sigma_32[11] = 7'd28;
 assign   sigma_32[12] = 7'd10;
 assign   sigma_32[13] = 7'd15;
 assign   sigma_32[14] = 7'd4;
 assign   sigma_32[15] = 7'd9;
 assign   sigma_32[16] = 7'd7;
 assign   sigma_32[17] = 7'd20;
 assign   sigma_32[18] = 7'd17;
 assign   sigma_32[19] = 7'd30;
 assign   sigma_32[20] = 7'd12;
 assign   sigma_32[21] = 7'd1;
 assign   sigma_32[22] = 7'd6;
 assign   sigma_32[23] = 7'd27;
    // W=64 SIGMA
 assign  sigma_64[0] = 7'd2;
 assign  sigma_64[1] = 7'd25;
 assign  sigma_64[2] = 7'd20;
 assign  sigma_64[3] = 7'd51;
 assign  sigma_64[4] = 7'd15;
 assign  sigma_64[5] = 7'd40;
 assign  sigma_64[6] = 7'd37;
 assign  sigma_64[7] = 7'd6;
 assign  sigma_64[8] = 7'd36;
 assign  sigma_64[9] = 7'd63;
 assign  sigma_64[10] = 7'd62;
 assign  sigma_64[11] = 7'd33;
 assign  sigma_64[12] = 7'd1;
 assign  sigma_64[13] = 7'd30;
 assign  sigma_64[14] = 7'd31;
 assign  sigma_64[15] = 7'd4;
 assign  sigma_64[16] = 7'd38;
 assign  sigma_64[17] = 7'd5;
 assign  sigma_64[18] = 7'd8;
 assign  sigma_64[19] = 7'd47;
 assign  sigma_64[20] = 7'd19;
 assign  sigma_64[21] = 7'd52;
 assign  sigma_64[22] = 7'd57;
 assign  sigma_64[23] = 7'd34;
    // W=96 SIGMA
 assign  sigma_96[0] = 7'd2;
 assign  sigma_96[1] = 7'd11;
 assign  sigma_96[2] = 7'd28;
 assign  sigma_96[3] = 7'd53;
 assign  sigma_96[4] = 7'd19;
 assign  sigma_96[5] = 7'd34;
 assign  sigma_96[6] = 7'd57;
 assign  sigma_96[7] = 7'd88;
 assign  sigma_96[8] = 7'd52;
 assign  sigma_96[9] = 7'd73;
 assign  sigma_96[10] = 7'd6;
 assign  sigma_96[11] = 7'd43;
 assign  sigma_96[12] = 7'd5;
 assign  sigma_96[13] = 7'd32;
 assign  sigma_96[14] = 7'd67;
 assign  sigma_96[15] = 7'd14;
 assign  sigma_96[16] = 7'd70;
 assign  sigma_96[17] = 7'd7;
 assign  sigma_96[18] = 7'd48;
 assign  sigma_96[19] = 7'd1;
 assign  sigma_96[20] = 7'd55;
 assign  sigma_96[21] = 7'd94;
 assign  sigma_96[22] = 7'd45;
 assign  sigma_96[23] = 7'd4;
    // W=128 SIGMA
 assign  sigma_128[0] = 7'd1;
 assign  sigma_128[1] = 7'd16;
 assign  sigma_128[2] = 7'd55;
 assign  sigma_128[3] = 7'd118;
 assign  sigma_128[4] = 7'd10;
 assign  sigma_128[5] = 7'd29;
 assign  sigma_128[6] = 7'd72;
 assign  sigma_128[7] = 7'd11;
 assign  sigma_128[8] = 7'd35;
 assign  sigma_128[9] = 7'd58;
 assign  sigma_128[10] = 7'd105;
 assign  sigma_128[11] = 7'd48;
 assign  sigma_128[12] = 7'd76;
 assign  sigma_128[13] = 7'd103;
 assign  sigma_128[14] = 7'd26;
 assign  sigma_128[15] = 7'd101;
 assign  sigma_128[16] = 7'd5;
 assign  sigma_128[17] = 7'd36;
 assign  sigma_128[18] = 7'd91;
 assign  sigma_128[19] = 7'd42;
 assign  sigma_128[20] = 7'd78;
 assign  sigma_128[21] = 7'd113;
 assign  sigma_128[22] = 7'd44;
 assign  sigma_128[23] = 7'd127;

    // RC常数
    // W=32 RC[16]
 assign  rc_32[0]  = 128'h000000000000000000000000B7E15162;
 assign  rc_32[1]  = 128'h0000000000000000000000008AED2A6A;
 assign  rc_32[2]  = 128'h000000000000000000000000BF715880;
 assign  rc_32[3]  = 128'h0000000000000000000000009CF4F3C7;
 assign  rc_32[4]  = 128'h00000000000000000000000062E7160F;
 assign  rc_32[5]  = 128'h00000000000000000000000038B4DA56;
 assign  rc_32[6]  = 128'h000000000000000000000000A784D904;
 assign  rc_32[7]  = 128'h0000000000000000000000005190CFEF;
 assign  rc_32[8]  = 128'h000000000000000000000000324E7738;
 assign  rc_32[9]  = 128'h000000000000000000000000926CFBE5;
 assign  rc_32[10] = 128'h000000000000000000000000F4BF8D8D;
 assign  rc_32[11] = 128'h0000000000000000000000008C31D763;
 assign  rc_32[12] = 128'h000000000000000000000000DA06C80A;
 assign  rc_32[13] = 128'h000000000000000000000000BB1185EB;
 assign  rc_32[14] = 128'h0000000000000000000000004F7C7B57;
 assign  rc_32[15] = 128'h00000000000000000000000057F59584;

    // ==========================================
    // W=64 RC 常数 [24个] 64bit
    // ==========================================
 assign  rc_64[0]  = 128'h0000000000000000B7E151628AED2A6A;
 assign  rc_64[1]  = 128'h0000000000000000BF7158809CF4F3C7;
 assign  rc_64[2]  = 128'h000000000000000062E7160F38B4DA56;
 assign  rc_64[3]  = 128'h0000000000000000A784D9045190CFEF;
 assign  rc_64[4]  = 128'h0000000000000000324E7738926CFBE5;
 assign  rc_64[5]  = 128'h0000000000000000F4BF8D8D8C31D763;
 assign  rc_64[6]  = 128'h0000000000000000DA06C80ABB1185EB;
 assign  rc_64[7]  = 128'h00000000000000004F7C7B5757F59584;
 assign  rc_64[8]  = 128'h000000000000000090CFD47D7C19BB42;
 assign  rc_64[9]  = 128'h0000000000000000158D9554F7B46BCE;
 assign  rc_64[10] = 128'h0000000000000000D55C4D79FD5F24D6;
 assign  rc_64[11] = 128'h0000000000000000613C31C3839A2DDF;
 assign  rc_64[12] = 128'h00000000000000008A9A276BCFBFA1C8;
 assign  rc_64[13] = 128'h000000000000000077C56284DAB79CD4;
 assign  rc_64[14] = 128'h0000000000000000C2B3293D20E9E5EA;
 assign  rc_64[15] = 128'h0000000000000000F02AC60ACC93ED87;
 assign  rc_64[16] = 128'h00000000000000004422A52ECB238FEE;
 assign  rc_64[17] = 128'h0000000000000000E5AB6ADD835FD1A0;
 assign  rc_64[18] = 128'h0000000000000000753D0A8F78E537D2;
 assign  rc_64[19] = 128'h0000000000000000B95BB79D8DCAEC64;
 assign  rc_64[20] = 128'h00000000000000002C1E9F23B829B5C2;
 assign  rc_64[21] = 128'h0000000000000000780BF38737DF8BB3;
 assign  rc_64[22] = 128'h000000000000000000D01334A0D0BD86;
 assign  rc_64[23] = 128'h000000000000000045CBFA73A6160FFE;

    // ==========================================
    // W=96 RC 常数 [32个] 96bit = {32bit, 64bit}
    // ==========================================
 assign  rc_96[0]  = 128'h00000000B7E151628AED2A6ABF715880;
 assign  rc_96[1]  = 128'h000000009CF4F3C762E7160F38B4DA56;
 assign  rc_96[2]  = 128'h00000000A784D9045190CFEF324E7738;
 assign  rc_96[3]  = 128'h00000000926CFBE5F4BF8D8D8C31D763;
 assign  rc_96[4]  = 128'h00000000DA06C80ABB1185EB4F7C7B57;
 assign  rc_96[5]  = 128'h0000000057F5958490CFD47D7C19BB42;
 assign  rc_96[6]  = 128'h00000000158D9554F7B46BCED55C4D79;
 assign  rc_96[7]  = 128'h00000000FD5F24D6613C31C3839A2DDF;
 assign  rc_96[8]  = 128'h000000008A9A276BCFBFA1C877C56284;
 assign  rc_96[9]  = 128'h00000000DAB79CD4C2B3293D20E9E5EA;
 assign  rc_96[10] = 128'h00000000F02AC60ACC93ED874422A52E;
 assign  rc_96[11] = 128'h00000000CB238FEEE5AB6ADD835FD1A0;
 assign  rc_96[12] = 128'h00000000753D0A8F78E537D2B95BB79D;
 assign  rc_96[13] = 128'h000000008DCAEC642C1E9F23B829B5C2;
 assign  rc_96[14] = 128'h00000000780BF38737DF8BB300D01334;
 assign  rc_96[15] = 128'h00000000A0D0BD8645CBFA73A6160FFE;
 assign  rc_96[16] = 128'h00000000393C48CBBBCA060F0FF8EC6D;
 assign  rc_96[17] = 128'h0000000031BEB5CCEED7F2F0BB088017;
 assign  rc_96[18] = 128'h00000000163BC60DF45A0ECB1BCD289B;
 assign  rc_96[19] = 128'h0000000006CBBFEA21AD08E1847F3F73;
 assign  rc_96[20] = 128'h0000000078D56CED94640D6EF0D3D37B;
 assign  rc_96[21] = 128'h00000000E67008E186D1BF275B9B241D;
 assign  rc_96[22] = 128'h00000000EB64749A47DFDFB96632C3EB;
 assign  rc_96[23] = 128'h00000000061B6472BBF84C26144E49C2;
 assign  rc_96[24] = 128'h00000000D04C324EF10DE513D3F5114B;
 assign  rc_96[25] = 128'h000000008B5D374D93CB8879C7D52FFD;
 assign  rc_96[26] = 128'h0000000072BA0AAE7277DA7BA1B4AF14;
 assign  rc_96[27] = 128'h0000000088D8E836AF14865E6C37AB68;
 assign  rc_96[28] = 128'h0000000076FE690B571121382AF341AF;
 assign  rc_96[29] = 128'h00000000E94F77BCF06C83B8FF5675F0;
 assign  rc_96[30] = 128'h00000000979074AD9A787BC5B9BD4B0C;
 assign  rc_96[31] = 128'h000000005937D3EDE4C3A79396215EDA;

    // ==========================================
    // W=128 RC 常数 [40个] 完整128bit
    // ==========================================
 assign  rc_128[0]  = 128'hB7E151628AED2A6ABF7158809CF4F3C7;
 assign  rc_128[1]  = 128'h62E7160F38B4DA56A784D9045190CFEF;
 assign  rc_128[2]  = 128'h324E7738926CFBE5F4BF8D8D8C31D763;
 assign  rc_128[3]  = 128'hDA06C80ABB1185EB4F7C7B5757F59584;
 assign  rc_128[4]  = 128'h90CFD47D7C19BB42158D9554F7B46BCE;
 assign  rc_128[5]  = 128'hD55C4D79FD5F24D6613C31C3839A2DDF;
 assign  rc_128[6]  = 128'h8A9A276BCFBFA1C877C56284DAB79CD4;
 assign  rc_128[7]  = 128'hC2B3293D20E9E5EAF02AC60ACC93ED87;
 assign  rc_128[8]  = 128'h4422A52ECB238FEEE5AB6ADD835FD1A0;
 assign  rc_128[9]  = 128'h753D0A8F78E537D2B95BB79D8DCAEC64;
 assign  rc_128[10] = 128'h2C1E9F23B829B5C2780BF38737DF8BB3;
 assign  rc_128[11] = 128'h00D01334A0D0BD8645CBFA73A6160FFE;
 assign  rc_128[12] = 128'h393C48CBBBCA060F0FF8EC6D31BEB5CC;
 assign  rc_128[13] = 128'hEED7F2F0BB088017163BC60DF45A0ECB;
 assign  rc_128[14] = 128'h1BCD289B06CBBFEA21AD08E1847F3F73;
 assign  rc_128[15] = 128'h78D56CED94640D6EF0D3D37BE67008E1;
 assign  rc_128[16] = 128'h86D1BF275B9B241DEB64749A47DFDFB9;
 assign  rc_128[17] = 128'h6632C3EB061B6472BBF84C26144E49C2;
 assign  rc_128[18] = 128'hD04C324EF10DE513D3F5114B8B5D374D;
 assign  rc_128[19] = 128'h93CB8879C7D52FFD72BA0AAE7277DA7B;
 assign  rc_128[20] = 128'hA1B4AF1488D8E836AF14865E6C37AB68;
 assign  rc_128[21] = 128'h76FE690B571121382AF341AFE94F77BC;
 assign  rc_128[22] = 128'hF06C83B8FF5675F0979074AD9A787BC5;
 assign  rc_128[23] = 128'hB9BD4B0C5937D3EDE4C3A79396215EDA;
 assign  rc_128[24] = 128'hB1F57D0B5A7DB461DD8F3C75540D0012;
 assign  rc_128[25] = 128'h1FD56E95F8C731E9C4D7221BBED0C62B;
 assign  rc_128[26] = 128'hB5A87804B679A0CAA41D802A4604C311;
 assign  rc_128[27] = 128'hB71DE3E5C6B400E024A6668CCF2E2DE8;
 assign  rc_128[28] = 128'h6876E4F5C50000F0A93B3AA7E6342B30;
 assign  rc_128[29] = 128'h2A0A47373B25F73E3B26D569FE2291AD;
 assign  rc_128[30] = 128'h36D6A147D1060B871A2801F978376408;
 assign  rc_128[31] = 128'h2FF592D9140DB1E9399DF4B0E14CA8E8;
 assign  rc_128[32] = 128'h8EE9110B2BD4FA98EED150CA6DD89322;
 assign  rc_128[33] = 128'h45EF7592C703F532CE3A30CD31C070EB;
 assign  rc_128[34] = 128'h36B4195FF33FB1C66C7D70F93918107C;
 assign  rc_128[35] = 128'hE2051FED33F6D1DE9491C7DEA6A5A442;
 assign  rc_128[36] = 128'hE154C8BB6D8D0362803BC248D414478C;
 assign  rc_128[37] = 128'h2AFB07FFE78E89B9FECA7E3060C08F0D;
 assign  rc_128[38] = 128'h61F8E36801DF66D1D8F9392E52CAEF06;
 assign  rc_128[39] = 128'h53199479DF2BE64BBAAB008CA8A06FDA;

always @(*) begin
    case(w_sel)
        `W_32:  begin 
            sigma_out0 = sigma_32[0]; rc_out0 = rc_32[0];
            sigma_out1 = sigma_32[1]; rc_out1 = rc_32[1];
            sigma_out2 = sigma_32[2]; rc_out2 = rc_32[2];
            sigma_out3 = sigma_32[3]; rc_out3 = rc_32[3];
            sigma_out4 = sigma_32[4]; rc_out4 = rc_32[4];
            sigma_out5 = sigma_32[5]; rc_out5 = rc_32[5];
            sigma_out6 = sigma_32[6]; rc_out6 = rc_32[6];
            sigma_out7 = sigma_32[7]; rc_out7 = rc_32[7];
            sigma_out8 = sigma_32[8]; rc_out8 = rc_32[8];
            sigma_out9 = sigma_32[9]; rc_out9 = rc_32[9];
            sigma_out10 = sigma_32[10]; rc_out10 = rc_32[10];
            sigma_out11 = sigma_32[11]; rc_out11 = rc_32[11];
            sigma_out12 = sigma_32[12]; rc_out12 = rc_32[12];
            sigma_out13 = sigma_32[13]; rc_out13 = rc_32[13];
            sigma_out14 = sigma_32[14]; rc_out14 = rc_32[14];
            sigma_out15 = sigma_32[15]; rc_out15 = rc_32[15];
            sigma_out16 = sigma_32[16]; rc_out16 = 128'd0;
            sigma_out17 = sigma_32[17]; rc_out17 = 128'd0;
            sigma_out18 = sigma_32[18]; rc_out18 = 128'd0;
            sigma_out19 = sigma_32[19]; rc_out19 = 128'd0;
            sigma_out20 = sigma_32[20]; rc_out20 = 128'd0;
            sigma_out21 = sigma_32[21]; rc_out21 = 128'd0;
            sigma_out22 = sigma_32[22]; rc_out22 = 128'd0;
            sigma_out23 = sigma_32[23]; rc_out23 = 128'd0;
                                        rc_out24 = 128'd0; 
                                        rc_out25 = 128'd0;
                                        rc_out26 = 128'd0;
                                        rc_out27 = 128'd0;
                                        rc_out28 = 128'd0;
                                        rc_out29 = 128'd0;
                                        rc_out30 = 128'd0;
                                        rc_out31 = 128'd0;
                                        rc_out32 = 128'd0;
                                        rc_out33 = 128'd0;
                                        rc_out34 = 128'd0;
                                        rc_out35 = 128'd0;
                                        rc_out36 = 128'd0;
                                        rc_out37 = 128'd0;
                                        rc_out38 = 128'd0;
                                        rc_out39 = 128'd0;
        end
        `W_64:  begin
            sigma_out0 = sigma_64[0]; rc_out0 = rc_64[0];
            sigma_out1 = sigma_64[1]; rc_out1 = rc_64[1];
            sigma_out2 = sigma_64[2]; rc_out2 = rc_64[2];
            sigma_out3 = sigma_64[3]; rc_out3 = rc_64[3];
            sigma_out4 = sigma_64[4]; rc_out4 = rc_64[4];
            sigma_out5 = sigma_64[5]; rc_out5 = rc_64[5];
            sigma_out6 = sigma_64[6]; rc_out6 = rc_64[6];
            sigma_out7 = sigma_64[7]; rc_out7 = rc_64[7];
            sigma_out8 = sigma_64[8]; rc_out8 = rc_64[8];
            sigma_out9 = sigma_64[9]; rc_out9 = rc_64[9];
            sigma_out10 = sigma_64[10]; rc_out10 = rc_64[10];
            sigma_out11 = sigma_64[11]; rc_out11 = rc_64[11];
            sigma_out12 = sigma_64[12]; rc_out12 = rc_64[12];
            sigma_out13 = sigma_64[13]; rc_out13 = rc_64[13];
            sigma_out14 = sigma_64[14]; rc_out14 = rc_64[14];
            sigma_out15 = sigma_64[15]; rc_out15 = rc_64[15];
            sigma_out16 = sigma_64[16]; rc_out16 = rc_64[16];
            sigma_out17 = sigma_64[17]; rc_out17 = rc_64[17];
            sigma_out18 = sigma_64[18]; rc_out18 = rc_64[18];
            sigma_out19 = sigma_64[19]; rc_out19 = rc_64[19];
            sigma_out20 = sigma_64[20]; rc_out20 = rc_64[20];
            sigma_out21 = sigma_64[21]; rc_out21 = rc_64[21];
            sigma_out22 = sigma_64[22]; rc_out22 = rc_64[22];
            sigma_out23 = sigma_64[23]; rc_out23 = rc_64[23];
                                        rc_out24 = 128'd0; 
                                        rc_out25 = 128'd0;
                                        rc_out26 = 128'd0;
                                        rc_out27 = 128'd0;
                                        rc_out28 = 128'd0;
                                        rc_out29 = 128'd0;
                                        rc_out30 = 128'd0;
                                        rc_out31 = 128'd0;
                                        rc_out32 = 128'd0;
                                        rc_out33 = 128'd0;
                                        rc_out34 = 128'd0;
                                        rc_out35 = 128'd0;
                                        rc_out36 = 128'd0;
                                        rc_out37 = 128'd0;
                                        rc_out38 = 128'd0;
                                        rc_out39 = 128'd0;
        end
        `W_96:  begin
            sigma_out0 = sigma_96[0]; rc_out0 = rc_96[0];
            sigma_out1 = sigma_96[1]; rc_out1 = rc_96[1];
            sigma_out2 = sigma_96[2]; rc_out2 = rc_96[2];
            sigma_out3 = sigma_96[3]; rc_out3 = rc_96[3];
            sigma_out4 = sigma_96[4]; rc_out4 = rc_96[4];
            sigma_out5 = sigma_96[5]; rc_out5 = rc_96[5];
            sigma_out6 = sigma_96[6]; rc_out6 = rc_96[6];
            sigma_out7 = sigma_96[7]; rc_out7 = rc_96[7];
            sigma_out8 = sigma_96[8]; rc_out8 = rc_96[8];
            sigma_out9 = sigma_96[9]; rc_out9 = rc_96[9];
            sigma_out10 = sigma_96[10]; rc_out10 = rc_96[10];
            sigma_out11 = sigma_96[11]; rc_out11 = rc_96[11];
            sigma_out12 = sigma_96[12]; rc_out12 = rc_96[12];
            sigma_out13 = sigma_96[13]; rc_out13 = rc_96[13];
            sigma_out14 = sigma_96[14]; rc_out14 = rc_96[14];
            sigma_out15 = sigma_96[15]; rc_out15 = rc_96[15];
            sigma_out16 = sigma_96[16]; rc_out16 = rc_96[16];
            sigma_out17 = sigma_96[17]; rc_out17 = rc_96[17];
            sigma_out18 = sigma_96[18]; rc_out18 = rc_96[18];
            sigma_out19 = sigma_96[19]; rc_out19 = rc_96[19];
            sigma_out20 = sigma_96[20]; rc_out20 = rc_96[20];
            sigma_out21 = sigma_96[21]; rc_out21 = rc_96[21];
            sigma_out22 = sigma_96[22]; rc_out22 = rc_96[22];
            sigma_out23 = sigma_96[23]; rc_out23 = rc_96[23];
                                        rc_out24 = rc_96[24]; 
                                        rc_out25 = rc_96[25];
                                        rc_out26 = rc_96[26];
                                        rc_out27 = rc_96[27];
                                        rc_out28 = rc_96[28];
                                        rc_out29 = rc_96[29];
                                        rc_out30 = rc_96[30];
                                        rc_out31 = rc_96[31];
                                        rc_out32 = 128'd0;
                                        rc_out33 = 128'd0;
                                        rc_out34 = 128'd0;
                                        rc_out35 = 128'd0;
                                        rc_out36 = 128'd0;
                                        rc_out37 = 128'd0;
                                        rc_out38 = 128'd0;
                                        rc_out39 = 128'd0;
        end
        `W_128: begin
            sigma_out0 = sigma_128[0]; rc_out0 = rc_128[0];
            sigma_out1 = sigma_128[1]; rc_out1 = rc_128[1];
            sigma_out2 = sigma_128[2]; rc_out2 = rc_128[2];
            sigma_out3 = sigma_128[3]; rc_out3 = rc_128[3];
            sigma_out4 = sigma_128[4]; rc_out4 = rc_128[4];
            sigma_out5 = sigma_128[5]; rc_out5 = rc_128[5];
            sigma_out6 = sigma_128[6]; rc_out6 = rc_128[6];
            sigma_out7 = sigma_128[7]; rc_out7 = rc_128[7];
            sigma_out8 = sigma_128[8]; rc_out8 = rc_128[8];
            sigma_out9 = sigma_128[9]; rc_out9 = rc_128[9];
            sigma_out10 = sigma_128[10]; rc_out10 = rc_128[10];
            sigma_out11 = sigma_128[11]; rc_out11 = rc_128[11];
            sigma_out12 = sigma_128[12]; rc_out12 = rc_128[12];
            sigma_out13 = sigma_128[13]; rc_out13 = rc_128[13];
            sigma_out14 = sigma_128[14]; rc_out14 = rc_128[14];
            sigma_out15 = sigma_128[15]; rc_out15 = rc_128[15];
            sigma_out16 = sigma_128[16]; rc_out16 = rc_128[16];
            sigma_out17 = sigma_128[17]; rc_out17 = rc_128[17];
            sigma_out18 = sigma_128[18]; rc_out18 = rc_128[18];
            sigma_out19 = sigma_128[19]; rc_out19 = rc_128[19];
            sigma_out20 = sigma_128[20]; rc_out20 = rc_128[20];
            sigma_out21 = sigma_128[21]; rc_out21 = rc_128[21];
            sigma_out22 = sigma_128[22]; rc_out22 = rc_128[22];
            sigma_out23 = sigma_128[23]; rc_out23 = rc_128[23];
                                        rc_out24 = rc_128[24]; 
                                        rc_out25 = rc_128[25];
                                        rc_out26 = rc_128[26];
                                        rc_out27 = rc_128[27];
                                        rc_out28 = rc_128[28];
                                        rc_out29 = rc_128[29];
                                        rc_out30 = rc_128[30];
                                        rc_out31 = rc_128[31];
                                        rc_out32 = rc_128[32];
                                        rc_out33 = rc_128[33];
                                        rc_out34 = rc_128[34];
                                        rc_out35 = rc_128[35];
                                        rc_out36 = rc_128[36];
                                        rc_out37 = rc_128[37];
                                        rc_out38 = rc_128[38];
                                        rc_out39 = rc_128[39];
        end
        default:begin 
            sigma_out0 = 7'd0; rc_out0 = 128'd0;
            sigma_out1 = 7'd0; rc_out1 = 128'd0;
            sigma_out2 = 7'd0; rc_out2 = 128'd0;
            sigma_out3 = 7'd0; rc_out3 = 128'd0;
            sigma_out4 = 7'd0; rc_out4 = 128'd0;
            sigma_out5 = 7'd0; rc_out5 = 128'd0;
            sigma_out6 = 7'd0; rc_out6 = 128'd0;
            sigma_out7 = 7'd0; rc_out7 = 128'd0;
            sigma_out8 = 7'd0; rc_out8 = 128'd0;
            sigma_out9 = 7'd0; rc_out9 = 128'd0;
            sigma_out10 = 7'd0; rc_out10 = 128'd0;
            sigma_out11 = 7'd0; rc_out11 = 128'd0;
            sigma_out12 = 7'd0; rc_out12 = 128'd0;
            sigma_out13 = 7'd0; rc_out13 = 128'd0;
            sigma_out14 = 7'd0; rc_out14 = 128'd0;
            sigma_out15 = 7'd0; rc_out15 = 128'd0;
            sigma_out16 = 7'd0; rc_out16 = 128'd0;
            sigma_out17 = 7'd0; rc_out17 = 128'd0;
            sigma_out18 = 7'd0; rc_out18 = 128'd0;
            sigma_out19 = 7'd0; rc_out19 = 128'd0;
            sigma_out20 = 7'd0; rc_out20 = 128'd0;
            sigma_out21 = 7'd0; rc_out21 = 128'd0;
            sigma_out22 = 7'd0; rc_out22 = 128'd0;
            sigma_out23 = 7'd0; rc_out23 = 128'd0;
                                        rc_out24 = 128'd0; 
                                        rc_out25 = 128'd0;
                                        rc_out26 = 128'd0;
                                        rc_out27 = 128'd0;
                                        rc_out28 = 128'd0;
                                        rc_out29 = 128'd0;
                                        rc_out30 = 128'd0;
                                        rc_out31 = 128'd0;
                                        rc_out32 = 128'd0;
                                        rc_out33 = 128'd0;
                                        rc_out34 = 128'd0;
                                        rc_out35 = 128'd0;
                                        rc_out36 = 128'd0;
                                        rc_out37 = 128'd0;
                                        rc_out38 = 128'd0;
                                        rc_out39 = 128'd0;
        end
    endcase
end

endmodule

