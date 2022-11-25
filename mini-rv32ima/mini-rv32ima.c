#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/ioctl.h>
#include <termios.h>

//#define DEBUG_INSTRUCTIONS
#ifdef DEBUG_INSTRUCTIONS
#define INST_DBG( x... ) printf( x );
#else
#define INST_DBG( x... )
#endif

#ifdef SOME_EXTRA_DEBUG
#define INST_INFO( x... ) printf( x );
#else
#define INST_INFO( x... )
#endif

#define MINIRV32WARN( x... ) printf( x );

uint64_t SimpleReadNumberUInt( const char * number, uint64_t defaultNumber );

uint32_t ram_amt = 64*1024*1024;
uint32_t ram_image_offset = 0x80000000;

//Tricky: First 32 + 4 words are internal CPU state.
uint8_t * ram_image = 0;

// As a note: We quouple-ify these, because in HLSL, we will be operating with
// uint4's.  We are going to uint4 data to/from system RAM.
//
// We're going to try to keep the full processor state to 12 x uint4.
struct InternalCPUState
{
	uint32_t registers[32];

	uint32_t pc;
	uint32_t cycleh;
	uint32_t cyclel;
	uint32_t mstatus;

	uint32_t timerh;
	uint32_t timerl;
	uint32_t timermatchh;
	uint32_t timermatchl;

	uint32_t mscratch;
	uint32_t mtvec;
	uint32_t mie;
	uint32_t mip;

	uint32_t mepc;
	uint32_t mtval;
	uint32_t mcause;
	
	// Note: only like a few bits are used.  (Machine = 3, User = 0)
	// Bits 0..2 = privilege.
	// Bit 7 = pending char in in buffer.
	// Bits 8..15 = pending char.
	uint32_t extraflags; 

	uint8_t * image;
};


int StepInstruction( struct InternalCPUState * state, uint8_t * image, uint32_t vProcAddress );

void reset_keyboard()
{
	struct termios term;
	tcgetattr(0, &term);
	term.c_lflag |= ICANON | ECHO;
	tcsetattr(0, TCSANOW, &term);
}


int main( int argc, char ** argv )
{
    atexit(reset_keyboard);

	struct InternalCPUState core = { 0 };
	int i;
	long long instct = -1;
	int show_help = 0;
	int dtb_ptr = 0;
	const char * image_file_name = 0;
	const char * dtb_file_name = 0;
	for( i = 1; i < argc; i++ )
	{
		const char * param = argv[i];
		if( param[0] == '-' )
		{
			switch( param[1] )
			{
			case 'm':
				i++;
				if( i < argc )
					ram_amt = SimpleReadNumberUInt( argv[i], ram_amt );
				break;
			case 'c':
				i++;
				if( i < argc )
					instct = SimpleReadNumberUInt( argv[i], -1 );
				break;
			case 'f':
				i++;
				image_file_name = (i<argc)?argv[i]:0;
				break;
			case 'b':
				i++;
				dtb_file_name = (i<argc)?argv[i]:0;
				break;
			default:
				show_help = 1;
				break;
			}
		}
		else
		{
			show_help = 1;
		}
	}
	if( show_help || image_file_name == 0 )
	{
		fprintf( stderr, "./mini-rv32imaf [parameters]\n\t-f [running image]\n\t-b [dtb file]\n\t-c instruction count\n" );
		return 1;
	}

	{
		FILE * f = fopen( image_file_name, "rb" );
		if( !f || ferror( f ) )
		{
			fprintf( stderr, "Error: Could not open image \"%s\"\n", image_file_name );
			return -5;
		}
		fseek( f, 0, SEEK_END );
		long flen = ftell( f );
		fseek( f, 0, SEEK_SET );
		if( flen > ram_amt )
		{
			fprintf( stderr, "Error: Could not fit RAM image (%ld bytes) into %d\n", flen, ram_amt );
			return -6;
		}

		ram_image = malloc( ram_amt );
		memset( ram_image, 0, ram_amt );
		if( fread( ram_image, flen, 1, f ) != 1)
		{
			fprintf( stderr, "Error: Could not load image.\n" );
			return -7;
		}
		fclose( f );
		
		if( dtb_file_name )
		{
			f = fopen( dtb_file_name, "rb" );
			if( !f || ferror( f ) )
			{
				fprintf( stderr, "Error: Could not open dtb \"%s\"\n", dtb_file_name );
				return -5;
			}
			fseek( f, 0, SEEK_END );
			long dtblen = ftell( f );
			fseek( f, 0, SEEK_SET );
			dtb_ptr = ram_amt - dtblen;
			if( fread( ram_image + dtb_ptr, dtblen, 1, f ) != 1 )
			{
				fprintf( stderr, "Error: Could not open dtb \"%s\"\n", dtb_file_name );
				return -9;
			}
			fclose( f );
		}
	}

	{
		struct termios term;
		tcgetattr(0, &term);
		term.c_lflag &= ~(ICANON | ECHO); // Disable echo as well
		tcsetattr(0, TCSANOW, &term);
	}

	core.pc = ram_image_offset;
	core.registers[10] = 0x00; //hart ID
	core.registers[11] = dtb_ptr?(dtb_ptr+0x80000000):0; //dtb_pa (Must be valid pointer) (Should be pointer to dtb)
	core.extraflags |= 3; // Machine-mode.
	core.image = ram_image;

	// Image is loaded.
	long long rt;
	for( rt = 0; rt < instct || instct < 0; rt++ )
	{
		int ret = StepInstruction( &core, ram_image, 0 );
		if( ret == 2 )
		{
			//HandleUART( &core, ram_image );
		}
		else if( ret != 0 )
		{
			break;
		}
	}
}

int StepInstruction( struct InternalCPUState * state, uint8_t * image, uint32_t vProcAddress )
{
	uint32_t * regs = state->registers;
	static int alsolog;

	// Handle Timer interrupt.
	if( ( state->timerh > state->timermatchh || ( state->timerh == state->timermatchh && state->timerl > state->timermatchl ) ) && ( state->timermatchh || state->timermatchl )  )
		state->mip |= 1<<7; //MSIP of MIP // https://stackoverflow.com/a/61916199/2926815  Fire interrupt.
	else
		state->mip &= ~(1<<7);

	//state->mstatus & 8 = MIE, & 0x80 = MPIE
	// On an interrupt, the system moves current MIE into MPIE
	if( ( state->mip & state->mie & (1<<7) /*mtie*/ ) && ( state->mstatus & 0x8 /*mie*/) )
	{
		state->mstatus = (( state->mstatus & 0x08) << 4) | ( (state->extraflags & 3) << 11 );
		state->extraflags |= 3; // HMM THERE ARE CONDITION WHERE THIS IS NOT TRUE.  XXX CNL XXX  ===>>> ACTUALLY IT IS ALWAYS 3 BUT WE SHOULD PUT OLD STATUS IN???
		state->mepc = state->pc;
		state->mtval = 0;
		state->mcause = 0x80000007; //MSB = "Interrupt 1" 7 = "Machine timer interrupt"
		state->pc = state->mtvec;
		return 0;
	}

	uint32_t pc = state->pc;

	uint32_t ofs_pc = pc - ram_image_offset;
	if( ofs_pc & 3 || ofs_pc >= ram_amt )
	{
		fprintf( stderr, "Error: CPU PC invalid: %08x\n", state->pc );
		return -1;
	}

	uint32_t ir = *(uint32_t*)(ram_image + ofs_pc);
	int retval = 0;

#if 1
	if( alsolog )
	{
		if( alsolog ) alsolog--;
		printf( "SPART: %d %08x [%08x] Z:%08x A1:%08x %08x %08x %08x %08x %08x %08x // %08x %08x %08x %08x %08x %08x %08x %08x//x16: %08x %08x %08x %08x %08x %08x %08x %08x\n", retval, pc, ir,
			regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7],
			regs[8], regs[9], regs[10], regs[11], regs[12], regs[13], regs[14], regs[15],
			regs[16], regs[17], regs[18], regs[19], regs[20], regs[21], regs[22], regs[23] );
	}
#endif

	switch( ir & 0x7f )
	{
		case 0b0110111: // LUI
		{
			uint32_t rdid = (ir >> 7) & 0x1f;
			if( rdid ) regs[ rdid ] = ( ir & 0xfffff000 ) ;
			break;
		}
		case 0b0010111: // AUIPC
		{
			uint32_t rdid = (ir >> 7) & 0x1f;
			if( rdid ) regs[ rdid ] = pc + ( ir & 0xfffff000 );
			break;
		}
		case 0b1101111: // JAL
		{
			uint32_t rdid = (ir >> 7) & 0x1f;
			int32_t reladdy = ((ir & 0x80000000)>>11) | ((ir & 0x7fe00000)>>20) | ((ir & 0x00100000)>>9) | ((ir&0x000ff000));
			if( reladdy & 0x00100000 ) reladdy |= 0xffe00000; // Sign extension.
			if( rdid ) regs[ rdid ] = pc + 4;
			pc = pc + reladdy - 4;
			break;
		}
		case 0b1100111: // JALR
		{
			uint32_t imm = ir >> 20;
			uint32_t rdid = (ir >> 7) & 0x1f;
			int32_t imm_se = imm | (( imm & 0x800 )?0xfffff000:0);
			uint32_t t = pc + 4;
			pc = ( (regs[ (ir >> 15) & 0x1f ] + imm_se) & ~1) - 4;
			if( rdid ) regs[ rdid ] = t;
			break;
		}
		case 0b1100011: // Branch
		{
			uint32_t immm4 = ((ir & 0xf00)>>7) | ((ir & 0x7e000000)>>20) | ((ir & 0x80) << 4) | ((ir >> 31)<<12);
			if( immm4 & 0x1000 ) immm4 |= 0xffffe000;
			int32_t rs1 = regs[(ir >> 15) & 0x1f];
			int32_t rs2 = regs[(ir >> 20) & 0x1f];
			immm4 = pc + immm4 - 4;
			switch( ( ir >> 12 ) & 0x7 )
			{
				// BEQ, BNE, BLT, BGE, BLTU, BGEU 
				case 0b000: if( rs1 == rs2 ) pc = immm4; break;
				case 0b001: if( rs1 != rs2 ) pc = immm4; break;
				case 0b100: if( rs1 < rs2 ) pc = immm4; break;
				case 0b101: if( rs1 >= rs2 ) pc = immm4; break; //BGE
				case 0b110: if( (uint32_t)rs1 < (uint32_t)rs2 ) pc = immm4; break;   //BLTU
				case 0b111: if( (uint32_t)rs1 >= (uint32_t)rs2 ) pc = immm4; break;  //BGEU
				default: retval = -1;
			}
			break;
		}
		case 0b0000011: // Load
		{
			uint32_t rdid = (ir >> 7) & 0x1f;
			uint32_t rs1 = regs[(ir >> 15) & 0x1f];
			uint32_t imm = ir >> 20;
			int32_t imm_se = imm | (( imm & 0x800 )?0xfffff000:0);
			uint32_t rsval = rs1 + imm_se;
			uint32_t loaded = 0;
			rsval -= ram_image_offset;
			if( rsval >= ram_amt-3 )
			{
				if( rsval >= 0x90000000 && rsval < 0x90000008 ) 
				{
					int rval = 0;
					int byteswaiting;
					ioctl(0, FIONREAD, &byteswaiting);
					if( rsval == 0x90000005 )
						rval = 0x60 | !!byteswaiting;
					else if( rsval == 0x90000000 && byteswaiting )
						rval = getchar();
					if( rdid ) regs[rdid] = rval;
				}
				else if( rsval >= 0x82000000 && rsval < 0x82010000 )
				{
					uint32_t val = 0;

					// https://chromitem-soc.readthedocs.io/en/latest/clint.html
					if( rsval == 0x8200bffc )
						val = state->timerh;
					else if( rsval == 0x8200bff8 )
						val = state->timerl;
					else
						printf( "****************FAIL***************************** CLNT Access READ [%08x] (%08x + %08x)\n", rsval, rs1, imm_se );
					if( rdid ) regs[rdid] = val;
				}
				else
				{
					retval = -99;
					printf( "Load OOB Access [%08x] (%08x + %08x)\n", rsval, rs1, imm_se );
				}
			}
			else
			{
				switch( ( ir >> 12 ) & 0x7 )
				{
					//LB, LH, LW, LBU, LHU
					case 0b000: loaded = *((int8_t*)(image + rsval)); break;
					case 0b001: loaded = *((int16_t*)(image + rsval)); break;
					case 0b010: loaded = *((uint32_t*)(image + rsval)); break;
					case 0b100: loaded = *((uint8_t*)(image + rsval)); break;
					case 0b101: loaded = *((uint16_t*)(image + rsval)); break;
					default: retval = -1;
				}
				if( rsval == 0x00293E5C )
				{
					//printf( ">>>>>>>>>>LOADING 80293E5C = %08x, PC: %08x\n", loaded, pc );
				}
				if( rdid ) regs[rdid] = loaded;
				INST_DBG( "LOAD [%d, %08x] = %08x  [%x]\n", rdid,rsval, regs[rdid], ( ir >> 12 ) & 0x7);
			}
			break;
		}
		case 0b0100011: // Store
		{
			uint32_t rs1 = regs[(ir >> 15) & 0x1f];
			uint32_t rs2 = regs[(ir >> 20) & 0x1f];
			uint32_t addy = ( ( ir >> 7 ) & 0x1f ) | ( ( ir & 0xfe000000 ) >> 20 );
			INST_DBG( "STORE ADDY: %08x + %08x; ", addy, rs1 );
			if( addy & 0x800 ) addy |= 0xfffff000;
			INST_DBG( "%08x\n", addy );
			addy += rs1 - ram_image_offset;

			if( addy >= ram_amt-3 )
			{
				if( addy >= 0x90000000 && addy < 0x90000008 ) 
				{
					//Special: UART (8250)
					//If writing a byte, allow it to flow into output.
					if( addy == 0x90000000 )
					{
						printf( "%c", rs2 );
						fflush( stdout );
					}
				}
				else if( addy >= 0x82000000 && addy < 0x82010000 )
				{

					if( addy == 0x82004004 )
						state->timermatchh = rs2;
					else if( addy == 0x82004000 )
						state->timermatchl = rs2;
					// Other CLNT access is ignored.
				}
				else
				{
					retval = -99;
					printf( "Store OOB Access [%08x]\n", addy );
				}
			}
			else
			{
				switch( ( ir >> 12 ) & 0x7 )
				{
					//SB, SH, SW
					case 0b000: *((uint8_t*)(image + addy)) = rs2; break;
					case 0b001: *((uint16_t*)(image + addy)) = rs2; break;
					case 0b010: *((uint32_t*)(image + addy)) = rs2; break;
					default: retval = -1;
				}
				INST_DBG( "STORE [%08x] = %08x [%x]\n", addy, rs2, ( ir >> 12 ) & 0x7 );
				if( addy == 0x00293E5C )
				{
					//printf( ">>>>>>>>>>saving 80293E5C = %08x, PC = %08x\n", rs2, pc);
				}
			}
			break;
		}
		case 0b0010011: // Op-immediate
		{
			uint32_t imm = ir >> 20;
			imm = imm | (( imm & 0x800 )?0xfffff000:0);
			uint32_t rs1 = regs[(ir >> 15) & 0x1f];
			uint32_t rs2id = (ir >> 20) & 0x1f;
			uint32_t rdid = (ir >> 7) & 0x1f;
			uint32_t val = 0;
			// ADDI, SLTI, SLTIU, XORI, ORI, ANDI
			switch( (ir>>12)&7 )
			{
				case 0b000: val = rs1 + imm; break;
				case 0b001: val = rs1 << rs2id; break;
				case 0b010: val = (int)rs1 < (int)imm; break;  //signed (SLTI)
				case 0b011: val = rs1 < imm; break; //unsigned (SLTIU)
				case 0b100: val = rs1 ^ imm; break;
				case 0b101: val = (ir & 0x40000000 ) ? ( ((int32_t)rs1) >> rs2id ) : (rs1 >> rs2id); break;
				case 0b110: val = rs1 | imm; break;
				case 0b111: val = rs1 & imm; break;
				default: retval = -1;
			}
			if( rdid )
				regs[rdid] = val;
			INST_DBG( "OP-IMMEDIATE:%d = %08x\n", rdid, regs[rdid] );
			break;
		}
		case 0b0110011: // Op
		{
			uint32_t rs1 = regs[(ir >> 15) & 0x1f];
			uint32_t rs2 = regs[(ir >> 20) & 0x1f];
			uint32_t rdid = (ir >> 7) & 0x1f;
			uint32_t val = 0;
			if( rdid )
			{
				if( ir & 0x02000000 )
				{
					// RV32M
					// XXX TODO: Check MULH/MULHSU/MULHU

					{
						switch( (ir>>12)&7 )
						{
							case 0b000: val = rs1 * rs2; break; // MUL
							case 0b001: val = ((int64_t)rs1 * (int64_t)rs2) >> 32; break; // MULH
							case 0b010: val = ((int64_t)rs1 * (uint64_t)rs2) >> 32; break; // MULHSU
							case 0b011: val = ((uint64_t)rs1 * (uint64_t)rs2) >> 32; break; // MULHU
							case 0b100: if( rs2 == 0 ) val = -1; else val = (int32_t)rs1 / (int32_t)rs2; break; // DIV
							case 0b101: if( rs2 == 0 ) val = 0xffffffff; else val = rs1 / rs2; break; // DIVU
							case 0b110: if( rs2 == 0 ) val = rs1; else val = (int32_t)rs1 % (int32_t)rs2; break; // REM
							case 0b111: if( rs2 == 0 ) val = rs1; else val = rs1 % rs2; break; // REMU
						}
					}
				}
				else
				{
					switch( (ir>>12)&7 )
					{
						case 0b000: val = (ir & 0x40000000 ) ? ( rs1 - rs2 ) : ( rs1 + rs2 ); break;
						case 0b001: val = rs1 << rs2; break;
						case 0b010: val = (int32_t)rs1 < (int32_t)rs2; break;
						case 0b011: val = rs1 < rs2; break;
						case 0b100: val = rs1 ^ rs2; break;
						case 0b101: val = (ir & 0x40000000 ) ? ( ((int32_t)rs1) >> rs2 ) : ( rs1 >> rs2 ); break;
						case 0b110: val = rs1 | rs2; break;
						case 0b111: val = rs1 & rs2; break;
					}
				}
				regs[rdid] = val;
				INST_DBG( "OP:%d = %08x\n", rdid, regs[rdid] );
			}
			break;
		}
		case 0b0001111:
		{
			int fencetype = (ir >> 12) & 0b111;
			break;
		}
		case 0b1110011:  // Zifencei+Zicsr
		{
			uint32_t rdid = (ir >> 7) & 0x1f;
			int rs1imm = (ir >> 15) & 0x1f;
			uint32_t rs1 = regs[rs1imm];
			uint32_t csrno = ir >> 20;

			int microop = ( ir >> 12 ) & 0b111;
			uint32_t writeval = rs1;
			int do_write = !!(microop & 3);
			uint32_t readval = 0;

			if( (microop & 3) ) // It's a Zicsr function.
			{
				// Will need to Read CSR first.								
				// https://raw.githubusercontent.com/riscv/virtual-memory/main/specs/663-Svpbmt.pdf
				// Generally, support for Zicsr
				switch( csrno )
				{
				case 0x340: readval = state->mscratch; break;
				case 0x305: readval = state->mtvec; break;
				case 0x304: readval = state->mie; break;
				case 0xC00: readval = state->cyclel; break;
				case 0x344: readval = state->mip; break;
				case 0x341: readval = state->mepc; break;
				case 0x300: readval = state->mstatus; break; //mstatus
				case 0x342: readval = state->mcause; break;
				case 0x343: readval = state->mtval; break;
				case 0x3B0: readval = 0; break; //pmpaddr0
				case 0x3a0: readval = 0; break; //pmpcfg0
				case 0xf11: readval = 0xff0ff0ff; break; //mvendorid
				case 0xf12: readval = 0x00000000; break; //marchid
				case 0xf13: readval = 0x00000000; break; //mimpid
				case 0xf14: readval = 0x00000000; break; //mhartid
				case 0x301: readval = 0x40001101; break; //misa (XLEN=32, IMA) TODO: Consider setting X bit.
				default:
					MINIRV32WARN( "READ CSR: %08x\n", csrno );
					break;
				}	

				switch( microop )
				{
					case 0b001: writeval = rs1; break;  				//CSRRW
					case 0b010: writeval = readval | rs1; break;		//CSRRS
					case 0b011: writeval = readval & ~rs1; break;		//CSRRC
					case 0b101: writeval = rs1imm; break;				//CSRRWI
					case 0b110: writeval = readval | rs1imm; break;		//CSRRSI
					case 0b111: writeval = readval & ~rs1imm; break;	//CSRRCI
				}

				if( do_write)
				{
					switch( csrno )
					{
					case 0x137: // Special, side-channel printf.
						MINIRV32WARN( "SIDE-CHANNEL-DEBUG: %s\n", state->image + writeval - 0x80000000 );
						break;
					case 0x138: // Special, side-channel printf.
						MINIRV32WARN( "SIDE-CHANNEL-DEBUG: %08x\n", writeval );
						break;
					case 0x340: state->mscratch = writeval; break;
					case 0x305: state->mtvec = writeval; break;
					case 0x304: state->mie = writeval; break;
					case 0x344: state->mip = writeval; break;
					case 0x341: state->mepc = writeval; break;
					case 0x300: state->mstatus = writeval; break; //mstatus
					case 0x342: state->mcause = writeval; break;
					case 0x343: state->mtval = writeval; break;
					case 0x3a0: break; //pmpcfg0
					case 0x3B0: break; //pmpaddr0
					case 0xf11: break; //mvendorid
					case 0xf12: break; //marchid
					case 0xf13: break; //mimpid
					case 0xf14: break; //mhartid
					case 0x301: break; //misa
					default:
						MINIRV32WARN( "WRITE CSR: %08x = %08x\n", csrno, writeval );
					}
				}
				if( rdid ) regs[rdid] = readval;
			}
			else if( microop == 0b000 )
			{
				//ECALL/EBREAK/WFI
				if( csrno == 0x105 )
					;// WFI, Ignore.
				else if( ( ( csrno & 0xff ) == 0x02 ) ) // MRET
				{
					uint32_t startmstatus = state->mstatus;
					state->mstatus = (( state->mstatus & 0x80) >> 4) | ((state->extraflags&3) << 11) | 0x80;
					state->extraflags = (state->extraflags & 0xfffffffc) | (startmstatus >> 11) & 3;

					pc = state->mepc-4;

					//https://raw.githubusercontent.com/riscv/virtual-memory/main/specs/663-Svpbmt.pdf
					//Table 7.6. MRET then in mstatus/mstatush sets MPV=0, MPP=0, MIE=MPIE, and MPIE=1. La
					// Should also update mstatus to reflect correct mode.
					rdid = 0; do_write= 0;
				}
				else //EBREAK
				{
					if( (ir >> 24) == 0xff )
					{
						printf( "Custom opcode for force exit\n" );
						exit(1);
					}
					if( csrno == 0 )
						state->mcause = 8; // 8 = "Environment call from U-mode"; 11 = "Environment call from M-mode"
					else
						state->mcause = 3; // 3 = "Breakpoint"
					rdid = 0; do_write= 0;
					state->mepc = pc; //XXX TRICKY: The kernel advances mepc
					state->mtval = pc;
					state->mstatus = (( state->mstatus & 0x08) << 4) | ((state->extraflags&3) << 11);
					state->extraflags = state->extraflags | 3;
					pc = (state->mtvec - 4);
				}
			} // Note micrrop 0b100 == undefined.
			break;
		}
		case 0b0101111: // RV32A
		{
			uint32_t rdid = (ir >> 7) & 0x1f;
			uint32_t rs1 = regs[(ir >> 15) & 0x1f];
			uint32_t rs2 = regs[(ir >> 20) & 0x1f];
			uint32_t readval = 0;
			uint32_t irmid = ( ir>>27 ) & 0x1f;

			rs1 -= ram_image_offset;

			// We don't implement load/store from UART or CLNT with RV32A here.

			if( rs1 >= ram_amt-3 )
			{
				retval = -99;
				printf( "Store OOB Access [%08x]\n", rs2 );
			}
			else
			{
				if( irmid != 0b00011 )
				{
					readval = *((uint32_t*)(image + rs1));
				}
				INST_DBG( "RV32A: %d: %08x ==> %08x\n", irmid, rs1, readval );

				// Referenced a little bit of https://github.com/franzflasch/riscv_em/blob/master/src/core/core.c
				int dowrite = 1;
				switch( irmid )
				{
					case 0b00010: dowrite = 0; break; //LR.W
					case 0b00011: readval = 0;
					
					break; //SC.W (Lie and always say it's good)
					case 0b00001: break; //AMOSWAP.W
					case 0b00000: rs2 += readval; break; //AMOADD.W
					case 0b00100: rs2 ^= readval; break; //AMOXOR.W
					case 0b01100: rs2 &= readval; break; //AMOAND.W
					case 0b01000: rs2 |= readval; break; //AMOOR.W
					case 0b10000: rs2 = ((int)rs2<(int)readval)?rs2:readval; break; //AMOMIN.W
					case 0b10100: rs2 = ((int)rs2>(int)readval)?rs2:readval; break; //AMOMAX.W
					case 0b11000: rs2 = (rs2<readval)?rs2:readval; break; //AMOMINU.W
					case 0b11100: rs2 = (rs2>readval)?rs2:readval; break; //AMOMAXU.W
					default: retval = -100; dowrite = 0; rdid = 0; break; //Not supported.
				}
				if( dowrite )
					*((uint32_t*)(image + rs1)) = rs2;
				if( rdid )
					regs[rdid] = readval;
			}

			break;
		}

		default:
		{
			retval = -1;
		}
	}

#ifndef DEBUG_INSTRUCTIONS
	if( retval )
#endif
	{
		printf( "%d %08x [%08x] Z:%08x A1:%08x %08x %08x %08x %08x %08x %08x // %08x %08x %08x %08x %08x %08x %08x %08x//x16: %08x %08x %08x %08x %08x %08x %08x %08x\n", retval, pc, ir,
			regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7],
			regs[8], regs[9], regs[10], regs[11], regs[12], regs[13], regs[14], regs[15],
			regs[16], regs[17], regs[18], regs[19], regs[20], regs[21], regs[22], regs[23] );
	}

	// Increment both wall-clock and instruction count time.
	++state->cyclel;
	if( state->cyclel == 0 ) state->cycleh++;

	if( ( state->cyclel % 1 ) == 0 )
	{
		state->timerl++;
		if( state->timerl == 0 ) state->timerh++;
	}

	if( retval < 0 )
	{
		fprintf( stderr, "Error PC: %08x / IR: %08x\n", pc, ir );
		return -1;
	}

	if( pc < 10000 )
	{
		INST_INFO( "PC WILL BE INVALID was %08x / %08x [%08x] Z:%08x A1:%08x %08x %08x %08x %08x %08x %08x // %08x %08x %08x %08x %08x %08x %08x %08x//x16: %08x %08x %08x %08x %08x %08x %08x %08x\n", state->pc, pc, ir,
						regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7],
						regs[8], regs[9], regs[10], regs[11], regs[12], regs[13], regs[14], regs[15],
						regs[16], regs[17], regs[18], regs[19], regs[20], regs[21], regs[22], regs[23] );
	}

	state->pc = pc + 4;
	return retval;
}

uint64_t SimpleReadNumberUInt( const char * number, uint64_t defaultNumber )
{
	if( !number || !number[0] ) return defaultNumber;
	int radix = 10;
	if( number[0] == '0' )
	{
		number++;
		char nc = number[0];
		if( nc == 0 ) return 0;
		else if( nc == 'x' ) radix = 16;
		else if( nc == 'b' ) radix = 2;
		else radix = 8;
	}
	char * endptr;
	uint64_t ret = strtoll( number, &endptr, radix );
	if( endptr == number )
	{
		return defaultNumber;
	}
	else
	{
		return ret;
	}	
}
