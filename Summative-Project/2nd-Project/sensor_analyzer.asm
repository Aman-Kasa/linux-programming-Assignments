; ==============================================================================
; Project: Assembly-Based Text File Analysis
; Description: Reads 'sensor_readings.txt', traverses data char by char,
;              and counts total records and valid (non-empty) records.
; Architecture: x86_64 Linux (NASM)
; ==============================================================================

global _start

section .data
    ; File configuration
    filename db "sensor_readings.txt", 0
    
    ; Output messages format
    msg_total db "Total records: ", 0
    msg_valid db "Valid records: ", 0
    newline   db 10, 0
    
    ; Error messages
    err_open  db "Error: Cannot open sensor_readings.txt", 10, 0
    err_read  db "Error: Failed to read from file", 10, 0

section .bss
    ; Memory buffer for file chunks (4KB)
    buffer resb 4096      
    ; Buffer for integer-to-string conversion (enough for 64-bit int)
    num_buf resb 32       

section .text
_start:
    ; ----------------------------------------------------------------------
    ; 1. Initialize Counters
    ; We use registers for fast counting during memory traversal
    ; ----------------------------------------------------------------------
    xor r12, r12        ; r12 = total_records (Count of all lines)
    xor r13, r13        ; r13 = valid_records (Count of non-empty lines)
    xor r14, r14        ; r14 = current_line_len (Tracks chars in current line)
    xor r15, r15        ; r15b = last_char (Tracks the last character read)
    xor r9, r9          ; r9 = total_bytes_read (To check if file is totally empty)

    ; ----------------------------------------------------------------------
    ; 2. Open the File
    ; ----------------------------------------------------------------------
    mov rax, 2          ; syscall number for sys_open
    mov rdi, filename   ; pointer to filename string
    mov rsi, 0          ; Flags: O_RDONLY (Read Only)
    mov rdx, 0          ; Mode: 0 (Not creating a file)
    syscall

    ; Error handling: If rax < 0, sys_open failed
    cmp rax, 0
    jl .open_error
    
    mov r8, rax         ; Save the successful file descriptor (FD) in r8

.read_loop:
    ; ----------------------------------------------------------------------
    ; 3. Read File in Chunks
    ; ----------------------------------------------------------------------
    mov rax, 0          ; syscall number for sys_read
    mov rdi, r8         ; FD saved earlier
    mov rsi, buffer     ; Memory buffer address
    mov rdx, 4096       ; Number of bytes to read
    syscall

    ; Error handling: sys_read failure or EOF check
    cmp rax, 0
    jl .read_error      ; If rax < 0, read error occurred
    je .eof_reached     ; If rax == 0, End of File (EOF) reached

    add r9, rax         ; Update total bytes read overall
    mov rcx, rax        ; Set loop counter (rcx) to bytes read in this chunk
    mov rsi, buffer     ; rsi = pointer to current character in buffer

.process_char:
    ; ----------------------------------------------------------------------
    ; 4. Memory Traversal & Conditional Logic
    ; ----------------------------------------------------------------------
    mov al, byte [rsi]  ; Load a single byte (character) from memory
    mov r15b, al        ; Save to last_char tracker

    ; Identify line boundaries and handle line endings
    cmp al, 10          ; Check if char is '\n' (Unix LF)
    je .is_newline
    cmp al, 13          ; Check if char is '\r' (Windows CR)
    je .is_carriage_return

    ; If it's not a line ending, it's valid data
    inc r14             ; current_line_len++
    jmp .next_char

.is_newline:
    ; We hit a line boundary. 
    inc r12             ; Increment total records count
    
    ; Check if line is empty
    cmp r14, 0          ; Did we see any data characters before this newline?
    je .reset_len       ; If 0, it's an empty line (skip valid increment)
    inc r13             ; If > 0, it contains data, increment valid records

.reset_len:
    xor r14, r14        ; Reset current_line_len for the next line
    jmp .next_char

.is_carriage_return:
    ; CRLF (\r\n) Handling: We simply ignore '\r'. 
    ; It won't increment current_line_len, meaning a line with just \r\n 
    ; will correctly evaluate as an empty line length of 0.
    jmp .next_char

.next_char:
    ; Advance memory pointer and decrement loop counter
    inc rsi             ; Move to next byte in buffer
    dec rcx             ; Decrement bytes remaining in this chunk
    jnz .process_char   ; If rcx != 0, process next char
    jmp .read_loop      ; If chunk exhausted, read the next chunk from file

.eof_reached:
    ; ----------------------------------------------------------------------
    ; 5. EOF Handling & Cleanup
    ; ----------------------------------------------------------------------
    ; Close the file
    mov rax, 3          ; syscall number for sys_close
    mov rdi, r8         ; FD to close
    syscall

    ; Edge Case: Handle files that don't end with a newline character
    cmp r9, 0           ; Was the file completely empty (0 bytes)?
    je .print_results   ; If yes, print 0s
    
    cmp r15b, 10        ; Was the very last character a newline?
    je .print_results   ; If yes, we already counted it.

    ; If the file ends with data but no trailing newline, count the final line
    inc r12             ; total_records++
    cmp r14, 0          ; Was there data on this trailing line?
    je .print_results
    inc r13             ; valid_records++

.print_results:
    ; ----------------------------------------------------------------------
    ; 6. Format and Display Output
    ; ----------------------------------------------------------------------
    ; Output: "Total records: X"
    mov rdi, msg_total
    call print_string
    mov rax, r12
    call print_number
    mov rdi, newline
    call print_string

    ; Output: "Valid records: Y"
    mov rdi, msg_valid
    call print_string
    mov rax, r13
    call print_number
    mov rdi, newline
    call print_string

    ; Terminate program cleanly
    mov rax, 60         ; syscall sys_exit
    xor rdi, rdi        ; Exit code 0 (Success)
    syscall

; ==============================================================================
; ERROR HANDLERS
; ==============================================================================
.open_error:
    mov rdi, err_open
    call print_string
    mov rax, 60         ; sys_exit
    mov rdi, 1          ; Exit code 1 (Failure)
    syscall

.read_error:
    mov rdi, err_read
    call print_string
    mov rax, 60         ; sys_exit
    mov rdi, 1          ; Exit code 1 (Failure)
    syscall

; ==============================================================================
; UTILITY FUNCTIONS
; ==============================================================================

; ------------------------------------------------------------------------------
; print_string: Prints a null-terminated string to STDOUT
; Input: rdi = pointer to string
; ------------------------------------------------------------------------------
print_string:
    push rcx
    push rdx
    push rsi
    push rdi
    push rax

    ; Calculate string length
    mov rsi, rdi        ; rsi = string address
    xor rdx, rdx        ; rdx = length counter
.len_loop:
    cmp byte [rsi + rdx], 0
    je .len_done
    inc rdx
    jmp .len_loop
.len_done:
    ; Perform sys_write
    mov rax, 1          ; sys_write
    mov rdi, 1          ; STDOUT
    syscall

    pop rax
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    ret

; ------------------------------------------------------------------------------
; print_number: Converts an unsigned 64-bit integer to ASCII and prints it
; Input: rax = number to print
; ------------------------------------------------------------------------------
print_number:
    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push rsi

    mov rbx, 10         ; Base 10 divisor
    mov rdi, num_buf    ; Target buffer
    add rdi, 31         ; Move to end of buffer
    mov byte [rdi], 0   ; Null terminator
    dec rdi

    ; Special case: if number is 0, store '0' and print immediately
    test rax, rax
    jnz .div_loop
    mov byte [rdi], '0'
    jmp .print_done

.div_loop:
    xor rdx, rdx        ; Clear rdx before division
    div rbx             ; rax = rax / 10, rdx = rax % 10
    add dl, '0'         ; Convert remainder to ASCII
    mov [rdi], dl       ; Store digit
    dec rdi             ; Move backward
    test rax, rax       ; Quotient zero?
    jnz .div_loop

    inc rdi             ; Point to first digit

.print_done:
    call print_string   ; Print the constructed number string

    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    ret
