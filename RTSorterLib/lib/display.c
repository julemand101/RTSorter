#include "display.h"

void print_int(int x, int y, int value) {
	display_goto_xy(x,y);
	display_int(value, 0);
}

void print_str(int x, int y, char string[]) {
	display_goto_xy(x,y);
	display_string(string);
}

void print_clear_line(int line) {
	print_clear(line, 0, 15);
}

void print_clear(int line, int start, int length) {
	/* this method is not pretty, but it is done this way to avoid repeatedly calling
	 * display_string, and because there can be max 15 chars to clear. */
	display_goto_xy(start, line);
	switch( length )
	{
	    case 1:  display_string(" ");
	    case 2:  display_string("  ");
	    case 3:  display_string("   ");
	    case 4:  display_string("    ");
	    case 5:  display_string("     ");
	    case 6:  display_string("      ");
	    case 7:  display_string("       ");
	    case 8:  display_string("        ");
	    case 9:  display_string("         ");
	    case 10: display_string("          ");
	    case 11: display_string("           ");
	    case 12: display_string("            ");
	    case 13: display_string("             ");
	    case 14: display_string("              ");
	    case 15: display_string("               ");
	}
}

void print_clear_display() {
	display_clear(1);
}

void print_update() {
	display_update();
}
