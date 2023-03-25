PROCESSOR    18F4620

#include <xc.inc>

; CONFIGURATION (DO NOT EDIT)
CONFIG OSC = HSPLL      ; Oscillator Selection bits (HS oscillator, PLL enabled (Clock Frequency = 4 x FOSC1))
CONFIG FCMEN = OFF      ; Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor disabled)
CONFIG IESO = OFF       ; Internal/External Oscillator Switchover bit (Oscillator Switchover mode disabled)
; CONFIG2L
CONFIG PWRT = ON        ; Power-up Timer Enable bit (PWRT enabled)
CONFIG BOREN = OFF      ; Brown-out Reset Enable bits (Brown-out Reset disabled in hardware and software)
CONFIG BORV = 3         ; Brown Out Reset Voltage bits (Minimum setting)
; CONFIG2H
CONFIG WDT = OFF        ; Watchdog Timer Enable bit (WDT disabled (control is placed on the SWDTEN bit))
; CONFIG3H
CONFIG PBADEN = OFF     ; PORTB A/D Enable bit (PORTB<4:0> pins are configured as digital I/O on Reset)
CONFIG LPT1OSC = OFF    ; Low-Power Timer1 Oscillator Enable bit (Timer1 configured for higher power operation)
CONFIG MCLRE = ON       ; MCLR Pin Enable bit (MCLR pin enabled; RE3 input pin disabled)
; CONFIG4L
CONFIG LVP = OFF        ; Single-Supply ICSP Enable bit (Single-Supply ICSP disabled)
CONFIG XINST = OFF      ; Extended Instruction Set Enable bit (Instruction set extension and Indexed Addressing mode disabled (Legacy mode))

; GLOBAL SYMBOLS
; You need to add your variables here if you want to debug them.
GLOBAL duration, duration2, duration3, last_portb, pause, temp, increment

; Define space for the variables in RAM
PSECT udata_acs
duration:
    DS 1 
duration2:
    DS 1
duration3:
    DS 1
last_portb:
    DS 1
pause:
    DS 1
temp:
    DS 1
increment:
    DS 1

PSECT resetVec,class=CODE,reloc=2
resetVec:
    goto       main

PSECT CODE
main:    
    clrf TRISA ; make PORTA an output
    clrf duration
    clrf duration2
    clrf duration3
    clrf last_portb
    clrf pause
    clrf temp
    movlw 100
    movwf duration
    movlw 00000111B ; light up RA1, RA2, RA3
    movwf LATA
    call wait1000ms
    clrf LATA
    setf PORTB

    
main_loop:
  call check_buttons
  movf pause
  bnz paused
  call metronome
paused:
  goto main_loop

check_buttons:
    comf PORTB, 0
    andwf last_portb, 0
    movwf temp
    btfsc temp, 0
    call rb0_pressed
    btfsc temp, 1
    call rb1_pressed
    btfsc temp, 2
    call rb2_pressed
    btfsc temp, 3
    call rb3_pressed
    btfsc temp, 4
    call rb4_pressed
    movff PORTB, last_portb
    return
  
    
rb0_pressed:
    comf pause
    return
    
rb1_pressed:
rb2_pressed:
rb3_pressed:
rb4_pressed:

metronome:
    
    incfsz duration
    return
    movlw 197
    movwf duration
    call overflow
    return
    
overflow:
    incfsz duration2
    return
    call overflow2
    return

overflow2:
    comf LATA  ; 500.486 ms
    return

  
wait1000ms: ; 999.783 ms
  movlw 246
  movwf duration
  clrf duration2
  call wait
  call wait
  call wait
  call wait
  call wait
  call wait
  return

wait: ; 992.022 ms
    call wait2
    incfsz duration
    goto wait
    return
    
wait2:
    incfsz duration2
    goto wait2
    return


end resetVec
