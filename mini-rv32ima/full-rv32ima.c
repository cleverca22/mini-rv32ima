// Copyright 2022 Charles Lohr, you may use this file or any portions herein under any of the BSD, MIT, or CC0 licenses.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <libfdt.h>

#include "default64mbdtc.h"

// #define CNFGOGL
#define CNFG_IMPLEMENTATION
// #define CNFGHTTP

#include "rawdraw_sf.h"

#include "virtio.h"

// Just default RAM amount is 64MB.
uint32_t ram_amt = 64*1024*1024;
int fail_on_all_faults = 0;

static int64_t SimpleReadNumberInt( const char * number, int64_t defaultNumber );
static uint64_t GetTimeMicroseconds();
static void ResetKeyboardInput();
static void CaptureKeyboardInput();
static uint32_t HandleException( uint32_t ir, uint32_t retval );
static uint32_t HandleControlStore( uint32_t addy, uint32_t val );
static uint32_t HandleControlLoad( uint32_t addy );
static void HandleOtherCSRWrite( uint8_t * image, uint16_t csrno, uint32_t value );
static int32_t HandleOtherCSRRead( uint8_t * image, uint16_t csrno );
static void MiniSleep();
static int IsKBHit();
static int ReadKBByte();

// This is the functionality we want to override in the emulator.
//  think of this as the way the emulator's processor is connected to the outside world.
#define MINIRV32WARN( x... ) printf( x );
#define MINIRV32_DECORATE  static
#define MINI_RV32_RAM_SIZE ram_amt
#define MINIRV32_IMPLEMENTATION
#define MINIRV32_POSTEXEC( pc, ir, retval ) { if( retval > 0 ) { if( fail_on_all_faults ) { printf( "FAULT\n" ); return 3; } else retval = HandleException( ir, retval ); } }
#define MINIRV32_HANDLE_MEM_STORE_CONTROL( addy, val ) if( HandleControlStore( addy, val ) ) return val;
#define MINIRV32_HANDLE_MEM_LOAD_CONTROL( addy, rval ) rval = HandleControlLoad( addy );
#define MINIRV32_OTHERCSR_WRITE( csrno, value ) HandleOtherCSRWrite( image, csrno, value );
#define MINIRV32_OTHERCSR_READ( csrno, value ) value = HandleOtherCSRRead( image, csrno );

#include "mini-rv32ima.h"

uint8_t * ram_image = 0;
struct MiniRV32IMAState * core;
const char * kernel_command_line = 0;

bool want_exit = false;


void virtio_maybe_clear_irq() {
  // TODO, go over every virtio device and check if the condition is actually clear
  clear_hart_irq(core, 9);
}

void virtio_raise_irq() {
  raise_hart_irq(core, 9);
}

#define WEAK __attribute__((weak))

// defined as WEAK, so virtio-input can steal them, but if you comment out input, they have a fallback
void WEAK HandleKey( int keycode, int bDown ) {
  printf("HandleKey: %d %d\n", keycode, bDown);
}
void WEAK HandleButton( int x, int y, int button, int bDown ) {
  printf("HandleButton(%d, %d, %d, %d)\n", x, y, button, bDown);
}
void WEAK HandleMotion( int x, int y, int mask ) {
  //printf("HandleMotion(%d, %d, %d)\n", x, y, mask);
}
int HandleDestroy() {
  puts(__func__);
  return 0;
}

static void DumpState( struct MiniRV32IMAState * core, uint8_t * ram_image );

void *cast_guest_ptr(void *image, uint32_t addr) {
  addr -= MINIRV32_RAM_IMAGE_OFFSET;
  return image + addr;
}

int main( int argc, char ** argv )
{
	int i;
        int image_size = 0;
	long long instct = -1;
	int show_help = 0;
	int time_divisor = 1;
	int fixed_update = 0;
	int do_sleep = 1;
	int single_step = 0;
	int dtb_ptr = 0;
	const char * image_file_name = 0;
        const char *initrd_name = NULL;
	const char * dtb_file_name = 0;
        bool enable_gfx = true;
        bool enable_virtio_blk = false;
        bool enable_virtio_input = true;
        virtio_input_init();

	for( i = 1; i < argc; i++ )
	{
		const char * param = argv[i];
		int param_continue = 0; // Can combine parameters, like -lpt x
		do
		{
			if( param[0] == '-' || param_continue )
			{
				switch( param[1] )
				{
				case 'm': if( ++i < argc ) ram_amt = SimpleReadNumberInt( argv[i], ram_amt ); break;
				case 'c': if( ++i < argc ) instct = SimpleReadNumberInt( argv[i], -1 ); break;
				case 'k': if( ++i < argc ) kernel_command_line = argv[i]; break;
				case 'f': image_file_name = (++i<argc)?argv[i]:0; break;
                                case 'i': initrd_name = (++i<argc)?argv[i]:0; break;
				case 'b': dtb_file_name = (++i<argc)?argv[i]:0; break;
				case 'l': param_continue = 1; fixed_update = 1; break;
				case 'p': param_continue = 1; do_sleep = 0; break;
				case 's': param_continue = 1; single_step = 1; break;
				case 'd': param_continue = 1; fail_on_all_faults = 1; break; 
				case 't': if( ++i < argc ) time_divisor = SimpleReadNumberInt( argv[i], 1 ); break;
				default:
					if( param_continue )
						param_continue = 0;
					else
						show_help = 1;
					break;
				}
			}
			else
			{
				show_help = 1;
				break;
			}
			param++;
		} while( param_continue );
	}
	if( show_help || image_file_name == 0 || time_divisor <= 0 )
	{
		fprintf( stderr, "./mini-rv32imaf [parameters]\n\t-m [ram amount]\n\t-f [running image]\n\t-k [kernel command line]\n\t-b [dtb file, or 'disable']\n\t-c instruction count\n\t-s single step with full processor state\n\t-t time divion base\n\t-l lock time base to instruction count\n\t-p disable sleep when wfi\n\t-d fail out immediately on all faults\n" );
		return 1;
	}

        if (enable_gfx) {
          CNFGSetup("full-rv32ima", 640, 480);
        }

	ram_image = malloc( ram_amt );
	if( !ram_image )
	{
		fprintf( stderr, "Error: could not allocate system image.\n" );
		return -4;
	}

restart:
	{
		FILE * f = fopen( image_file_name, "rb" );
		if( !f || ferror( f ) )
		{
			fprintf( stderr, "Error: \"%s\" not found\n", image_file_name );
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

		memset( ram_image, 0, ram_amt );
		if( fread( ram_image, flen, 1, f ) != 1)
		{
			fprintf( stderr, "Error: Could not load image.\n" );
			return -7;
		}
		fclose( f );
                image_size = flen;

		if( dtb_file_name )
		{
			if( strcmp( dtb_file_name, "disable" ) == 0 )
			{
				// No DTB reading.
			}
			else
			{
				f = fopen( dtb_file_name, "rb" );
				if( !f || ferror( f ) )
				{
					fprintf( stderr, "Error: \"%s\" not found\n", dtb_file_name );
					return -5;
				}
				fseek( f, 0, SEEK_END );
				long dtblen = ftell( f );
				fseek( f, 0, SEEK_SET );
				dtb_ptr = ram_amt - dtblen - sizeof( struct MiniRV32IMAState );
				if( fread( ram_image + dtb_ptr, dtblen, 1, f ) != 1 )
				{
					fprintf( stderr, "Error: Could not open dtb \"%s\"\n", dtb_file_name );
					return -9;
				}
				fclose( f );
			}
		}
		else
		{
			// Load a default dtb.
#if !defined(DTB_SUPPORT)
			dtb_ptr = ram_amt - sizeof(default64mbdtb) - sizeof( struct MiniRV32IMAState );
			memcpy( ram_image + dtb_ptr, default64mbdtb, sizeof( default64mbdtb ) );
			if( kernel_command_line )
			{
				strncpy( (char*)( ram_image + dtb_ptr + 0xc0 ), kernel_command_line, 54 );
			}
#else
			dtb_ptr = ram_amt - (64 * 1024);
			memcpy(ram_image + dtb_ptr, default64mbdtb, sizeof( default64mbdtb ) );
                        void *v_fdt = ram_image + dtb_ptr;
			int ret = fdt_open_into(v_fdt, v_fdt, 32*1024);
                        if (ret) {
                          printf("ERROR: fdt_open_into() == %d\n", ret);
                          return -1;
                        }
                        uint32_t initrd_addr = 0;
                        uint32_t initrd_len = 0;
                        if (initrd_name) {
                          initrd_addr = image_size + (1024*1024);
                          initrd_addr += 0x1000 - (initrd_addr & 0xfff);
                          printf("loading %s to 0x%x\n", initrd_name, initrd_addr);
                          FILE *fh = fopen(initrd_name, "rb");
                          if( !fh || ferror( fh ) ) {
                            fprintf( stderr, "Error: \"%s\" not found\n", initrd_name);
                            return -1;
                          }
                          fseek( fh, 0, SEEK_END );
                          initrd_len = ftell(fh);
                          fseek(fh, 0, SEEK_SET);
                          if ((initrd_addr + initrd_len) > ram_amt) {
                            fprintf( stderr, "Error: Could not fit initrd image (%d bytes) into %d\n", initrd_len, ram_amt);
                            return -1;
                          }
                          if (fread(ram_image + initrd_addr, initrd_len, 1, fh) != 1) {
                            fprintf( stderr, "Error: Could not load initrd.\n" );
                            return -1;
                          }
                          fclose(fh);
                        }
                        if (enable_gfx) {
                          // TODO, adjust the top of ram in /memory below
                          int system = fdt_add_subnode(v_fdt, 0, "system");
                          fdt_setprop_string(v_fdt, system, "compatible", "simple-bus");
                          fdt_setprop_u32(v_fdt, system, "#address-cells", 1);
                          fdt_setprop_u32(v_fdt, system, "#size-cells", 1);
                          fdt_setprop(v_fdt, system, "ranges", NULL, 0);

                          // FB_SIMPLE=y in linux
                          int fb = fdt_add_subnode(v_fdt, system, "framebuffer");
                          fdt_setprop_string(v_fdt, fb, "compatible", "simple-framebuffer");
                          fdt_appendprop_addrrange(v_fdt, system, fb, "reg", 0x80000000 + (32 * 1024 * 1024), 640 * 480 * 4);
                          fdt_setprop_u32(v_fdt, fb, "width", 640);
                          fdt_setprop_u32(v_fdt, fb, "height", 480);
                          fdt_setprop_u32(v_fdt, fb, "stride", 640*4);
                          fdt_setprop_string(v_fdt, fb, "format", "a8r8g8b8");
                        }
                        if (enable_virtio_blk) {
                          struct virtio_device *virtio_dev = virtio_create(ram_image, &virtio_blk_type, 0x10010000, 0x200);
                          int soc = fdt_path_offset(v_fdt, "/soc");
                          int virtio = fdt_add_subnode(v_fdt, soc, "virtio_blk");
                          fdt_appendprop_addrrange(v_fdt, soc, virtio, "reg", virtio_dev->reg_base, virtio_dev->reg_size);
                          fdt_setprop_u32(v_fdt, virtio, "interrupts", 9);
                          fdt_setprop_string(v_fdt, virtio, "status", "okay");
                          fdt_setprop_string(v_fdt, virtio, "compatible", "virtio,mmio");
                        }
                        if (enable_virtio_input) {
                          struct virtio_device *virtio_dev = virtio_create(ram_image, &virtio_input_type, 0x10020000, 0x200);
                          int soc = fdt_path_offset(v_fdt, "/soc");
                          int virtio = fdt_add_subnode(v_fdt, soc, "virtio_input");
                          fdt_appendprop_addrrange(v_fdt, soc, virtio, "reg", virtio_dev->reg_base, virtio_dev->reg_size);
                          fdt_setprop_u32(v_fdt, virtio, "interrupts", 9);
                          fdt_setprop_string(v_fdt, virtio, "status", "okay");
                          fdt_setprop_string(v_fdt, virtio, "compatible", "virtio,mmio");
                        }
                        int chosen = fdt_path_offset(v_fdt, "/chosen");
                        if (chosen < 0) {
                          puts("ERROR: no chosen node in fdt");
                          return -1;
                        } else {
                          if (kernel_command_line) {
                            fdt_setprop_string(v_fdt, chosen, "bootargs", kernel_command_line);
                          }
                          if (initrd_len > 0) {
                            uint32_t start = 0x80000000 + initrd_addr;
                            uint32_t end = start + initrd_len;
                            fdt_setprop_u32(v_fdt, chosen, "linux,initrd-start", start);
                            fdt_setprop_u32(v_fdt, chosen, "linux,initrd-end", end);
                          }
                        }
                        int memory = fdt_path_offset(v_fdt, "/memory@80000000");
                        if (memory > 0) {
                          fdt_setprop(v_fdt, memory, "reg", NULL, 0);
                          if (enable_gfx) {
                            fdt_appendprop_addrrange(v_fdt, 0, memory, "reg", 0x80000000, 32 * 1024 * 1024);
                          } else {
                            fdt_appendprop_addrrange(v_fdt, 0, memory, "reg", 0x80000000, ram_amt - (16*1024));
                          }
                        }
#endif
		}
	}

	CaptureKeyboardInput();

	// The core lives at the end of RAM.
	core = (struct MiniRV32IMAState *)(ram_image + ram_amt - sizeof( struct MiniRV32IMAState ));
	core->pc = MINIRV32_RAM_IMAGE_OFFSET;
	core->regs[10] = 0x00; //hart ID
	core->regs[11] = dtb_ptr?(dtb_ptr+MINIRV32_RAM_IMAGE_OFFSET):0; //dtb_pa (Must be valid pointer) (Should be pointer to dtb)
	core->extraflags |= 3; // Machine-mode.

	if( dtb_file_name == 0 )
	{
		// Update system ram size in DTB (but if and only if we're using the default DTB)
		// Warning - this will need to be updated if the skeleton DTB is ever modified.
		uint32_t * dtb = (uint32_t*)(ram_image + dtb_ptr);
		if( dtb[0x13c/4] == 0x00c0ff03 )
		{
			uint32_t validram = dtb_ptr;
			dtb[0x13c/4] = (validram>>24) | ((( validram >> 16 ) & 0xff) << 8 ) | (((validram>>8) & 0xff ) << 16 ) | ( ( validram & 0xff) << 24 );
		}
	}

	// Image is loaded.
	uint64_t rt;
	uint64_t lastTime = (fixed_update)?0:(GetTimeMicroseconds()/time_divisor);
	int instrs_per_flip = single_step?1:1024;
        uint64_t lastflip = GetTimeMicroseconds();
	for( rt = 0; rt < instct+1 || instct < 0; rt += instrs_per_flip )
	{
		uint64_t * this_ccount = ((uint64_t*)&core->cyclel);
		uint32_t elapsedUs = 0;
		if( fixed_update )
			elapsedUs = *this_ccount / time_divisor - lastTime;
		else
			elapsedUs = GetTimeMicroseconds()/time_divisor - lastTime;
		lastTime += elapsedUs;

		if( single_step )
			DumpState( core, ram_image);

		int ret = MiniRV32IMAStep( core, ram_image, 0, elapsedUs, instrs_per_flip ); // Execute upto 1024 cycles before breaking out.
		switch( ret )
		{
			case 0: break;
			case 1: if( do_sleep ) MiniSleep(); *this_ccount += instrs_per_flip; break;
			case 3: instct = 0; break;
			case 0x7777: goto restart;	//syscon code for restart
			case 0x5555: printf( "POWEROFF@0x%08x%08x\n", core->cycleh, core->cyclel ); return 0; //syscon code for power-off
			default: printf( "Unknown failure\n" ); break;
		}
                if (enable_gfx) {
                  uint64_t now = GetTimeMicroseconds();
                  uint64_t sinceflip = now - lastflip;
#ifdef CNFGHTTP
                  if (sinceflip > 1000000) {
#else
                  if (sinceflip > 16666) {
#endif
                    uint64_t start = GetTimeMicroseconds();
                    CNFGBlitImage((uint32_t*)(ram_image + (32*1024*1024)), 0, 0, 640, 480);
                    CNFGSwapBuffers();
                    CNFGHandleInput();
                    uint64_t end = GetTimeMicroseconds();
#if 0
                    uint64_t spent = end - start;
                    printf("spent %ld\n", spent);
#endif
                    lastflip = end;
                  }
                }
                if (want_exit) break;
	}
        // virtio_dump_all();

	DumpState( core, ram_image);

        return 0;
}


//////////////////////////////////////////////////////////////////////////
// Platform-specific functionality
//////////////////////////////////////////////////////////////////////////


#if defined(WINDOWS) || defined(WIN32) || defined(_WIN32)

#include <windows.h>
#include <conio.h>

#define strtoll _strtoi64

static void CaptureKeyboardInput()
{
	system(""); // Poorly documented tick: Enable VT100 Windows mode.
}

static void ResetKeyboardInput()
{
}

static void MiniSleep()
{
	Sleep(1);
}

static uint64_t GetTimeMicroseconds()
{
	static LARGE_INTEGER lpf;
	LARGE_INTEGER li;

	if( !lpf.QuadPart )
		QueryPerformanceFrequency( &lpf );

	QueryPerformanceCounter( &li );
	return ((uint64_t)li.QuadPart * 1000000LL) / (uint64_t)lpf.QuadPart;
}


static int IsKBHit()
{
	return _kbhit();
}

static int ReadKBByte()
{
	// This code is kind of tricky, but used to convert windows arrow keys
	// to VT100 arrow keys.
	static int is_escape_sequence = 0;
	int r;
	if( is_escape_sequence == 1 )
	{
		is_escape_sequence++;
		return '[';
	}

	r = _getch();

	if( is_escape_sequence )
	{
		is_escape_sequence = 0;
		switch( r )
		{
			case 'H': return 'A'; // Up
			case 'P': return 'B'; // Down
			case 'K': return 'D'; // Left
			case 'M': return 'C'; // Right
			case 'G': return 'H'; // Home
			case 'O': return 'F'; // End
			default: return r; // Unknown code.
		}
	}
	else
	{
		switch( r )
		{
			case 13: return 10; //cr->lf
			case 224: is_escape_sequence = 1; return 27; // Escape arrow keys
			default: return r;
		}
	}
}

#else

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

static void CtrlC()
{
  want_exit = true;
}

// Override keyboard, so we can capture all keyboard input for the VM.
static void CaptureKeyboardInput()
{
	// Hook exit, because we want to re-enable keyboard.
	atexit(ResetKeyboardInput);
	signal(SIGINT, CtrlC);

	struct termios term;
	tcgetattr(0, &term);
	term.c_lflag &= ~(ICANON | ECHO); // Disable echo as well
	tcsetattr(0, TCSANOW, &term);
}

static void ResetKeyboardInput()
{
	// Re-enable echo, etc. on keyboard.
	struct termios term;
	tcgetattr(0, &term);
	term.c_lflag |= ICANON | ECHO;
	tcsetattr(0, TCSANOW, &term);
}

static void MiniSleep()
{
	usleep(500);
}

static uint64_t GetTimeMicroseconds()
{
	struct timeval tv;
	gettimeofday( &tv, 0 );
	return tv.tv_usec + ((uint64_t)(tv.tv_sec)) * 1000000LL;
}

static int is_eofd;

static int ReadKBByte()
{
	if( is_eofd ) return 0xffffffff;
	char rxchar = 0;
	int rread = read(fileno(stdin), (char*)&rxchar, 1);

	if( rread > 0 ) // Tricky: getchar can't be used with arrow keys.
		return rxchar;
	else
		return -1;
}

static int IsKBHit()
{
	if( is_eofd ) return -1;
	int byteswaiting;
	ioctl(0, FIONREAD, &byteswaiting);
	if( !byteswaiting && write( fileno(stdin), 0, 0 ) != 0 ) { is_eofd = 1; return -1; } // Is end-of-file for 
	return !!byteswaiting;
}


#endif


//////////////////////////////////////////////////////////////////////////
// Rest of functions functionality
//////////////////////////////////////////////////////////////////////////

static uint32_t HandleException( uint32_t ir, uint32_t code )
{
	// Weird opcode emitted by duktape on exit.
	if( code == 3 )
	{
		// Could handle other opcodes here.
	}
	return code;
}

static uint32_t HandleControlStore( uint32_t addy, uint32_t val )
{
  if( addy == 0x10000000 ) //UART 8250 / 16550 Data Buffer
  {
    printf("%c", val);
    fflush( stdout );
  }
  else if (addy == 0x10000001) {}
  else if (addy == 0x10000002) {}
  else if (addy == 0x10000003) {}
  else if (addy == 0x10000004) {}
  else {
    bool handled = false;
    handled = virtio_store(addy, val);
    if (!handled) printf("HandleControlStore(0x%x, 0x%x)\n", addy, val);
  }
	return 0;
}


static uint32_t HandleControlLoad( uint32_t addy )
{
  uint32_t ret = 0;
	// Emulating a 8250 / 16550 UART
	if( addy == 0x10000005 )
		ret = 0x60 | IsKBHit();
	else if( addy == 0x10000000 && IsKBHit() )
		ret = ReadKBByte();
        else if (addy == 0x10000001) {}
        else if (addy == 0x10000002) {}
        else if (addy == 0x10000006) {}
  else {
    ret = virtio_load(addy);
    //printf("HandleControlLoad(0x%x) == 0x%x\n", addy, ret);
  }
	return ret;
}

static void HandleOtherCSRWrite( uint8_t * image, uint16_t csrno, uint32_t value )
{
	if( csrno == 0x136 )
	{
		printf( "%d", value ); fflush( stdout );
	}
	if( csrno == 0x137 )
	{
		printf( "%08x", value ); fflush( stdout );
	}
	else if( csrno == 0x138 )
	{
		//Print "string"
		uint32_t ptrstart = value - MINIRV32_RAM_IMAGE_OFFSET;
		uint32_t ptrend = ptrstart;
		if( ptrstart >= ram_amt )
			printf( "DEBUG PASSED INVALID PTR (%08x)\n", value );
		while( ptrend < ram_amt )
		{
			if( image[ptrend] == 0 ) break;
			ptrend++;
		}
		if( ptrend != ptrstart )
			fwrite( image + ptrstart, ptrend - ptrstart, 1, stdout );
	}
	else if( csrno == 0x139 )
	{
		putchar( value ); fflush( stdout );
	}
}

static int32_t HandleOtherCSRRead( uint8_t * image, uint16_t csrno )
{
	if( csrno == 0x140 )
	{
		if( !IsKBHit() ) return -1;
		return ReadKBByte();
	}
	return 0;
}

static int64_t SimpleReadNumberInt( const char * number, int64_t defaultNumber )
{
	if( !number || !number[0] ) return defaultNumber;
	int radix = 10;
	if( number[0] == '0' )
	{
		char nc = number[1];
		number+=2;
		if( nc == 0 ) return 0;
		else if( nc == 'x' ) radix = 16;
		else if( nc == 'b' ) radix = 2;
		else { number--; radix = 8; }
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

static void DumpState( struct MiniRV32IMAState * core, uint8_t * ram_image )
{
	uint32_t pc = core->pc;
	uint32_t pc_offset = pc - MINIRV32_RAM_IMAGE_OFFSET;
	uint32_t ir = 0;

	printf( "PC: %08x ", pc );
	if( pc_offset >= 0 && pc_offset < ram_amt - 3 )
	{
		ir = *((uint32_t*)(&((uint8_t*)ram_image)[pc_offset]));
		printf( "[0x%08x] ", ir );
	}
	else
		printf( "[xxxxxxxxxx] " );
        printf("MIP:%08x MIE:%08x ", core->mip, core->mie);
	uint32_t * regs = core->regs;
	printf( "Z:%08x ra:%08x sp:%08x gp:%08x tp:%08x t0:%08x t1:%08x t2:%08x s0:%08x s1:%08x a0:%08x a1:%08x a2:%08x a3:%08x a4:%08x a5:%08x ",
		regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7],
		regs[8], regs[9], regs[10], regs[11], regs[12], regs[13], regs[14], regs[15] );
	printf( "a6:%08x a7:%08x s2:%08x s3:%08x s4:%08x s5:%08x s6:%08x s7:%08x s8:%08x s9:%08x s10:%08x s11:%08x t3:%08x t4:%08x t5:%08x t6:%08x\n",
		regs[16], regs[17], regs[18], regs[19], regs[20], regs[21], regs[22], regs[23],
		regs[24], regs[25], regs[26], regs[27], regs[28], regs[29], regs[30], regs[31] );
}

