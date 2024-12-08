#include "stdio.h"

#if defined(WINDOWS) || defined(WIN32) || defined(_WIN32)

#include <conio.h>

int ReadKBByte()
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
int IsKBHit() {
  int ret = _kbhit();
  //printf("IsKBHit %d\n", ret);
  return ret;
}
#else

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int is_eofd;
int ReadKBByte(void)
{
	if( is_eofd ) return 0xffffffff;
	char rxchar = 0;
	int rread = read(fileno(stdin), (char*)&rxchar, 1);

	if( rread > 0 ) // Tricky: getchar can't be used with arrow keys.
		return rxchar;
	else
		return -1;
}
int IsKBHit(void) {
  if( is_eofd ) return -1;
  int byteswaiting;
  ioctl(0, FIONREAD, &byteswaiting);
  if( !byteswaiting && write( fileno(stdin), 0, 0 ) != 0 ) { is_eofd = 1; return -1; } // Is end-of-file for
  return !!byteswaiting;
}
#endif
