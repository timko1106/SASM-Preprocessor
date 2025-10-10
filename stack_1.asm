#RSP %rsp
%RSP <- 0xFF ; 0 1
RC <- @_init
; 2 3

#R(K)R%K

#REG 2

#reg %R(%REG)

#nop R0 <- R0

; DST - number of register.
#push(DST) \
@P <- %rsp \
S(%DST): %nop \
P: %nop \
%R(%DST) <- 1 \
%rsp <- %rsp - %R(%DST)

; SRC - number of register.

#pop(SRC) \
%rsp <- %rsp++ \
@P <- %rsp \
L(%SRC): %nop \
P: %nop

_init: R0 <- 0x22 ; 4 nop
@19 <- R0 ; 5 6 @addr <- R2
R0 <- 0x18 ; 7 8
@27 <- R0 ; 9 10 R2 <- @addr
RC <- @_start ; 11 12

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

_start: R0 <- R0 ; 13 14
%reg <- 0x15 ; 15 16

;PUSH
%push(%REG)

;POP

%pop(%REG)

@0x54 <- %rsp ;29 30

HLT: RC <- @HLT ;31 32
