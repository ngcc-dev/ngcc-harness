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
module barrel_shifter_512 #(
    parameter W_MAX = `MAX_W
) (
    input  wire [W_MAX-1:0]     din,        // 输入数据
    input  wire [6:0]           shift_num,  // 移位位数(0~127)
    output      [W_MAX-1:0]     dout        // 循环左移输出
);

assign dout = {din[63:0], din[63:0]} >> (64 - shift_num); // 64bit循环左移

endmodule

