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

module hash_top #(
    parameter W_MAX = `MAX_W
) (
    input  wire                 aclk,
    input  wire                 aresetn,

    input  wire [31:0]          awaddr,
    input  wire                 awvalid,
    output wire                 awready,

    input  wire [127:0]         wdata,
    input  wire [15:0]          wstrb,
    input  wire                 wvalid,
    output wire                 wready,

    output wire [1:0]           bresp,
    output wire                 bvalid,
    input  wire                 bready,

    input  wire [31:0]          araddr,
    input  wire                 arvalid,
    output wire                 arready,

    output wire [127:0]         rdata,
    output wire [1:0]           rresp,
    output wire                 rvalid,
    input  wire                 rready
);

// 内部信号
wire [1:0]          w_sel;
wire                start;
wire                rst_core;
wire                idle, busy, done;
wire [63:0]         msg_len;
wire [W_MAX*8-1:0]  data_in;
wire [W_MAX*8-1:0]  data_out;
wire out_valid;
wire out_ready;
wire [1023:0] out_data;
wire pad_idle;
wire pad_busy;
wire core_idle;
wire core_busy;
wire core_done;
assign idle = pad_idle && core_idle;
assign busy = pad_busy || core_busy;
assign done = core_done;

wire wready_data;
wire msg_valid;
wire [63:0] msg_cnt;
wire rcv_done_flag;


axi_slave u_axi(
    .aclk(aclk),
    .aresetn(aresetn),
    .awaddr(awaddr),.awvalid(awvalid),.awready(awready),
    .wdata(wdata),.wstrb(wstrb),.wvalid(wvalid),.wready(wready),
    .wready_data(wready_data),
    .bresp(bresp),.bvalid(bvalid),.bready(bready),
    .araddr(araddr),.arvalid(arvalid),.arready(arready),
    .rdata(rdata),.rresp(rresp),.rvalid(rvalid),.rready(rready),
    .cfg_w_sel(w_sel),.cfg_start(start),.cfg_rst(rst_core),
    .sts_idle(idle),.sts_busy(busy),.sts_done(done),
    .cfg_msg_len(msg_len), .msg_valid(msg_valid),
    .sts_data_out(data_out)
);

msg_padder u_msg_padder(
    .clk(aclk),
    .rst_n(aresetn & ~rst_core),
    .w_sel(w_sel),          
    .wdata(wdata),
    .wready(wready_data),
    .len(msg_len),
    .start(start),
    .out_valid(out_valid),
    .out_ready(out_ready),
    .buf_data(out_data),   
    .idle(pad_idle),
    .busy(pad_busy),
    .msg_valid(msg_valid),
    .last_group(rcv_done_flag)
    );

hash_core u_core(
    .clk(aclk),
    .rst_n(aresetn & ~rst_core),
    .w_sel(w_sel),
    .start(start),
    .core_ready(out_ready),
    .msg_len(msg_len),
    .data_in(out_data),
    .padding_flag(rcv_done_flag),
    .idle(core_idle),.busy(core_busy),.done(core_done),
    .data_out(data_out)
);

endmodule

