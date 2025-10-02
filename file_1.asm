%rsp <- 0xFF ; 0 1
RC <- @_init ; 2 3

#R(K)R%K

#REG 2

#reg %R(%REG)

#nop R0 <- R0

; PUSH - 7 bytes
; DST - number of register.
; Offsets:
; Store command address - 2
; P - offset 3
#push(DST,P,NO) \
@%P <- %rsp \
S_%NO: %nop \
P_S_%NO: %nop \
%R(%DST) <- 1 \
%rsp <- %rsp - %R(%DST)

; POP - 5 bytes
; SRC - number of register.
; Offsets:
; Load command address - 3
; P - offset 4
#pop(SRC,P,NO) \
%rsp <- %rsp++ \
@%P <- %rsp \
L_%NO: %nop \
P_L_%NO: %nop

; init - 4 bytes
#init(addr,value)R0 <- %value \
@%addr <- R0

_init: %init(19,0x22) ; 4 5 6 7 @addr <- R2
%init(27,0x18) ; 8 9 10 11
RC <- @_start ; 12 13

; Пусть регистр %rsp отвечает за стек.
; Стек начинается с адреса 0xFF и растёт вверх (=> хотелось бы декремент, но...)
; Поэтому для пробрасывания значения в стек будет использован "костыль"
; (сейчас для примера регистр R2):
; 1) write R2 по назначению
; 2) R2 = 1
; 3) %rsp -= R2
; Для взятия из стека нужен следующий метод (регистр R2):
; 1) inc %rsp
; 2) read R2

_start: %nop
%reg <- 0x15 ; 15 16

;PUSH 17-23
%push(%REG,20,0)

;POP 24-28
%pop(%REG,28,1)

@0x54 <- %rsp ;29 30

HLT: RC <- @HLT ; 31 32
