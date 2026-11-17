#!/bin/sh
# hash-16 (LLH): the submitted ASIC RTL does not compute LLH for most message lengths.
# Run from the repository root:  sh hash-16/reproduce_asic_padder.sh
# Needs: iverilog/vvp, cc.  Builds in mktemp -d; touches nothing in the repository.
set -eu
cd "$(dirname "$0")/.." || exit 2
for t in cc iverilog vvp; do
    command -v "$t" >/dev/null 2>&1 || { echo "SKIP hash-16-1: $t unavailable"; exit 77; }
done
ROOT=$(pwd)
BASE="$ROOT/hash-16/LLH/Implementations and Test_Vectors"
REF="$BASE/LLH_C/API_CryptHash/Implementations/Reference_Implementation/LLH-256"
T=$(mktemp -d)
trap 'rm -r "$T"' EXIT
cd "$T"

cat > ref.c <<'XEOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int CryptHash(int, const unsigned char *, unsigned long long, unsigned char *);
int main(int argc, char **argv) {           /* ref <bits> <hex> */
    unsigned long long bits = strtoull(argv[1], 0, 10);
    unsigned char m[256] = {0}, d[32];
    size_t n = strlen(argv[2]) / 2;
    for (size_t i = 0; i < n; i++) { unsigned v; sscanf(argv[2] + 2 * i, "%2x", &v); m[i] = (unsigned char)v; }
    if (CryptHash(256, m, bits, d)) return 1;
    for (int i = 0; i < 32; i++) printf("%02x", d[i]);
    printf("\n");
    return 0;
}
XEOF
cc -O2 -std=gnu99 -I"$REF" ref.c "$REF/CryptHash_AlgorithmInstance.c" -o ref

cat > tb.v <<'XEOF'
`timescale 1ns/1ps
module tb;
  reg aclk=0, aresetn=0;
  reg [31:0] awaddr=0; reg awvalid=0; wire awready;
  reg [127:0] wdata=0; reg [15:0] wstrb=16'hFFFF; reg wvalid=0; wire wready;
  wire [1:0] bresp; wire bvalid; reg bready=0;
  reg [31:0] araddr=0; reg arvalid=0; wire arready;
  wire [127:0] rdata; wire [1:0] rresp; wire rvalid; reg rready=0;
  hash_top dut(.aclk(aclk),.aresetn(aresetn),.awaddr(awaddr),.awvalid(awvalid),.awready(awready),
     .wdata(wdata),.wstrb(wstrb),.wvalid(wvalid),.wready(wready),.bresp(bresp),.bvalid(bvalid),.bready(bready),
     .araddr(araddr),.arvalid(arvalid),.arready(arready),.rdata(rdata),.rresp(rresp),.rvalid(rvalid),.rready(rready));
  always #5 aclk=~aclk;
  reg [2047:0] msg; integer nch, i, guard, g; reg [63:0] len; reg [127:0] rd; reg [255:0] dout;
  task axi_write(input [31:0] addr, input [127:0] data);
    begin
      @(negedge aclk); awaddr=addr; awvalid=1;
      while(!awready) @(negedge aclk);
      @(negedge aclk); awvalid=0; wdata=data; wvalid=1;
      g=0; while(!wready && g<400) begin @(negedge aclk); g=g+1; end
      @(negedge aclk); wvalid=0; bready=1;
      g=0; while(!bvalid && g<50) begin @(negedge aclk); g=g+1; end
      @(negedge aclk); bready=0;
    end
  endtask
  task axi_read(input [31:0] addr, output [127:0] data);
    begin
      @(negedge aclk); araddr=addr; arvalid=1;
      while(!arready) @(negedge aclk);
      @(negedge aclk); arvalid=0; rready=1;
      while(!rvalid) @(negedge aclk);
      data=rdata;
      @(negedge aclk); rready=0;
    end
  endtask
  initial begin
    if(!$value$plusargs("LEN=%d", len)) len=0;
    if(!$value$plusargs("NCH=%d", nch)) nch=0;
    if(!$value$plusargs("MSG=%h", msg)) msg=0;
    repeat(4) @(posedge aclk); #1 aresetn=1; repeat(2) @(posedge aclk);
    axi_write(32'h0, {len, 32'h0, 29'h0, 1'b1, 2'b00});        // w_sel=00 (LLH-256), start=1, length in bits
    for (i=0;i<nch;i=i+1) axi_write(32'h10, msg[128*(nch-1-i) +: 128]);
    guard=0; rd=0;
    while (!rd[6] && guard<1000) begin axi_read(32'h0, rd); guard=guard+1; end
    axi_read(32'h10, dout[127:0]); axi_read(32'h20, dout[255:128]);
    if (rd[6]) $display("%h", dout); else $display("NODONE");
    $finish;
  end
endmodule
XEOF

pad32() { printf '%s' "$1"; n=$(( 64 - ${#1} )); while [ $n -gt 0 ]; do printf '0'; n=$((n-1)); done; }
fail=0
for tree in Basic_Implementation HighPerformance_Implementation LowSize_Implementation; do
  D="$BASE/LLH_ASIC/API_CryptHash/Implementations/$tree"
  iverilog -g2005 -o sim tb.v "$D"/*.v
  hdl() { vvp -n sim +LEN="$1" +NCH="$2" ${3:+"+MSG=$3"} | grep -v finish | tail -1; }
  # sanity: a full 256-bit block is hashed correctly, so the test bench drives the core as intended
  M=000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f
  [ "$(hdl 256 2 $M)" = "$(./ref 256 $M)" ] || { echo "$tree: sanity check failed"; fail=1; }
  # (a) one-byte messages: the digest does not depend on the message byte
  A=$(hdl 8 1 $(pad32 00)); B=$(hdl 8 1 $(pad32 5a)); C=$(hdl 8 1 $(pad32 ff))
  echo "$tree: RTL H(00)=$A"; echo "$tree: RTL H(5a)=$B"; echo "$tree: RTL H(ff)=$C"; echo "$tree: ref H(5a)=$(./ref 8 5a)"
  [ "$A" = "$B" ] && [ "$B" = "$C" ] && [ "$B" != "$(./ref 8 5a)" ] || fail=1
  # (b) 251-bit message (all chunks needed): RTL pads the partial byte in the opposite bit order to the specification
  M2=a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a5a540
  R=$(hdl 251 2 $M2); echo "$tree: 251-bit message RTL=$R"; echo "$tree: 251-bit message ref=$(./ref 251 $M2)"
  [ "$R" != "$(./ref 251 $M2)" ] || fail=1
  # (c) empty message: 'done' is never raised
  E=$(hdl 0 0); echo "$tree: empty message RTL=$E ref=$(./ref 0 '')"
  [ "$E" = "NODONE" ] || fail=1
done
if [ $fail -eq 0 ]; then
  echo "ATTACK hash-16-1 ASIC RTL: short final block ignored (colliding digests), wrong partial-byte order, empty message hangs: CONFIRMED"
else
  echo "ATTACK hash-16-1 NOT reproduced"; exit 1
fi
