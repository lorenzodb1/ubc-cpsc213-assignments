#include <stdio.h>

int           reg[8];
unsigned char mem[40*4];
int           pc;
unsigned char insOpCode;
unsigned char insOp0;
unsigned char insOp1;
unsigned char insOp2;
char          insOpImm;
int           insOpExt;

void fetch() {
  insOpCode = mem [pc] >> 4;
  insOp0    = mem [pc] & 0xf;
  insOp1    = mem [pc+1] >> 4;
  insOp2    = mem [pc+1] & 0xf;
  insOpImm  = mem [pc+1];
  pc += 2;
  switch (insOpCode) {
      case 0x0:
      case 0xb:
        insOpExt = mem [pc] << 24 | mem [pc+1] << 16 | mem [pc+2] << 8 | mem [pc+3];
        pc += 4;
        break;
      default:
        ;
  }
}

int exec() {
  int cont = 1;
  int addr, val;
  
  static const void* jumptable[16] = {&&L0, &&L1, &&L2, &&L3, &&L4, &&DEFAULT, &&L6, &&L7, &&L8, &&L9, 
  									  &&LA, &&LB, &&LC, &&LD, &&LE, &&LF};
  									  
  static const void* alu_instructions[9] = {&&LL0, &&LL1, &&LL2, &&LL3, &&LL4, &&LL5, &&LL6, &&LL7, &&LLF}; 
  
  if (insOpCode < 0 || insOpCode > 16) 
      goto DEFAULT;
  else
  	  goto *jumptable[insOpCode];
  	  
  L0: // ld $i, d .............. 0d-- iiii iiii
	reg [insOp0] = insOpExt;
    goto END_JUMP;
  L1: // ld o(rs), rd .......... 1osd
  	addr = (insOp0 << 2) + reg [insOp1];
    reg [insOp2] = mem [addr] << 24 | mem [addr+1] << 16 | mem [addr+2] << 8 | mem [addr+3];
    goto END_JUMP;
  L2: // ld (rs, ri, 2), rd .... 2sid
  	addr = reg [insOp0] + (reg [insOp1] << 2);
  	reg [insOp2] = mem [addr] << 24 | mem [addr+1] << 16 | mem [addr+2] << 8 | mem [addr+3];
  	goto END_JUMP;
  L3: // st rs, o(rd) .......... 3sod
	addr = (insOp1 << 2) + reg [insOp2];
	val  = reg [insOp0];
	mem [addr]   = val >> 24 & 0xff;
	mem [addr+1] = val >> 16 & 0xff;
	mem [addr+2] = val >>  8 & 0xff;
	mem [addr+3] = val       & 0xff;
	goto END_JUMP;
  L4: // st rs, (rd, ri, 4) .... 4sdi
	addr = reg [insOp1] + (reg [insOp2] << 2);
	val  = reg [insOp0];
	mem [addr]   = val >> 24 & 0xff;
	mem [addr+1] = val >> 16 & 0xff;
	mem [addr+2] = val >>  8 & 0xff;
	mem [addr+3] = val       & 0xff;
	goto END_JUMP;
  
  L6: // ALU ................... 6-sd
  	
  	if (insOp0 < 0 || insOp0 > 16)
  		goto DEFAULT_ALU;
  	else
  		goto *alu_instructions[insOp0];
  		
	LL0: // mov rs, rd ........ 60sd
	  reg [insOp2] = reg [insOp1];
	  goto END_ALU;
	LL1: // add rs, rd ........ 61sd
	  reg [insOp2] = reg [insOp1] + reg [insOp2];
	  goto END_ALU;
	LL2: // and rs, rd ........ 62sd
	  reg [insOp2] = reg [insOp1] & reg [insOp2];
	  goto END_ALU;
	LL3: // inc rr ............ 63-r
	  reg [insOp2] = reg [insOp2] + 1;
	  goto END_ALU;
	LL4: // inca rr ........... 64-r
	  reg [insOp2] = reg [insOp2] + 4;
	  goto END_ALU;
	LL5: // dec rr ............ 65-r
	  reg [insOp2] = reg [insOp2] - 1;
	  goto END_ALU;
	LL6: // deca rr ........... 66-r
	  reg [insOp2] = reg [insOp2] -4;
	  goto END_ALU;
	LL7: // not ............... 67-r
	  reg [insOp2] = ~ reg [insOp2];
	  goto END_ALU;
	LLF: // gpc ............... 6f-r
	  reg [insOp2] = pc + (insOp1 << 1);
	  goto END_ALU;
	DEFAULT_ALU:
	  printf ("Illegal ALU instruction: pc=0x%x fun=0x%x\n", pc, insOp0);
	END_ALU:
	  goto END_JUMP;

  L7: // sh? $i,rd ............. 7dii
	if (insOpImm > 0)
	  reg [insOp0] = reg [insOp0] << insOpImm;
	else
	  reg [insOp0] = reg [insOp0] >> -insOpImm;
	goto END_JUMP;
  L8: // br o .................. 8-oo
	pc += insOpImm << 1;
	goto END_JUMP;
  L9: // beq rs, o ............. 9roo
	if (reg [insOp0] == 0)
	  pc += insOpImm << 1;
	goto END_JUMP;
  LA: // bgt rs, o .............. aroo
	if (reg [insOp0] > 0)
	  pc += insOpImm << 1;
	goto END_JUMP;
  LB: // j i ................... b--- iiii iiii
	pc = insOpExt;
	goto END_JUMP;
  LC: // j o(rr) ............... croo
	pc = (((unsigned short) insOpImm) << 1) + reg [insOp0];
	goto END_JUMP;
  LD: // j *o(rr) .............. droo
	// NYI
	goto END_JUMP;
  LE: // j*(rr,ri,4) ............. eri-
	// NYI
	goto END_JUMP;
  LF: // halt or nop ............. f?--
	if (insOp0 == 0)
	  cont = 0;
	goto END_JUMP;
  DEFAULT:
	printf ("Illegal  instruction: pc=0x%x opCode=0x%x\n", pc, insOpCode);
  END_JUMP:
    return cont;
}

void load2 (unsigned short v) {
  mem [pc]   = v >> 8;
  mem [pc+1] = v & 0xff;
  pc += 2;
}

void load4 (unsigned int v) {
  mem [pc]   = v >> 24 & 0xff;
  mem [pc+1] = v >> 16 & 0xff;
  mem [pc+2] = v >> 8  & 0xff;
  mem [pc+3] = v       & 0xff;
  pc += 4;
}

void loadMax () {
  pc = 0;
  load4 (-1);    // max
  load4 (0);     // i
  load4 (0x2);   // a[0]
  load4 (0x28);  // a[1]
  load4 (0x6);   // a[2]
  load4 (0x50);  // a[3]
  load4 (0xa);   // a[4]
  load4 (0x78);  // a[5]
  load4 (0xe);   // a[6]
  load4 (0x10);  // a[7]
  load4 (0x12);  // a[8]
  load4 (0x14);  // a[9]
  pc = 0x50;
  load2 (0x0000); load4 (0x00000000);
  load2 (0x0100); load4 (0xfffffff6);
  load2 (0x0200); load4 (0x00000000);
  load2 (0x1023);
  load2 (0x0400); load4 (0x00000008);
  load2 (0x6005);
  load2 (0x6115);
  load2 (0x9509);
  load2 (0x2405);
  load2 (0x6057);
  load2 (0x6707);
  load2 (0x6307);
  load2 (0x6137);
  load2 (0xa701);
  load2 (0x6053);
  load2 (0x6300);
  load2 (0x80f4);
  load2 (0x3302);
  load2 (0x0100); load4 (0x00000004);
  load2 (0x3001);
  load2 (0xf000);
  pc = 0x50;
}

void loadTest() {
  pc = 0x0;
  load2 (0x0200); load4 (0x00000030);
  load2 (0x1001);
  load2 (0x2001);
  load2 (0x3100);
  load2 (0x4100);
  load2 (0x6001);
  load2 (0x6101);
  load2 (0x6201);
  load2 (0x6301);
  load2 (0x6401);
  load2 (0x6501);
  load2 (0x6601);
  load2 (0x6701);
  load2 (0x6f01);
  load2 (0x7100);
  load2 (0x8000);
  load2 (0x9000);
  load2 (0xa000);
  load2 (0xb000); load4 (0x0000002e);
  load2 (0xc200);
  load2 (0xf000);
  pc = 0x0;
}

void showMem (int start, int count) {
  int i;
  for (i=start; i<count; i+=4)
    printf ("0x%08x: %02x %02x %02x %02x\n", i, mem [i], mem [i+1], mem [i+2], mem [i+3]);
}

void compute (void (*loader)(), int showStart, int showCount) {
  int cont = 0;
  
  loader();
  
  do {
    fetch();
    cont = exec();
  } while (cont);
  
  showMem (showStart, showCount);
}

int main (int argc, char** argv) {
  compute (loadMax,  0, 4);
}