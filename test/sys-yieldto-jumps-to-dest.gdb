# -*- mode: gdb-script -*-

set verbose off
set confirm off

break *sys_yieldto
commands
  print $pc
  continue 
end

# breakpoint on v1++;
break kmain-yieldto.c:12
commands
  # we don't just print $pc, as it would be too hard to assess later on
  print &user_process_1
  continue 
end

set $iterations=0

# breakpoint on v2-=2;
break kmain-yieldto.c:22
commands
  # we don't just print $pc, as it would be too hard to assess later on
  print &user_process_2
  set $iterations++
  if $iterations==5
    assess_execution
  end
  continue 
end

define assess_execution
  # integer used as boolean
  set $ok = 1 
  # multiplication used as logical AND
  set $ok *= ($1  == sys_yieldto)
  set $ok *= ($2  == user_process_1)
  set $ok *= ($3  == sys_yieldto)
  set $ok *= ($4  == user_process_2)
  set $ok *= ($5  == sys_yieldto)
  set $ok *= ($6  == user_process_1)
  set $ok *= ($7  == sys_yieldto)
  set $ok *= ($8  == user_process_2)
  set $ok *= ($9  == sys_yieldto)
  set $ok *= ($10 == user_process_1)
  set $ok *= ($11 == sys_yieldto)
  set $ok *= ($12 == user_process_2)
  set $ok *= ($13 == sys_yieldto)
  set $ok *= ($14 == user_process_1)
  set $ok *= ($15 == sys_yieldto)
  set $ok *= ($16 == user_process_2)
  set $ok *= ($17 == sys_yieldto)
  set $ok *= ($18 == user_process_1)
  set $ok *= ($19 == sys_yieldto)
  set $ok *= ($20 == user_process_2)

  if $ok
    printf "test OK\n"
  else
    printf "test ERROR\n"
  end
  quit
end

target remote:1234
continue
