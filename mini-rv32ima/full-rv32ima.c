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

#include "rawdraw_sf.h"

#include "virtio.h"
#include "plic.h"
#include "mmio.h"
#include "pl011.h"
#include "stdio.h"

#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))

// Just default RAM amount is 64MB.
uint32_t ram_amt = 64*1024*1024;
const int w = 640;
const int h = 480;
const int bpp = 4;
int fail_on_all_faults = 0;

static int64_t SimpleReadNumberInt( const char * number, int64_t defaultNumber );
static uint64_t GetTimeMicroseconds(void);
static void ResetKeyboardInput(void);
static void CaptureKeyboardInput(void);
static uint32_t HandleException( uint32_t ir, uint32_t retval );
static uint32_t HandleControlStore( uint32_t addy, uint32_t val );
static uint32_t HandleControlLoad( uint32_t addy );
static void HandleOtherCSRWrite( uint8_t * image, uint16_t csrno, uint32_t value );
static int32_t HandleOtherCSRRead( uint8_t * image, uint16_t csrno );
static void MiniSleep(void);
static uint32_t uart_load(void *state, uint32_t addr);
static void uart_store(void *state, uint32_t addr, uint32_t val);

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

bool want_exit = false;

void hart_raise_irq(int irq) {
  raise_hart_irq(core, irq);
}

void hart_clear_irq(int irq) {
  clear_hart_irq(core, irq);
}

#define WEAK __attribute__((weak))

// defined as WEAK, so virtio-input can steal them, but if you comment out input, they have a fallback

#ifndef VIRTIO_WEAK_HACK
void WEAK HandleKey( int keycode, int bDown ) {
  printf("HandleKey: %d %d\n", keycode, bDown);
}
void WEAK HandleMotion( int x, int y, int mask ) {
  //printf("HandleMotion(%d, %d, %d)\n", x, y, mask);
}
#endif
void WEAK HandleButton( int x, int y, int button, int bDown ) {
  printf("HandleButton(%d, %d, %d, %d)\n", x, y, button, bDown);
}
int HandleDestroy(void) {
  puts(__func__);
  return 0;
}

static void DumpState( struct MiniRV32IMAState * core, uint8_t * ram_image );

void *cast_guest_ptr(void *image, uint32_t addr) {
  addr -= MINIRV32_RAM_IMAGE_OFFSET;
  return image + addr;
}

uint32_t next_guest_alloc = MINIRV32_RAM_IMAGE_OFFSET;
uint32_t next_guest_top_alloc = 0;

uint32_t guest_alloc(uint32_t size, const char *name) {
  uint32_t ret = next_guest_alloc;
  uint32_t size_2 = ROUNDUP(size, 4096);
  next_guest_alloc += size_2;
  printf("ALLOC: 0x%08x + 0x%08x == %s\n", ret, size, name);
  return ret;
}

uint32_t guest_top_alloc(uint32_t size, const char *name) {
  uint32_t size_2 = ROUNDUP(size, 4096);
  uint32_t ret = next_guest_top_alloc - size_2;
  next_guest_top_alloc -= size_2;
  next_guest_top_alloc &= ~0xfff;
  ret &= ~0xfff;
  printf("ALLOC: 0x%08x + 0x%08x == %s\n", ret, size, name);
  return ret;
}

void load_kernel(const char *image_file_name) {
  FILE * f = fopen( image_file_name, "rb" );
  if( !f || ferror( f ) ) {
    fprintf( stderr, "Error: \"%s\" not found\n", image_file_name );
    exit(-5);
  }
  fseek( f, 0, SEEK_END );
  long flen = ftell( f );
  fseek( f, 0, SEEK_SET );
  if( flen > ram_amt ) {
    fprintf( stderr, "Error: Could not fit RAM image (%ld bytes) into %d\n", flen, ram_amt );
    exit(-6);
  }
  uint32_t kernel_base = guest_alloc(flen + (512*1024), "KERNEL");
  void *kernel_ptr = cast_guest_ptr(ram_image, kernel_base);
  if( fread(kernel_ptr, flen, 1, f ) != 1) {
    fprintf( stderr, "Error: Could not load image.\n" );
    exit(-7);
  }
  fclose( f );
}

void load_initrd(const char *initrd_name, uint32_t *addr, uint32_t *size) {
  FILE *fh = fopen(initrd_name, "rb");
  if( !fh || ferror( fh ) ) {
    fprintf( stderr, "Error: \"%s\" not found\n", initrd_name);
    exit(-1);
  }
  fseek( fh, 0, SEEK_END );
  *size = ftell(fh);
  fseek(fh, 0, SEEK_SET);
  *addr = guest_alloc(*size, "INITRD");
  void *initrd_host_ptr = cast_guest_ptr(ram_image, *addr);
  printf("loading %s to 0x%x\n", initrd_name, *addr);
  printf("MEM: 0x%08x -> 0x%08x == INITRD\n", 0x80000000 + *addr, 0x80000000 + *addr + *size - 1);
  if (fread(initrd_host_ptr, *size, 1, fh) != 1) {
    fprintf( stderr, "Error: Could not load initrd.\n" );
    exit(-1);
  }
  fclose(fh);
}

void load_dtb(const char *dtb_name, uint32_t *dtb_addr) {
  uint32_t dtb_ptr;
  if (dtb_name) {
    FILE *f = fopen( dtb_name, "rb" );
    if( !f || ferror( f ) ) {
      fprintf( stderr, "Error: \"%s\" not found\n", dtb_name );
      exit(-5);
    }
    fseek( f, 0, SEEK_END );
    long dtblen = ftell( f );
    fseek( f, 0, SEEK_SET );
    dtb_ptr = guest_alloc(64 * 1024, "DTB");

    printf("MEM: 0x%08x -> 0x%08x == INITRD\n", (uint32_t)(0x80000000 + dtb_ptr), (uint32_t)(0x80000000 + dtb_ptr + dtblen - 1));
    if( fread( ram_image + dtb_ptr, dtblen, 1, f ) != 1 ) {
      fprintf( stderr, "Error: Could not open dtb \"%s\"\n", dtb_name );
      exit(-9);
    }
    fclose( f );
  } else {
    // Load a default dtb.
    dtb_ptr = guest_alloc(64 * 1024, "DTB");
    void *host_dtb_ptr = cast_guest_ptr(ram_image, dtb_ptr);
    memcpy(host_dtb_ptr, default64mbdtb, sizeof( default64mbdtb ) );
  }
  *dtb_addr = dtb_ptr;
}

void patch_dtb_gfx(void *v_fdt, void **fb_virt_ptr) {
  const int bytes_per_frame = w * h * bpp;
  const int size_rounded_up = ROUNDUP(bytes_per_frame,4096);

  uint32_t guest_ptr = guest_top_alloc(size_rounded_up, "FB");

  int system = fdt_add_subnode(v_fdt, 0, "system");
  fdt_setprop_string(v_fdt, system, "compatible", "simple-bus");
  fdt_setprop_u32(v_fdt, system, "#address-cells", 1);
  fdt_setprop_u32(v_fdt, system, "#size-cells", 1);
  fdt_setprop(v_fdt, system, "ranges", NULL, 0);

  // FB_SIMPLE=y in linux
  int fb = fdt_add_subnode(v_fdt, system, "framebuffer");
  fdt_setprop_string(v_fdt, fb, "compatible", "simple-framebuffer");
  fdt_appendprop_addrrange(v_fdt, system, fb, "reg", guest_ptr, bytes_per_frame);
  fdt_setprop_u32(v_fdt, fb, "width", w);
  fdt_setprop_u32(v_fdt, fb, "height", h);
  fdt_setprop_u32(v_fdt, fb, "stride", w*bpp);
  fdt_setprop_string(v_fdt, fb, "format", "a8r8g8b8");
  *fb_virt_ptr = cast_guest_ptr(ram_image, guest_ptr);
}

void patch_dtb(uint32_t dtb_ptr, bool enable_gfx, void **fb_virt_ptr, bool enable_virtio_blk, bool enable_virtio_input, const char *kernel_command_line, uint32_t initrd_addr, uint32_t initrd_len) {
  void *v_fdt = cast_guest_ptr(ram_image, dtb_ptr);
  int ret = fdt_open_into(v_fdt, v_fdt, 64*1024);
  if (ret) {
    printf("ERROR: fdt_open_into() == %d\n", ret);
    exit(-1);
  }
  {
    uint32_t uart_base = get_next_base(0x100);
    pl011_create(v_fdt, uart_base);
  }
  if (enable_gfx) patch_dtb_gfx(v_fdt, fb_virt_ptr);
  {
    int soc = fdt_path_offset(v_fdt, "/soc");
    int plic = fdt_add_subnode(v_fdt, soc, "plic");
    fdt_setprop_string(v_fdt, plic, "status", "okay");
    mmio_add_handler(0x10400000, 0x4000000, plic_load, plic_store, NULL, "PLIC");
  }
#ifdef WITH_BLOCK
  if (enable_virtio_blk) {
    uint32_t base = get_next_base(0x1000);
    struct virtio_device *virtio_blk = virtio_blk_create(ram_image, base);
    virtio_add_dtb(virtio_blk, v_fdt);
    mmio_add_handler(virtio_blk->reg_base, virtio_blk->reg_size, virtio_mmio_load, virtio_mmio_store, virtio_blk, "virtio-blk");
  }
#endif
#ifdef WITH_INPUT
  if (enable_virtio_input) {
    uint32_t base = get_next_base(0x1000);
    struct virtio_device *virtio_input_keyb = virtio_input_create(ram_image, base, false);
    virtio_add_dtb(virtio_input_keyb, v_fdt);
    mmio_add_handler(virtio_input_keyb->reg_base, virtio_input_keyb->reg_size, virtio_mmio_load, virtio_mmio_store, virtio_input_keyb, "virtio-input keyboard");

    base = get_next_base(0x1000);
    struct virtio_device *virtio_input_mouse = virtio_input_create(ram_image, base, true);
    virtio_add_dtb(virtio_input_mouse, v_fdt);
    mmio_add_handler(virtio_input_mouse->reg_base, virtio_input_mouse->reg_size, virtio_mmio_load, virtio_mmio_store, virtio_input_mouse, "virtio-input mouse");
  }
#endif
#ifdef WITH_SND
  {
    uint32_t base = get_next_base(0x1000);
    struct virtio_device *virtio_snd = virtio_snd_create(ram_image, base);
    virtio_add_dtb(virtio_snd, v_fdt);
    mmio_add_handler(virtio_snd->reg_base, virtio_snd->reg_size, virtio_mmio_load, virtio_mmio_store, virtio_snd, "virtio-snd");
  }
#endif
  {
    uint32_t base = get_next_base(0x1000);
    struct virtio_device *virtio_net = virtio_net_create(ram_image, base);
    if (virtio_net) {
      virtio_add_dtb(virtio_net, v_fdt);
      mmio_add_handler(virtio_net->reg_base, virtio_net->reg_size, virtio_mmio_load, virtio_mmio_store, virtio_net, "virtio-net");
    } else {
      fputs("error creating network device", stderr);
    }
  }

  int chosen = fdt_path_offset(v_fdt, "/chosen");
  if (chosen < 0) {
    puts("ERROR: no chosen node in fdt");
    exit(-1);
  } else {
    if (kernel_command_line) {
      fdt_setprop_string(v_fdt, chosen, "bootargs", kernel_command_line);
    }
    if (initrd_len > 0) {
      uint32_t start = initrd_addr;
      uint32_t end = start + initrd_len;
      fdt_setprop_u32(v_fdt, chosen, "linux,initrd-start", start);
      fdt_setprop_u32(v_fdt, chosen, "linux,initrd-end", end);
    }
  }
  int memory = fdt_path_offset(v_fdt, "/memory@80000000");
  if (memory > 0) {
    fdt_setprop(v_fdt, memory, "reg", NULL, 0);
    uint32_t ram_start = MINIRV32_RAM_IMAGE_OFFSET;
    uint32_t ram_size = next_guest_top_alloc - MINIRV32_RAM_IMAGE_OFFSET;
    printf("setting ram to 0x%08x + 0x%08x\n", ram_start, ram_size);
    fdt_appendprop_addrrange(v_fdt, 0, memory, "reg", ram_start, ram_size);
  }
}

int main( int argc, char ** argv ) {
  int i;
  long long instct = -1;
  int show_help = 0;
  int time_divisor = 1;
  int fixed_update = 0;
  int do_sleep = 1;
  int single_step = 0;
  uint32_t dtb_ptr = 0;
  const char * image_file_name = 0;
  const char *initrd_name = NULL;
  const char * dtb_file_name = 0;
  const bool enable_gfx = true;
  const bool enable_virtio_blk = false;
  const bool enable_virtio_input = true;
  const char * kernel_command_line = "earlycon=pl011,0x10000000 console=tty1 console=ttyAMA0 no_hash_pointers";
  void *fb_virt_ptr = NULL;

  for( i = 1; i < argc; i++ ) {
    const char * param = argv[i];
    int param_continue = 0; // Can combine parameters, like -lpt x
    do {
      if( param[0] == '-' || param_continue ) {
        switch( param[1] ) {
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
      } else {
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
  printf("MEM: 0x%08x -> 0x%08x == RAM\n", 0x80000000, (0x80000000 + ram_amt)-1);
  if( !ram_image ) {
    fprintf( stderr, "Error: could not allocate system image.\n" );
    return -4;
  }

restart:
  next_guest_alloc = MINIRV32_RAM_IMAGE_OFFSET;
  next_guest_top_alloc = MINIRV32_RAM_IMAGE_OFFSET + ram_amt;

  // The core lives at the end of RAM.
  uint32_t core_guest_ptr = guest_top_alloc(sizeof(struct MiniRV32IMAState), "CORE");
  core = cast_guest_ptr(ram_image, core_guest_ptr);
  {
    memset( ram_image, 0, ram_amt );
    load_kernel(image_file_name);

    if( dtb_file_name && (strcmp( dtb_file_name, "disable" ) == 0) ) {
      // No DTB reading.
      if (initrd_name) {
        fprintf(stderr, "unable to load initrd without a dtb\n");
        return -1;
      }
    } else {
      uint32_t initrd_addr = 0;
      uint32_t initrd_len = 0;
      if (initrd_name) {
        load_initrd(initrd_name, &initrd_addr, &initrd_len);
      }
      load_dtb(dtb_file_name, &dtb_ptr);
      patch_dtb(dtb_ptr, enable_gfx, &fb_virt_ptr, enable_virtio_blk, enable_virtio_input, kernel_command_line, initrd_addr, initrd_len);
    }

  }

  //mmio_add_handler(0x10000000, 0x100, uart_load, uart_store, NULL, "legacy uart");
	CaptureKeyboardInput();

	core->pc = MINIRV32_RAM_IMAGE_OFFSET;
	core->regs[10] = 0x00; //hart ID
	core->regs[11] = dtb_ptr?(dtb_ptr):0; //dtb_pa (Must be valid pointer) (Should be pointer to dtb)
	core->extraflags |= 3; // Machine-mode.

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
			case 1:
                          if( do_sleep ) {
                            MiniSleep();
                          }
                          *this_ccount += instrs_per_flip;
                          break;
			case 3: instct = 0; break;
			case 0x7777: goto restart;	//syscon code for restart
			case 0x5555: printf( "POWEROFF@0x%08x%08x\n", core->cycleh, core->cyclel ); return 0; //syscon code for power-off
			default: printf( "Unknown failure\n" ); break;
		}
                if (enable_gfx) {
                  uint64_t now = GetTimeMicroseconds();
                  uint64_t sinceflip = now - lastflip;
#ifdef CNFGHTTP
                  if (sinceflip > 16666) {
#else
                  if (sinceflip > 16666) {
#endif
                    uint64_t start = GetTimeMicroseconds();
                    CNFGBGColor = 0;
                    CNFGClearFrame();
                    CNFGBlitImage((uint32_t*)fb_virt_ptr, 0, 0, 640, 480);
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
                pl011_poll();
                if (want_exit) break;
	}
        // virtio_dump_all();

	DumpState( core, ram_image);
  free(ram_image);

  if (want_exit) return 1;

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




#else

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

static void CtrlC(int)
{
  want_exit = true;
}

// Override keyboard, so we can capture all keyboard input for the VM.
static void CaptureKeyboardInput(void)
{
	// Hook exit, because we want to re-enable keyboard.
	atexit(ResetKeyboardInput);
	signal(SIGINT, CtrlC);

	struct termios term;
	tcgetattr(0, &term);
	term.c_lflag &= ~(ICANON | ECHO); // Disable echo as well
	tcsetattr(0, TCSANOW, &term);
}

static void ResetKeyboardInput(void)
{
	// Re-enable echo, etc. on keyboard.
	struct termios term;
	tcgetattr(0, &term);
	term.c_lflag |= ICANON | ECHO;
	tcsetattr(0, TCSANOW, &term);
}

static void MiniSleep(void)
{
	usleep(500);
}

static uint64_t GetTimeMicroseconds(void)
{
	struct timeval tv;
	gettimeofday( &tv, 0 );
	return tv.tv_usec + ((uint64_t)(tv.tv_sec)) * 1000000LL;
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

// Emulating a 8250 / 16550 UART
static uint32_t uart_load(void *state, uint32_t addr) {
  uint32_t ret = 0;
  switch (addr) {
  case 0:
    if (IsKBHit()) ret = ReadKBByte();
    break;
  case 5:
    ret = 0x60 | IsKBHit();
    break;
  }
  return ret;
}

static void uart_store(void *state, uint32_t addr, uint32_t val) {
  switch (addr) {
  case 0: //UART 8250 / 16550 Data Buffer
    printf("%c", val);
    fflush( stdout );
    break;
  }
}

static uint32_t HandleControlStore( uint32_t addy, uint32_t val )
{
  mmio_routed_store(addy, val);
  //printf("HandleControlStore(0x%x, 0x%x)\n", addy, val);
  return 0;
}

static uint32_t HandleControlLoad( uint32_t addy ) {
  uint32_t ret = 0;
  ret = mmio_routed_load(addy);
  //printf("HandleControlLoad(0x%x) == 0x%x\n", addy, ret);
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

