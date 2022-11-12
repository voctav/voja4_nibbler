/*
 * Nibbler - Emulator for Voja's 4-bit processor.
 *
 * Copyright (c) 2022 Octavian Voicu
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _OPS_H
#define _OPS_H

#include "vm.h"

#include <stdint.h>

#define MNEMNONIC_SIZE 5
#define INFO_SIZE 10

struct instruction_descriptor;

typedef void (*operation_fn_t)(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm);
typedef memory_addr_t (*operand_addr_fn_t)(const struct vm_instruction *instr, const struct vm_state *vm);
typedef uint8_t (*operand_val_fn_t)(const struct vm_instruction *instr, const struct vm_state *vm);
typedef void (*operand_info_fn_t)(const struct vm_instruction *instr, char *out);

struct operation {
	char mnemnonic[MNEMNONIC_SIZE];
	operation_fn_t op_fn;
};

struct operand_dst {
	char mnemnonic[MNEMNONIC_SIZE];
	operand_addr_fn_t get_addr;
	operand_info_fn_t get_info;
};

struct operand_cnd {
	char mnemnonic[MNEMNONIC_SIZE];
	operand_val_fn_t get_val;
	operand_info_fn_t get_info;
};

struct operand_src {
	char mnemnonic[MNEMNONIC_SIZE];
	operand_val_fn_t get_val;
	operand_info_fn_t get_info;
};

enum {
	OP_FLAG_DST_BYTE = 0x1,
	OP_FLAG_CAN_JUMP = 0x2,
	OP_FLAG_CAN_RD_SFR = 0x4,
	OP_FLAG_CAN_WR_SFR = 0x8,
	OP_FLAG_UPDATE_CARRY = 0x10,
};

struct instruction_descriptor {
	const struct operation *op;
	const struct operand_dst *dst;
	const struct operand_cnd *cnd;
	const struct operand_src *src;
	uint8_t flg;
};

void decode_instruction(program_word_t pi, struct vm_instruction *vmi);

const struct instruction_descriptor *get_instruction_descriptor(const struct vm_instruction *vmi);

void disassemble_instruction(const struct vm_instruction *vmi, const struct instruction_descriptor *descr, char *out, size_t size);

#endif /* _OPS_H */
