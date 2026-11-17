`timescale 1ns / 1ps
`define  MAX_W        128      // 最大字宽（硬件固定128bit，动态适配小位宽）
`define  ROWS         24       // 固定行数
`define  RATE_ROWS    4        // 固定Rate行数
`define  CAP_ROWS     20       // 固定Cap行数
`define  MAX_NR       40       // 最大轮数（W=128时40轮）
`define  AXI_DATA_W   128       // AXI数据位宽
`define  AXI_ADDR_W   32        // AXI地址位宽

// W配置选择编码
`define  W_32         2'b00
`define  W_64         2'b01
`define  W_96         2'b10
`define  W_128        2'b11
// AXI-Lite从机：实现配置寄存器读写（W选择、启动、状态、数据）
module axi_slave #(
    parameter DATA_W = `AXI_DATA_W,
    parameter ADDR_W = `AXI_ADDR_W
) (
    input  wire                 aclk,
    input  wire                 aresetn,

    input  wire [ADDR_W-1:0]    awaddr,
    input  wire                 awvalid,
    output                      awready,

    input  wire [DATA_W-1:0]    wdata,
    input  wire [DATA_W/8-1:0]  wstrb,
    input  wire                 wvalid,
    output                      wready,

    output      [1:0]           bresp,
    output                      bvalid,
    input  wire                 bready,

    input  wire [ADDR_W-1:0]    araddr,
    input  wire                 arvalid,
    output                      arready,

    output      [DATA_W-1:0]    rdata,
    output      [1:0]           rresp,
    output                      rvalid,
    input  wire                 rready,

    output      [1:0]           cfg_w_sel,      // W配置(00=32/01=64/10=96/11=128)
    output                      cfg_start,      // 启动哈希
    output                      cfg_rst,        // 软复位
    input                       sts_idle,       // 空闲状态
    input                       sts_busy,       // 忙状态
    input                       sts_done,       // 完成标志
    output      [63:0]          cfg_msg_len,    // 消息长度(字节)
    input       [1024-1:0]      sts_data_out,    // 输出哈希值
    input                       wready_data,
    output                      msg_valid
);

//state define
reg      [7:0] sm;
reg      [7:0] sm_next;
parameter IDLE = 8'h00; 

parameter READ_REG      = 8'h01;
parameter WRITE_REG     = 8'h02;
parameter WRITE_REG_B   = 8'h03;
parameter WRITE_MSG     = 8'h04;
parameter WRITE_MSG_B   = 8'h05;

/////////////////////////////////////////
/////////state transform logic///////////
/////////////////////////////////////////
always @ (posedge aclk or negedge aresetn)begin
    if(!aresetn) begin
        sm <= IDLE;
    end
    else begin
        sm <= sm_next;
    end
end

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////next state logic///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

always @ (*) begin
    case (sm)
        IDLE: begin
            if(arvalid&arready) begin
                sm_next = READ_REG;
            end
            else if(awvalid&awready&(awaddr[6:4]!=3'b001)) begin//reg:0x0,0x4,0x8,0xc
                sm_next = WRITE_REG;
            end
            else if(awvalid&awready&(awaddr[6:4]==3'b001)) begin//msg reg
                sm_next = WRITE_MSG;
            end
            else begin
                sm_next = IDLE;
            end
        end
        READ_REG:begin
            if(rready&rvalid) begin
                sm_next = IDLE;
            end
            else begin
                sm_next = READ_REG;
            end
        end
        WRITE_REG:begin
            if(wready&wvalid) begin
                sm_next = WRITE_REG_B;
            end
            else begin
                sm_next = WRITE_REG;
            end
        end
        WRITE_REG_B:begin
            if(bready&bvalid) begin
                sm_next = IDLE;
            end
            else begin
                sm_next = WRITE_REG_B;
            end
        end
        WRITE_MSG:begin
            if(wvalid & wready)begin 
                sm_next = WRITE_MSG_B;
            end
            else begin
                sm_next = WRITE_MSG;
            end
        end
        WRITE_MSG_B:begin
            if(bready&bvalid) begin
                sm_next = IDLE;
            end
            else begin
                sm_next = WRITE_MSG_B;
            end
        end
        default:begin
            sm_next = IDLE;
        end
    endcase
end

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////state opration logic///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

reg [31:0] reg0;
reg [31:0] reg1;
reg [31:0] reg2;
reg [31:0] reg3;

wire [31:0] reg0_mask = {{8{~wstrb[3] }}, {8{~wstrb[2]  }}, {8{~wstrb[1]}}, {8{~wstrb[0]}}};
wire [31:0] reg1_mask = {{8{~wstrb[7] }}, {8{~wstrb[6]  }}, {8{~wstrb[5]}}, {8{~wstrb[4]}}};
wire [31:0] reg2_mask = {{8{~wstrb[11]}}, {8{~wstrb[10] }}, {8{~wstrb[9] }},{8{~wstrb[8]}}};
wire [31:0] reg3_mask = {{8{~wstrb[15]}}, {8{~wstrb[14]}},  {8{~wstrb[13]}},{8{~wstrb[12]}}};

reg [32   -1:0] araddr_r;
reg [32   -1:0] awaddr_r;

always @ (posedge aclk or negedge aresetn) begin
    if(!aresetn) begin
        reg0<= 32'h0;    //[31:16] version = 'h01;
        reg1<= 32'h0;
        reg2<= 32'h0;
        reg3<= 32'h0;
        araddr_r   <= 32'h0;
        awaddr_r   <= 32'h0;
    end
    else begin
        case(sm) 
            IDLE:begin
                if(arvalid&arready) begin
                    araddr_r       <= araddr;
                end
                else if(awvalid&awready&(awaddr[6:4]!=3'b001))begin
                    awaddr_r      <= awaddr;
                end
                else if(awvalid&awready&(awaddr[6:4]==3'b001)) begin
                    awaddr_r      <= awaddr;
                end
            end
            READ_REG:begin
            end
            WRITE_REG:begin
                if(wvalid&wready)begin
                    if(awaddr_r[6:4]==3'b000)begin
                    {reg3, reg2, reg1, reg0} <= {
                        ((reg3&reg3_mask) | (wdata[127:96]&(~reg3_mask))),
                        ((reg2&reg2_mask) | (wdata[ 95:64]&(~reg2_mask))),
                        ((reg1&reg1_mask) | (wdata[ 63:32]&(~reg1_mask))),
                        ((reg0&reg0_mask) | (wdata[ 31: 0]&(~reg0_mask)))
                    };
                    end
                end
            end
            WRITE_REG_B:begin
            end
            WRITE_MSG:begin
            end
            WRITE_MSG_B:begin
            end
            default:begin
            end
        endcase
    end
end

assign arready = (sm==IDLE)? 1'b1 : 1'b0;

assign rvalid  = (sm==READ_REG) ? 1'b1 : 1'b0;
assign rdata = (araddr_r[7:4]==4'b0000) ? {
                                      reg3,
                                      reg2,
                                      reg1, 
                                      16'd1,reg0[15:7],sts_done,sts_busy,sts_idle,reg0[3:0]
                                      }:
               (araddr_r[7:4]==4'b0001)? sts_data_out[127:0]:
               (araddr_r[7:4]==4'b0010)? sts_data_out[255:128]:
               (araddr_r[7:4]==4'b0011)? sts_data_out[383:256]:
               (araddr_r[7:4]==4'b0100)? sts_data_out[511:384]:
               (araddr_r[7:4]==4'b0101)? sts_data_out[639:512]:
               (araddr_r[7:4]==4'b0110)? sts_data_out[767:640]:
               (araddr_r[7:4]==4'b0111)? sts_data_out[895:768]:
               (araddr_r[7:4]==4'b1000)? sts_data_out[1023:896]:
               128'h0;

assign rresp  = 2'h0;

assign awready = (sm==IDLE & arvalid==1'b0) ? 1'b1 : 1'b0;//read first

assign wready  = (sm==WRITE_REG) ? 1'b1 : 
                 (sm==WRITE_MSG) ? wready_data :1'b0;

assign bvalid = ((sm==WRITE_REG_B)||(sm==WRITE_MSG_B))? 1'b1 : 1'b0;
assign bresp = 2'h0;

assign cfg_w_sel = reg0[1:0];
assign cfg_start = reg0[2];
assign cfg_rst   = reg0[3];
assign cfg_msg_len = {reg3, reg2};

assign msg_valid = (sm==WRITE_MSG) ? wvalid : 1'b0;


endmodule

