        bits 64
        section .text

        global IoOut32          ; void IoOut32(uint16_t addr, uint32_t data);
IoOut32:
        mov dx, di              ; dx = addr
        mov eax, esi            ; eax = data
        out dx, eax
        ret

        global IoIn32           ; uint32_t IoIn32(uint16_t addr);
IoIn32:
        mov dx, di              ; dx = addr
        in eax, dx
        ret


        global GetCs            ; uint16_t GetCs(void);
GetCs:
        xor eax, eax            ; also clears upper 32 bits of rax
        mov ax, cs
        ret

        global LoadIdt          ; void LoadIdt(uint16_t limit, uint64_t offset);
LoadIdt:
        push rbp
        mov rbp, rsp
        sub rsp, 10
        mov [rsp], di           ; limit
        mov [rsp + 2], rsi      ; offset
        lidt [rsp]
        mov rsp, rbp
        pop rbp
        ret

