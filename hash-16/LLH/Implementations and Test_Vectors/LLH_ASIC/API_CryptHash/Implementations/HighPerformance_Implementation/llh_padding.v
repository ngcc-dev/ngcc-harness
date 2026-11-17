`timescale 1ns / 1ps
module msg_padder #(
    parameter MAX_BYTES = 128,           
    parameter DATA_W    = 32
) (
    input  wire               clk,
    input  wire               rst_n,

    input  wire [1:0]         w_sel,          // 当前W配置
    input  wire [127:0]       wdata,
    output reg                wready,
    input  wire               msg_valid,
    input       [63:0]        len,

    input  wire               start,

    output reg                out_valid,
    input  wire               out_ready,
    output reg [MAX_BYTES*8-1:0] buf_data, 

    output                    idle,
    output                    busy,
    output reg                last_group
);

localparam IDLE      = 2'd0,
           PADDING   = 2'd1,  //padding and send the remain bits
           SEND      = 2'd2,
           DONE      = 2'd3;

wire [63:0]    r_bits;
wire [3:0]     recv_times; 
wire [63:0]    data_group;
wire [63:0]    data_bits;
wire           send_state_done;
reg  [63:0]    group;
reg  [3:0]     recv_cnt;
wire [1023:0]  msg_pad;
reg  [63:0]    remain_data;
reg  [1:0]     state, next_state;
reg  [127:0]   fifo_data;
reg            padding_flag;


assign r_bits = (w_sel == 2'b00) ? 64'd256 :    
                (w_sel == 2'b01) ? 64'd512 :    
                (w_sel == 2'b10) ? 64'd768 :    
                                   64'd1024;
// one r_bits need receive 2/4/6/8 times 128bits wdate  
assign recv_times = (w_sel == 2'b00) ? 4'd2 :    
                    (w_sel == 2'b01) ? 4'd4 :    
                    (w_sel == 2'b10) ? 4'd6 :    
                                       4'd8;
// (L mod r =0) data group
assign data_group = len / r_bits;
//remain data bits
assign data_bits = len % r_bits;
//data group already sent, remain bits need padding
assign send_state_done = (group == data_group) && |data_bits;

always @(posedge clk ) begin
    if (!rst_n) begin
        state <= IDLE;
    end else begin
        state <= next_state;
    end
end
always @(*) begin
    case (state)
        IDLE: begin
            if (start && group <= data_group )begin
                next_state = SEND;
            end
            else if(start && ((group == data_group && |data_bits) | len == 64'd0))begin
                next_state = PADDING;
            end
            else  begin
                next_state = IDLE;
            end
        end
        // send group data to llh core when (L mod r =0)
        SEND: begin
            if(send_state_done)begin
                next_state = PADDING;
            end
            else if(group == data_group && ~(&data_bits))begin
                 next_state = DONE;
            end
            else begin
                 next_state = SEND;
            end
        end
        //padding the remain data and send to llh core        
        PADDING: begin
            if(remain_data >= data_bits)begin
                next_state = DONE;
            end
            else begin
                next_state = PADDING;
            end
        end
        DONE: begin
            next_state = DONE;
        end
        default: next_state = IDLE;
    endcase
end
assign  busy = (state != IDLE);
assign  idle = (state == IDLE);

always @(posedge clk ) begin
    if (!rst_n) begin
        out_valid   <= 1'b0;
        wready      <= 1'd0;
        recv_cnt    <= 4'd0;
        group       <= 64'd0;
        remain_data <= 64'd0;
        last_group  <= 1'd0;
        padding_flag <= 1'd0;
    end else begin
        case (state)
            IDLE: begin
                out_valid <= 1'b0;
            end
            SEND: begin
                if(recv_cnt != recv_times && out_valid == 1'd0 && wready == 1'd0)begin
                    wready <= 1'd1;
                end
                else if(msg_valid == 1'b1)begin
                    wready <= 1'd0;
                end
                //receive 128 bits data
                if(msg_valid && wready && ~out_valid)begin
                    recv_cnt <= recv_cnt + 4'd1;
                end     
                else if(recv_cnt == recv_times)begin
                    recv_cnt <= 4'd0;
                end
                //update data group already sent
                if(out_valid && out_ready)begin
                    group <= group + 64'd1;
                end
                else if(group == data_group)begin
                    group <= 64'd0;
                end
                if(recv_cnt == recv_times)begin
                    out_valid <= 1'b1;
                end
                else if(out_ready) begin
                    out_valid <= 1'b0;
                end
                if((group == data_group - 1'b1) && (data_bits == 64'd0))begin
                    last_group <= 1'd1;
                end
            end
            PADDING: begin
                padding_flag <= 1'd1;
                if(remain_data < data_bits && wready == 1'd0)begin
                    wready <= 1'd1;
                end
                else if(msg_valid == 1'b1)begin
                    wready <= 1'd0;
                end
                if(msg_valid && wready)begin
                    remain_data <= remain_data + 64'd128;
                end     
                else if(remain_data >= data_bits)begin
                    remain_data <= 64'd0;
                end
                if(remain_data >= data_bits)begin
                    out_valid  <= 1'b1;
                    last_group <= 1'd1;
                end
                else if(out_ready) begin
                    out_valid <= 1'b0;
                end
            end
            DONE: begin
                if(out_ready) begin
                    out_valid <= 1'b0;
                end
            end
            default: ;
        endcase
    end
end

always @(posedge clk ) begin
    if (!rst_n) begin
        buf_data <= 1024'd0;
    end else begin
        case (state)
            SEND: begin
                if(msg_valid && wready)begin
                    case(w_sel)
                      2'b00: buf_data <= {buf_data[895:768], wdata,{768{1'b0}}};
                      2'b01: buf_data <= {buf_data[895:512], wdata,{512{1'b0}}};
                      2'b10: buf_data <= {buf_data[895:256], wdata,{256{1'b0}}};
                      2'b11: buf_data <= {buf_data[895:0],   wdata};
                      default: ;
                    endcase
                end  
                else if(group == data_group) begin 
                    buf_data <= 1024'd0;
                end
            end
            PADDING: begin
                if(msg_valid && wready)begin
                    case(w_sel)
                      2'b00: buf_data <= {buf_data[895:768], wdata,{768{1'b0}}};
                      2'b01: buf_data <= {buf_data[895:512], wdata,{512{1'b0}}};
                      2'b10: buf_data <= {buf_data[895:256], wdata,{256{1'b0}}};
                      2'b11: buf_data <= {buf_data[895:0],   wdata};
                      default: ;
                    endcase
                end
            end
            DONE: begin
                if(padding_flag)begin
                    buf_data <= msg_pad;
                end
            end
            default: ;
        endcase
    end
end

wire [63:0]valid_len;
wire [63:0]fill_len;
wire [1023:0] fill_one;
wire [1023:0] data_shift;
wire [63:0] shift;

assign shift = 64'd1024-data_bits;
assign valid_len = r_bits - data_bits;
assign fill_len  = 64'd1024 - r_bits + valid_len;
assign fill_one = {1024{1'b1}} >> data_bits;
assign data_shift = valid_len == 64'd0 ? {1024{1'b1}} : ((buf_data >> shift) << fill_len);
assign msg_pad = data_shift |  fill_one;
endmodule

