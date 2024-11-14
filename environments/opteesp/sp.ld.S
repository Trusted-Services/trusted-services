/* SPDX-License-Identifier: BSD-2-Clause AND BSD-3-Clause */
/*
 * Copyright (c) 2014, STMicroelectronics International N.V. All rights reserved.
 * Copyright (c) 2015, Linaro Limited. All rights reserved.
 * Copyright (c) 2021-2022, Arm Limited and Contributors. All rights reserved.
 */
#ifdef ARM32
OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
#endif
#ifdef ARM64
OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
#endif

EXTERN(sp_head)
ENTRY(__sp_entry)

SECTIONS {
	.sp_head : {*(.sp_head)}
	.text : {
		__text_start = .;
		*(.text .text.*)
		*(.stub)
		*(.glue_7)
		*(.glue_7t)
		*(.gnu.linkonce.t.*)
		/* Workaround for an erratum in ARM's VFP11 coprocessor */
		*(.vfp11_veneer)
		__text_end = .;
	}
	.plt : { *(.plt) }

	.eh_frame_hdr : {
		*(.eh_frame_hdr)
		*(.eh_frame_entry .eh_frame_entry.*)
	}
	.eh_frame : { KEEP(*(.eh_frame)) *(.eh_frame.*) }
	.gcc_except_table : { *(.gcc_except_table .gcc_except_table.*) }
	.rodata : {
		*(.gnu.linkonce.r.*)
		*(.rodata .rodata.*)
	}
	.ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) }
	/* .ARM.exidx is sorted, so has to go in its own output section.  */
	PROVIDE_HIDDEN(__exidx_start = .);
	.ARM.exidx : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) }
	PROVIDE_HIDDEN(__exidx_end = .);
	.ctors : { *(.ctors) }
	.dtors : { *(.dtors) }
	.dynsym : { *(.dynsym) }
	.dynstr : { *(.dynstr) }
	.hash : { *(.hash) }
	.note.gnu.build-id : { *(.note.gnu.build-id) }
	.note.gnu.property : { *(.note.gnu.property) }

	/* Page align to allow dropping execute bit for RW data */
	. = ALIGN(4096);

	.dynamic : { *(.dynamic) }
	.tdata : { *(.tdata .tdata.* .gnu.linkonce.td.*) }
	.tbss : { *(.tbss .tbss.* .gnu.linkonce.tb.*) *(.tcommon) }
	.got : { *(.got.plt) *(.got) }
	.rel.text : { *(.rel.text) *(.rel.gnu.linkonce.t*) }
	.rela.text : { *(.rela.text) *(.rela.gnu.linkonce.t*) }
	.rel.data : { *(.rel.data) *(.rel.gnu.linkonce.d*) }
	.rela.data : { *(.rela.data) *(.rela.gnu.linkonce.d*) }
	.rel.tdata : { *(.rel.tdata .rel.tdata.* .rel.gnu.linkonce.td.*) }
	.rela.tdata : { *(.rela.tdata .rela.tdata.* .rela.gnu.linkonce.td.*) }
	.rel.tbss : { *(.rel.tbss .rel.tbss.* .rel.gnu.linkonce.tb.*) }
	.rela.tbss : { *(.rela.tbss .rela.tbss.* .rela.gnu.linkonce.tb.*) }
	.rel.rodata : { *(.rel.rodata) *(.rel.gnu.linkonce.r*) }
	.rela.rodata : { *(.rela.rodata) *(.rela.gnu.linkonce.r*) }
	.rel.dyn : { *(.rel.dyn) }
	.rel.got : { *(.rel.got) }
	.rela.got : { *(.rela.got) }
	.rel.ctors : { *(.rel.ctors) }
	.rela.ctors : { *(.rela.ctors) }
	.rel.dtors : { *(.rel.dtors) }
	.rela.dtors : { *(.rela.dtors) }
	.rel.init : { *(.rel.init) }
	.rela.init : { *(.rela.init) }
	.rel.fini : { *(.rel.fini) }
	.rela.fini : { *(.rela.fini) }
	.rel.bss : { *(.rel.bss) }
	.rela.bss : { *(.rela.bss) }
	.rel.plt : { *(.rel.plt) }
	.rela.plt : { *(.rela.plt) }

	.data : { *(.data .data.* .gnu.linkonce.d.*) }
	.bss : {
		*(.bss .bss.* .gnu.linkonce.b.* COMMON)
	}

	/DISCARD/ : { *(.interp) }
}

ASSERT(sp_head == 0,
	"sp_head must be at address 0. Please check input sections and linker settings.");
