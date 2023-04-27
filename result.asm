bits 16

mov si, bx
mov dh, al
mov cl, +12
mov ch, -12
mov cx, +12
mov cx, -12
mov dx, +3948
mov dx, -3948
mov byte al, [bx+si]
mov word bx, [bp+di]
mov word dx, [bp+0]
mov byte ah, [bx+si+4]
mov byte al, [bx+si+4999]
mov word [bx+di], cx
mov byte [bp+si], cl
mov byte [bp+0], ch
mov word ax, [bx+di-37]
mov word [si-300], cx
mov word dx, [bx-32]
mov byte [bp+di], +7
mov word [di+901], +347
mov word bp, [5]
mov word bx, [3458]
mov word ax, [2555]
mov word ax, [16]
mov word [2554], ax
mov word [15], ax
push word [bp+si]
push word [3000]
push word [bx+di-30]
push cx
push ax
push dx
push cs
pop word [bp+si]
pop word [3]
pop word [bx+di-3000]
pop sp
pop di
pop si
pop ds
xchg ax, [bp-1000]
xchg bp, [bx+50]
xchg ax, ax
xchg ax, dx
xchg ax, sp
xchg ax, si
xchg ax, di
xchg cx, dx
xchg si, cx
xchg cl, ah
in al, 200
in al, dx
in ax, dx
out 44, ax
out dx, al
xlat 
lea ax, [bx+di+1420]
lea bx, [bp-50]
lea sp, [bp-1003]
lea di, [bx+si-7]
lds ax, [bx+di+1420]
lds bx, [bp-50]
lds sp, [bp-1003]
lds di, [bx+si-7]
les ax, [bx+di+1420]
les bx, [bp-50]
les sp, [bp-1003]
les di, [bx+si-7]
lahf 
sahf 
pushf 
popf 
add cx, [bp+0]
add dx, [bx+si]
add byte [bp+di+5000], ah
add byte [bx], al
add sp, +392
add si, +5
add ax, +1000
add ah, +30
add al, +9
add cx, bx
add ch, al
adc cx, [bp+0]
adc dx, [bx+si]
adc byte [bp+di+5000], ah
adc byte [bx], al
adc sp, +392
adc si, +5
adc ax, +1000
adc ah, +30
adc al, +9
adc cx, bx
adc ch, al
inc ax
inc cx
inc dh
inc al
inc ah
inc sp
inc di
inc byte [bp+1002]
inc word [bx+39]
inc byte [bx+si+5]
inc word [bp+di-10044]
inc word [9349]
inc byte [bp+0]
aaa 
daa 
sub cx, [bp+0]
sub dx, [bx+si]
sub byte [bp+di+5000], ah
sub byte [bx], al
sub sp, +392
sub si, +5
sub ax, +1000
sub ah, +30
sub al, +9
sub cx, bx
sub ch, al
sbb cx, [bp+0]
sbb dx, [bx+si]
sbb byte [bp+di+5000], ah
sbb byte [bx], al
sbb sp, +392
sbb si, +5
sbb ax, +1000
sbb ah, +30
sbb al, +9
sbb cx, bx
sbb ch, al
dec ax
dec cx
dec dh
dec al
dec ah
dec sp
dec di
dec byte [bp+1002]
dec word [bx+39]
dec byte [bx+si+5]
dec word [bp+di-10044]
dec word [9349]
dec byte [bp+0]
neg ax
neg cx
neg dh
neg al
neg ah
neg sp
neg di
neg byte [bp+1002]
neg word [bx+39]
neg byte [bx+si+5]
neg word [bp+di-10044]
neg word [9349]
neg byte [bp+0]
cmp bx, cx
cmp dh, [bp+390]
cmp word [bp+2], si
cmp bl, +20
cmp byte [bx], +34
cmp ax, +23909
