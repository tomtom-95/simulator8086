bits 16

lock xchg byte [100], al
mov byte al, cs:[bx+si]
mov word bx, ds:[bp+di]
mov word dx, es:[bp+0]
mov byte ah, ss:[bx+si+4]
