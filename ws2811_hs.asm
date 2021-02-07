;
;    Copyright (C) 2012  Kevin Timmerman
;
;	This program is free software: you can redistribute it and/or modify
;	it under the terms of the GNU General Public License as published by
;	the Free Software Foundation, either version 3 of the License, or
;	(at your option) any later version.
;
;	This program is distributed in the hope that it will be useful,
;	but WITHOUT ANY WARRANTY; without even the implied warranty of
;	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;	GNU General Public License for more details.
;
;	You should have received a copy of the GNU General Public License
;	along with this program.  If not, see <http://www.gnu.org/licenses/>.
;

		.cdecls C, LIST, "msp430g2452.h"

		.def	write_ws2811_hs

										; --- High Speed Mode
										;        High / Low us   High / Low cycles @ 16 MHz
										; Zero:  0.25 / 1.00        4 / 16
										; One:   0.60 / 0.65       10 / 10
										; Reset:    0 / 50+         0 / 800+
										;
		.text							;
										;
write_ws2811_hs:						; void write_ws2811_hs(uint8_t *data, uint16_t length, uint8_t pinmask);
										;
		push	R11						; Save R11
byte_loop_hs:							;		
		mov		#7, R11					; Do 7 bits in a loop
		mov.b	@R12+, R15				; Get next byte from buffer
bit_loop_hs:							; - Bit loop - 20 cycles per bit		
		rla.b	R15						; Get next bit
		jc		one_hs					; Jump if one...
		bis.b	R14, &P1OUT				; Output high
		bic.b	R14, &P1OUT				; Output low - 4 cycles elapsed
		bic.b	R14, &P1OUT				; 4 cycle nop
		jmp		next_bit_hs				; Next bit...
one_hs:									;
		bis.b	R14, &P1OUT				; Output high
		bis.b	R14, &P1OUT				; 4 cycle nop
		jmp		$ + 2					; 2 cycle nop
		bic.b	R14, &P1OUT				; Output low - 10 cycles elapsed
next_bit_hs:							;
		dec		R11						; Decrement bit count
		jne		bit_loop_hs				; Do next bit of not zero...
										;
		rla.b	R15						; Get final bit of byte
		jc		last_one_hs				; Jump if one...
		bis.b	R14, &P1OUT				; Output high
		bic.b	R14, &P1OUT				; Output low - 4 cycles elapsed
		jmp		$ + 2					; 2 cycle nop
		dec		R13						; Decrement byte count
		jne		byte_loop_hs			; Next byte if count not zero...
		jmp		reset_hs				; All bytes done, reset...
last_one_hs:							;
		bis.b	R14, &P1OUT				; Output high
		jmp		$ + 2					; 2 cycle nop
		mov		#7, R11					; Reset bit counter
		mov.b	@R12+, R15				; Get next byte from buffer
		bic.b	R14, &P1OUT				; Output low - 10 cycles elapsed
		dec		R13						; Decrement byte count
		jne		bit_loop_hs				; Do next byte if count is not zero...
										;
reset_hs:								;										
		mov		#800 / 3, R15			; 800 cycle delay for reset
		dec		R15						;
		jne		$ - 2					;
		pop		R11						; Restore R11
		ret								; Return
