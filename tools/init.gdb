# -*- mode:gdb-script -*-

layout split
focus cmd
winheight cmd 20
target remote:1234

set history filename ~/.gdb_history
set history save

b *reset_asm_handler
b *after_kmain
b kernel_panic

b *kmain
b *sys_exit

source utils.gdb

continue
