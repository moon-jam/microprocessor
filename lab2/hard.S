#include "xc.inc"
; Change 1/2/3 here to select one test case, then rebuild before debugging.
#define TEST_CASE 2

PSECT testcase, local, class=STRCODE, reloc=1
ORG 0x5000

#if TEST_CASE == 1
; Test Case 1: [0x03, 0x02, 0x01]
DW  0x03
DB  0x03, 0x02, 0x01
#elif TEST_CASE == 2
; Test Case 2: [0x0A, 0x34, 0x16, 0x0A, 0xF0, 0xC8]
DW  0x06
DB  0x0A, 0x34, 0x16, 0x0A, 0xF0, 0xC8
#elif TEST_CASE == 3
; Original example: 64 elements.
DW  0x40
DB  0x88, 0x7b, 0xb6, 0xbd, 0xf5, 0xfa, 0x8c, 0x9a
DB  0xca, 0xec, 0x95, 0x69, 0x3f, 0x33, 0xae, 0x23
DB  0x3d, 0x29, 0x85, 0x80, 0x44, 0x8a, 0x99, 0xc9
DB  0x41, 0x09, 0xc1, 0x5c, 0x66, 0xba, 0xac, 0xa4
DB  0xe0, 0xcb, 0x64, 0xc6, 0xa1, 0xa5, 0x2f, 0xa8
DB  0x7c, 0xd3, 0x93, 0x0c, 0x07, 0xd1, 0xd2, 0xc6
DB  0x11, 0x34, 0x1f, 0x2b, 0x31, 0x72, 0x4e, 0x71
DB  0xc3, 0xb9, 0xf6, 0xf0, 0x74, 0x6c, 0x5f, 0x70
#else
#error TEST_CASE must be 1, 2, or 3
#endif

PSECT main, local, abs, class=CODE, reloc=2
ORG 0x00
; Data loader, !DO NOT MODIFY!
DW 0x6af6, 0x6af8, 0x0e50, 0x6ef7, 0x0009, 0x50f5, 0x6f00, 0xee01
DW 0xf000, 0x000b, 0xcff5, 0xffee, 0x62e9, 0xd7fb, 0xee00, 0xf000

; (A[k] means RAM[0x100 + k]):
;   stack.push(0, RAM[0x000] - 1);       // store a pair (lo, hi)
;   while (!stack.empty()) {
;       (lo, hi) = stack.pop();
;       if (lo >= hi) continue;
;       pivot = A[hi];                    // last element of this range
;       i = lo;
;       for (j = lo; j < hi; ++j) {
;           if (A[j] <= pivot) {
;               swap(A[i], A[j]);
;               ++i;
;           }
;       }
;       swap(A[i], A[hi]);                // pivot reaches its final position
;       if (lo < i) stack.push(lo, i - 1);
;       if (i < hi) stack.push(i + 1, hi);
;   }
; FSR0 points to A[j] or A[hi]; FSR1 points to A[i].
; FSR2 is the stack pointer for unfinished (lo, hi) ranges.

main:
    ; q_lo/q_hi bound the current range; q_i/q_j are partition indices.
    ; These scratch bytes are in the access bank, outside A[0x100..0x1FE].
    q_lo       EQU 0x20
    q_hi       EQU 0x21
    q_i        EQU 0x22
    q_j        EQU 0x23
    q_pivot    EQU 0x24
    q_tmp      EQU 0x25
    q_pending  EQU 0x26

    ; stack.push(0, RAM[0x000] - 1).
    ; FSR2 starts one byte before the stack at 0x300.
    ; Each push writes lo then hi; each pop reads hi then lo.
    LFSR 2, 0x2FF
    CLRF q_pending, 0
    CLRF q_lo, 0
    DECF 0x00, 0, 0       ; WREG = length - 1; RAM[0x00] stays unchanged
    MOVWF q_hi, 0
    MOVFF q_lo, PREINC2
    MOVFF q_hi, PREINC2
    INCF q_pending, 1, 0

next_range:
    ; while (!stack.empty()) { (lo, hi) = stack.pop(); }
    ; q_pending counts how many ranges are waiting on the stack.
    MOVF q_pending, 0, 0
    BZ sorted             ; no ranges left
    DECF q_pending, 1, 0
    MOVFF POSTDEC2, q_hi
    MOVFF POSTDEC2, q_lo

    ; if (lo >= hi) continue; zero/one-element ranges need no work.
    MOVF q_hi, 0, 0
    CPFSLT q_lo, 0        ; skip GOTO when lo < hi
    GOTO next_range

    ; pivot = A[hi]; i = lo; j = lo.
    ; FSR0H/FSR1H = 0x01 because A starts at RAM address 0x100.
    MOVLW 0x01
    MOVWF FSR0H, 0
    MOVWF FSR1H, 0
    MOVFF q_hi, FSR0L
    MOVF INDF0, 0, 0
    MOVWF q_pivot, 0
    MOVFF q_lo, q_i
    MOVFF q_lo, q_j

scan:
    ; for (; j < hi; ++j) { if (A[j] <= pivot) ... }
    ; The compare instructions skip the following GOTO when true.
    MOVF q_hi, 0, 0
    CPFSLT q_j, 0         ; skip GOTO while j < hi
    GOTO place_pivot

    MOVFF q_j, FSR0L
    MOVF q_pivot, 0, 0
    CPFSGT INDF0, 0       ; skip GOTO when A[j] > pivot
    GOTO take_element
    GOTO advance_j

take_element:
    ; swap(A[i], A[j]); ++i. Both FSRs point into the original array.
    MOVFF q_i, FSR1L
    MOVFF INDF0, q_tmp
    MOVFF INDF1, INDF0
    MOVFF q_tmp, INDF1
    INCF q_i, 1, 0

advance_j:
    ; ++j; return to the for-loop condition.
    INCF q_j, 1, 0
    GOTO scan

place_pivot:
    ; after the for loop, swap(A[i], A[hi]).
    ; The pivot is now at its final index i; both sides still need sorting.
    MOVFF q_i, FSR1L
    MOVFF q_hi, FSR0L
    MOVFF INDF1, q_tmp
    MOVFF INDF0, INDF1
    MOVFF q_tmp, INDF0

    ; Push left range [lo, i-1] only when lo < i.
    MOVF q_i, 0, 0
    CPFSLT q_lo, 0        ; skip GOTO when lo < i
    GOTO no_left
    MOVFF q_lo, PREINC2
    DECF q_i, 0, 0        ; WREG = i - 1; q_i stays unchanged
    MOVWF q_tmp, 0
    MOVFF q_tmp, PREINC2
    INCF q_pending, 1, 0

no_left:
    ; if (i < hi) stack.push(i + 1, hi).
    ; We reach here whether or not the left range was pushed.
    MOVF q_hi, 0, 0
    CPFSLT q_i, 0         ; skip GOTO when i < hi
    GOTO next_range
    INCF q_i, 0, 0        ; WREG = i + 1; q_i stays unchanged
    MOVWF q_tmp, 0
    MOVFF q_tmp, PREINC2
    MOVFF q_hi, PREINC2
    INCF q_pending, 1, 0
    GOTO next_range

sorted:
    ; the while loop is over. Break on this NOP to inspect A in RAM.
    NOP

    END
