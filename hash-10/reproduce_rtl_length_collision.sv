`timescale 1ns/1ps

module tb;
  import feilian_pkg::*;

  logic clk = 0;
  logic rst = 0;
  logic start = 0;
  logic last = 1;
  logic [BLOCK_BITS-1:0] block = '0;
  logic [7:0] valid_bytes = 0;
  logic done, busy;
  logic [DIGEST_BITS-1:0] digest;
  logic [DIGEST_BITS-1:0] d1, d2;

`ifdef PORT_I
  core dut(.clk_i(clk), .rst_i(rst), .start_i(start), .block_i(block),
           .last_i(last), .valid_bytes_i(valid_bytes), .done_o(done),
           .digest_o(digest), .busy_o(busy));
`else
  core dut(.clk(clk), .rst(rst), .start(start), .block(block),
           .last(last), .valid_bytes(valid_bytes), .done(done),
           .digest(digest), .busy(busy));
`endif

  always #1 clk = ~clk;

  task reset_dut;
    begin
      rst = 1;
      repeat (3) @(posedge clk);
      rst = 0;
      @(posedge clk);
    end
  endtask

  task hash_one(input [7:0] length, output logic [DIGEST_BITS-1:0] result);
    begin
      valid_bytes = length;
      start = 1;
      @(posedge clk);
      start = 0;
      wait (done);
      result = digest;
      @(posedge clk);
    end
  endtask

  initial begin
    // Canonical byte strings 61 and 61 00.  Zero padding makes their block
    // input equal; a correct domain counter still distinguishes their lengths.
    block = '0;
    block[7:0] = 8'h61;
    reset_dut();
    hash_one(1, d1);
    reset_dut();
    hash_one(2, d2);
    if (d1 === d2)
      $display("COLLISION");
    else
      $display("DISTINCT");
    $finish;
  end
endmodule
