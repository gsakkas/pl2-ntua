#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// op-codes
#define HALT 0x00
#define JUMP 0x01
#define JNZ 0x02
#define DUP 0x03
#define DROP 0x04
#define PUSH4 0x05
#define PUSH2 0x06
#define PUSH1 0x07
#define ADD 0x08
#define SUB 0x09
#define MUL 0x0a
#define DIV 0x0b
#define MOD 0x0c
#define EQ 0x0d
#define NE 0x0e
#define LT 0x0f
#define GT 0x10
#define LE 0x11
#define GE 0x12
#define NOT 0x13
#define AND 0x14
#define OR 0x15
#define INPUT 0x16
#define OUTPUT 0x17
#define CLOCK 0x2a

// Debug
// #define __DEBUG__
// #define __DEBUG_STACK__
// #define __DEBUG_GET_BYTES__


// Helper functions
#define push(STACK, TOP, ELEM) (STACK[++TOP] = ELEM)
#define pop(STACK, TOP) (STACK[TOP--])
#define get_1_ubyte(PC) program[PC + 1]
#define get_2_ubytes(PC) program[PC + 1] + (program[PC + 2] << 8)
#define get_1_byte(PC) program[PC + 1]
#define get_2_bytes(PC) program[PC + 1] + (program[PC + 2] << 8)
#define get_4_bytes(PC) program[PC + 1] + (program[PC + 2] << 8) + (program[PC + 3] << 16) + (program[PC + 4] << 24)

// Directly threaded interpreter's next instruction
#ifndef __DEBUG_STACK__
	#define NEXT_INSTR goto *program[pc]
#else
	#define NEXT_INSTR goto print_stack_label
	#define NEXT_INSTR_ORIG goto *program[pc]
#endif
#define EXIT goto exit_label


int main(int argc, char const *argv[]) {
	// Indirectly threaded interpreter's label table
	static void *label_tab[] = {
		&&halt_label,
		&&jump_label,
		&&jnz_label,
		&&dup_label,
		&&drop_label,
		&&push4_label,
		&&push2_label,
		&&push1_label,
		&&add_label,
		&&sub_label,
		&&mul_label,
		&&div_label,
		&&mod_label,
		&&eq_label,
		&&ne_label,
		&&lt_label,
		&&gt_label,
		&&le_label,
		&&ge_label,
		&&not_label,
		&&and_label,
		&&or_label,
		&&input_label,
		&&ouput_label,
		&&not_valid_label,
		&&not_valid_label,
		&&not_valid_label,
		&&not_valid_label,
		&&not_valid_label,
		&&not_valid_label,
		&&not_valid_label,
		&&not_valid_label,
		&&not_valid_label,
		&&not_valid_label,
		&&not_valid_label,
		&&not_valid_label,
		&&not_valid_label,
		&&not_valid_label,
		&&not_valid_label,
		&&not_valid_label,
		&&not_valid_label,
		&&not_valid_label,
		&&clock_label
	};

	static uint8_t bytes_to_skip[] = {
		0,	//HALT
		2,	//JUMP
		2,	//JNZ
		1,	//DUP
		0,	//DROP
		4,	//PUSH4
		2,	//PUSH2
		1,	//PUSH1
		0,	//ADD
		0,	//SUB
		0,	//MUL
		0,	//DIV
		0,	//MOD
		0,	//EQ
		0,	//NE
		0,	//LT
		0,	//GT
		0,	//LE
		0,	//GE
		0,	//NOT
		0,	//AND
		0,	//OR
		0,	//INPUT
		0,	//OUTPUT
		0,	//NOT_VALID
		0,	//NOT_VALID
		0,	//NOT_VALID
		0,	//NOT_VALID
		0,	//NOT_VALID
		0,	//NOT_VALID
		0,	//NOT_VALID
		0,	//NOT_VALID
		0,	//NOT_VALID
		0,	//NOT_VALID
		0,	//NOT_VALID
		0,	//NOT_VALID
		0,	//NOT_VALID
		0,	//NOT_VALID
		0,	//NOT_VALID
		0,	//NOT_VALID
		0,	//NOT_VALID
		0,	//NOT_VALID
		0	//CLOCK
	};

	// Take program from stdin
	FILE *fin = fopen(argv[1], "rb");

	// Initialize a stack to hold the program
	register int32_t top = -1;
	int32_t stack[1 << 16];
	uint64_t program[1 << 16];
	uint8_t opcode_byte;

	// Read the file that contains the program
	uint16_t length = 0;
	while (fscanf(fin, "%c", &opcode_byte) == 1) {
		program[length++] = (uint64_t)label_tab[opcode_byte];
		for(uint8_t byte = 0; byte < bytes_to_skip[opcode_byte]; byte++) {
			uint8_t opcode_byte_temp;
			if (fscanf(fin, "%c", &opcode_byte_temp) != 1) {
				printf("Error: Program bytecode is wrong!\n");
				return -1;
			}
			else program[length++] = (uint64_t)opcode_byte_temp;
		}
	}
	fclose(fin);

	// The Bytecode Interpreter
	register uint32_t pc = 0;
	clock_t start_time = clock();
	NEXT_INSTR;
	#ifdef __DEBUG_STACK__
	print_stack_label:
		if (top < 0) printf("stack[-1] = ...\n");
		else {
			for (int32_t j = top; j >= 0; j--){
				printf("stack[%d] = %d\n", j, stack[j]);
			}
		}
		NEXT_INSTR_ORIG;
	#endif
	halt_label:
	{
		#ifdef __DEBUG__
			printf("HALT\n");
		#endif
		EXIT;
	}
	jump_label:
	{	
		#ifdef __DEBUG__
			printf("JUMP\n");
		#endif
		pc = get_2_ubytes(pc);
		NEXT_INSTR;
	}
	jnz_label:
	{
		#ifdef __DEBUG__
			printf("JNZ\n");
		#endif
		int32_t stack_top = pop(stack, top);
		pc = (stack_top != 0) ? get_2_ubytes(pc) : (pc + 3);
		NEXT_INSTR;
	}
	dup_label:
	{	
		#ifdef __DEBUG__
			printf("DUP\n");
		#endif
		uint8_t i = get_1_ubyte(pc);
		pc += 2;
		int32_t elem = stack[top - i];
		push(stack, top, elem);
		NEXT_INSTR;
	}
	drop_label:
	{
		#ifdef __DEBUG__
			printf("DROP\n");
		#endif
		pc += 1;
		top--;
		NEXT_INSTR;
	}
	push4_label:
	{
		#ifdef __DEBUG__
			printf("PUSH4\n");
		#endif
		int32_t num = get_4_bytes(pc);
		pc += 5;
		push(stack, top, num);
		NEXT_INSTR;
	}
	push2_label:
	{
		#ifdef __DEBUG__
			printf("PUSH2\n");
		#endif
		int16_t num = get_2_bytes(pc);
		pc += 3;
		push(stack, top, num);
		NEXT_INSTR;
	}
	push1_label:
	{
		#ifdef __DEBUG__
			printf("PUSH1\n");
		#endif
		int8_t num = get_1_byte(pc);
		pc += 2;
		push(stack, top, num);
		NEXT_INSTR;
	}
	add_label:
	{
		#ifdef __DEBUG__
			printf("ADD\n");
		#endif
		pc += 1;
		stack[top - 1] += stack[top];
		top--;
		NEXT_INSTR;
	}
	sub_label:
	{
		#ifdef __DEBUG__
			printf("SUB\n");
		#endif
		pc += 1;
		stack[top - 1] -= stack[top];
		top--;
		NEXT_INSTR;
	}
	mul_label:
	{
		#ifdef __DEBUG__
			printf("MUL\n");
		#endif
		pc += 1;
		stack[top - 1] *= stack[top];
		top--;
		NEXT_INSTR;
	}
	div_label:
	{
		#ifdef __DEBUG__
			printf("DIV\n");
		#endif
		pc += 1;
		stack[top - 1] /= stack[top];
		top--;
		NEXT_INSTR;
	}
	mod_label:
	{
		#ifdef __DEBUG__
			printf("MOD\n");
		#endif
		pc += 1;
		stack[top - 1] %= stack[top];
		top--;
		NEXT_INSTR;
	}
	eq_label:
	{
		#ifdef __DEBUG__
			printf("EQ\n");
		#endif
		pc += 1;
		stack[top - 1] = stack[top - 1] == stack[top];
		top--;
		NEXT_INSTR;
	}
	ne_label:
	{
		#ifdef __DEBUG__
			printf("NE\n");
		#endif
		pc += 1;
		stack[top - 1] = stack[top - 1] != stack[top];
		top--;
		NEXT_INSTR;
	}
	lt_label:
	{
		#ifdef __DEBUG__
			printf("LT\n");
		#endif
		pc += 1;
		stack[top - 1] = stack[top - 1] < stack[top];
		top--;
		NEXT_INSTR;
	}
	gt_label:
	{
		#ifdef __DEBUG__
			printf("GT\n");
		#endif
		pc += 1;
		stack[top - 1] = stack[top - 1] > stack[top];
		top--;
		NEXT_INSTR;
	}
	le_label:
	{
		#ifdef __DEBUG__
			printf("LE\n");
		#endif
		pc += 1;
		stack[top - 1] = stack[top - 1] <= stack[top];
		top--;
		NEXT_INSTR;
	}
	ge_label:
	{
		#ifdef __DEBUG__
			printf("GE\n");
		#endif
		pc += 1;
		stack[top - 1] = stack[top - 1] >= stack[top];
		top--;
		NEXT_INSTR;
	}
	not_label:
	{
		#ifdef __DEBUG__
			printf("NOT\n");
		#endif
		pc += 1;
		stack[top] = !stack[top];
		NEXT_INSTR;
	}
	and_label:
	{
		#ifdef __DEBUG__
			printf("AND\n");
		#endif
		pc += 1;
		stack[top - 1] = stack[top - 1] && stack[top];
		top--;
		NEXT_INSTR;
	}
	or_label:
	{
		#ifdef __DEBUG__
			printf("OR\n");
		#endif
		pc += 1;
		stack[top - 1] = stack[top - 1] || stack[top];
		top--;
		NEXT_INSTR;
	}
	input_label:
	{
		#ifdef __DEBUG__
			printf("INPUT\n");
		#endif
		pc += 1;
		char ch;
		if (scanf("%c", &ch) != 1) {
			printf("Error: Problem with input!\n");
			return -1;
		}
		push(stack, top, (int32_t)ch);
		NEXT_INSTR;
	}
	ouput_label:
	{
		#ifdef __DEBUG__
			printf("OUTPUT\n");
		#endif
		pc += 1;
		int32_t ch = pop(stack, top);
		printf("%c", (char)ch);
		#ifdef __DEBUG__
			printf("\n");
			printf("%d\n", ch);
		#endif
		NEXT_INSTR;
	}
	clock_label:
	{	
		#ifdef __DEBUG__
			printf("CLOCK\n");
		#endif
		pc += 1;
		double time_spent = (double)(clock() - start_time) / CLOCKS_PER_SEC;
		printf("%0.6lf\n", time_spent);
		NEXT_INSTR;
	}
	not_valid_label:
	{
		#ifdef __DEBUG__
			printf("NOT_AN_OPCODE\n");
		#endif
		EXIT;
	}
	exit_label:
	#ifdef __DEBUG_STACK__
		if (top < 0) printf("stack[-1] = ...\n");
		else {
			for (int32_t j = top; j >= 0; j--){
				printf("stack[%d] = %d\n", j, stack[j]);
			}
		}
	#endif
	return 0;
}