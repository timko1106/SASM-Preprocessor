#R(X)R%X
#REG 2
#reg %R(%REG)
#nop R0 <- R0

#push(DST)@N <- %rsp ; PUSH %R(%DST) \
N: @0 <- %R(%DST) \
%R(%DST) <- 1 \
%rsp <- %rsp - %R(%DST)

#pop(SRC)%rsp <- %rsp++ ; POP %R(%SRC) \
@N <- %rsp \
N: %R(%SRC) <- @0 \

#call_extreme(name,_2) ; Не сохранять регистры \
R0 <- [@_%_2] \
%push(0) \
RC <- @%name \
_%_2: %nop

#call_short(name,_2) ; Сохранить только регистр R0 \
%push(0) \
R0 <- [@_%_2] \
%push(0) \
RC <- @%name \
_%_2: %pop(0)

#ret(name)ret_%name

#func(name) \
%ret(%name): %nop \
; FUNCTION %name\
%name: %nop

#exit %pop(0) \
;EXIT \
@N <- R0 \
N: RC <- @0

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

; place to save argument
var1: %nop
; n - N; fib(n<3)=1
%func(fibbonaci)
	R0 <- @var1 ;Прочитать аргумент
	R1 <- 3
	RF <- R0 ~ R1
	RC <- @fib_fast(S) ; Если < 3 - короткий путь
	RF <- R0 ~ R1
	RC <- @fib_3(Z) ; Если 3 - ответ 2
	R1 <- 1 ;fib(n)=fib(n-1)+fib(n-2). n хранить в R0, поэтому R0 сохранить ОБЯЗАНЫ.
	R0 <- R0 - R1
	@var1 <- R0
	%call_short(fibbonaci,fib_1)
	R2 <- @%ret(fibbonaci) ; R2=fib(n-1)
	R1 <- 1
	R0 <- R0 - R1; n-2
	@var1 <- R0
	R0 <- R2 ; теперь можно сохранять только R0
	%call_short(fibbonaci,fib_2)
	R2 <- @%ret(fibbonaci) ; R0=fib(n-2)
	R2 <- R2 + R0
	@%ret(fibbonaci) <- R2
	%exit
fib_3: R0 <- 2
	@%ret(fibbonaci) <- R0
	%exit
fib_fast: R1 <- 0
	RF <- R0 ~ R1
	RC <- @fib_zero(Z)
	R0 <- 1
	RC <- @exit1
fib_zero: R0 <- 0
exit1:@%ret(fibbonaci) <- R0
	%exit



_start: %nop

	R0 <- 5 ; fibbonaci(10)
	@var1 <- R0
	%call_extreme(fibbonaci,start_fib)
	R0 <- @%ret(fibbonaci)
	@0xCF <- R0

HLT: RC <- @HLT 
