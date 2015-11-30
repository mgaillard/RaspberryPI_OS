# -*- mode: gdb-script -*-

set verbose off
set confirm off


# breakpoint on v1++;
break kmain-yieldto.c:12
commands
  print v1
  continue 
end

set $iterations=0

# breakpoint on v2-=2;
break kmain-yieldto.c:22
commands
  print v2
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
  set $ok *= ($1 == 5) 
  set $ok *= ($2 == -12)
  set $ok *= ($3 == 6)
  set $ok *= ($4 == -14)
  set $ok *= ($5 == 7)
  set $ok *= ($6 == -16)
  set $ok *= ($7 == 8)
  set $ok *= ($8 == -18)
  set $ok *= ($9 == 9)
  set $ok *= ($10== -20)

  if $ok
    printf "test OK\n"
  else
    printf "test ERROR\n"
  end
  quit
end

target remote:1234
continue
