/*
  simple compiler from bf to ld-only instructions in the sm83 processor

  hl is the bf pointer.

  currently it's ok for me to cheat with ldi/ldd :)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    if (argc < 2) {
	fprintf(stderr, "usage: %s file.bf\n", argv[0]);
	exit(1);
    }
    FILE* in = fopen(argv[1], "r");

    FILE* out;
    {
	strtok(argv[1], "."); // remove the extension hehe
	char* out_name = malloc(6+strlen(argv[1]));
	strcpy(out_name, argv[1]);
	strcat(out_name, ".sm83");
	out = fopen(out_name, "w");
    }
    // define required macros
    fprintf(out,
	    "cond_assign: MACRO ; cond_assign var, cond, t, f is like "
	        "var=cond?t:f. cond: bool, var=constant address, "
	        "t,f: 8bit values. cond,t,f must be (const|[bce])\n"
	    "    ld a, \\4\n"
	    "    ld [temp_0], a\n"
	    "    ld a, \\3\n"
	    "    ld [temp_1], a\n"
	    "    ld e, \\2\n"
	    "    ld d, HIGH(temp_0) ; [de] = cond?t:f\n"
	    "    ld a, [de] ; a = cond?t:f\n"
	    "    ld de, \\1 ; [de] = var\n"
	    "    ld [de], a ; var = cond?t:f\n"
	    "ENDM\n");
    fprintf(out,
	    "equals: MACRO ; equals op1, op2 is like b = op1==op2. "
	    "op1, op2 are 8bit values - must be (constant|[bce]), "
	    "GARBAGE_ZONE must be all 0s before calling this macro.\n"
	    "    ld d, HIGH(GARBAGE_ZONE)\n"
	    "    ld e, \\1\n"
	    "    ld a, 1\n"
	    "    ld [de], a\n"
	    "    ld e, \\2\n ; [de] == 1 iff op1==op2\n"
	    "    ld a, [de]\n"
	    "    ld b, a\n ; b == 1 iff op1==op2. now time for cleanup.\n"
	    "    ld e, \\1\n ; cleanup. [GARBAGE_ZONE+op1] = 0."
	    "    ld a, 0\n"
	    "    ld [de], a\n"
	    "ENDM\n"
	);
    fprintf(out,
	    "check_active_block: MACRO ; \\1 = current block. \n"
	    /// if @ == active_block:
	    ///     exec = on
	    "    clean_garbage_zone ; now the garbage zone is all 0s\n"
	    "    ldh a, [active_block]\n"
	    "    ld c, a\n"
	    "    equals c, LOW(\\1)\n"
	    "    cond_assign hram_temp+0, b, 1, 0\n"
	    "    ldh a, [active_block]\n"
	    "#############ARF#A #ARC#ARXARC#
);
    // initializing shit
    fprintf(out,
	    "SECTION \"Header\", ROM0[$100]\n\n"
	    "EntryPoint:\n"
	    "    DI\n"
	    "    jp Start\n\n"
	    "REPT $150 - $104\n"
	    "    DB $00\n"
	    "ENDR\n\n");
    fprintf(out,
	    "SECTION \"Garbage\", WRAM0[$c000]\n\n"
	    "GARBAGE_ZONE:\n"
	    "temp_0:\n" // those temp vars are aligned
	    "    ds 1\n"
	    "temp_1:\n"
	    "    ds 1\n"
	    "    ds $1000-$2\n");
    fprintf(out,
	    "SECTION \"Array\", WRAM0[$d000]\n\n"
	    "Array:\n"
	    "    ds $1000\n");
    fprintf(out,
	    "SECTION \"internal data\", HRAM[$ff80]\n\n"
	    "exec:\n"
	    "    ds 1\n"
	    "active_block:\n"
	    "    ds 2\n"
	    "hram_temp:\n"
	    "    ds 4\n"
	    "");

    fprintf(out,
	    "SECTION \"Game code\", ROM0[$150]\n\n"
	    "Start:\n"
	    "    ld sp,$FFF0\n");

    long branch_counter = 0;
    int c;
    for (c = fgetc(in); !feof(in); c = fgetc(in)) {
	switch(c) {
	case '+': case '-':
	    fprintf(out,
		    "ld d, h\n"
		    "ld e, l\n"

		    "ld l, [hl]\n"
		    "ld h, 0\n"
		    "ld a, [hl%c]\n"
		    "ld a, l\n"

		    "ld h, d\n"
		    "ld l, e\n"

		    "ld [hl], a\n\n",
		    c=='+'? '+' : '-');
	    break;
	case '>': case '<': // todo - wrap hl around
	    fprintf(out,
		    "ld a, [hl%c]\n\n",
		    c=='>'? '+' : '-');
	    break;


	case ']':
	    branch_counter--;
	    /// if exec == on:
	    ///     active_block = matching [
	    /// exec = off
            /// check_active_block
	    fprintf(out,
		    "ld a, LOW(hram_temp)\n"    // when the exec is off
		    "ld [temp_0], a\n"          // if exec is off
		    "ld a, LOW(active_block)\n" // when the exec is on
		    "ld [temp_1], a\n"          // if exec is on
		    
		    "ld de, temp_0\n"
		    "ldh a, [exec]\n"
		    "ld e, a\n" // [de] == exec? active_block: hram_temp
		    
		    "ld a, [de]\n"
		    "ld c, a\n"
		    "ld a, block_%ld\n" // matching [
		    "ldh [c], a\n"
		    "ld a, 0\n"
		    "ldh [exec], a\n" // exec = off

		    "check_active_block\n",
		    branch_counter);
	    break;
	case '[':
	    /// check_active_block
	    /// if [hl] == 0:
	    ///     exec = off
	    ///     active_block = block_after_matching_]

	    // business logic;
	    branch_counter++;
	    break;
	}
    }
}
