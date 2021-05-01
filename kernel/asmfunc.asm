        ;; asmfunc.asm
        ;;
        ;; System V AMD64 Calling Convention
        ;; Registers: RDI, RSI, RDX, RCX, R8, R9

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

        global LoadGdt          ; void LoadGdt(uint16_t limit, uint64_t offset);
LoadGdt:
        push rbp
        mov rbp, rsp
        sub rsp, 10
        mov [rsp], di           ; limit
        mov [rsp + 2], rsi      ; offset
        lgdt [rsp]
        mov rsp, rbp
        pop rbp
        ret

        global SetCsSs          ; void SetCsSs(uint16_t cs, uint16_t ss);
SetCsSs:
        push rbp
        mov rbp, rsp
        mov ss, si
        mov rax, .next
        push rdi                ; CS
        push rax                ; RIP
        o64 retf
        .next:
        mov rsp, rbp
        pop rbp
        ret

        global SetDsAll         ; void SetDsAll(uint16_t value);
SetDsAll:
        mov ds, di
        mov es, di
        mov fs, di
        mov gs, di
        ret

        global SetCr3           ; void SetCr3(uint64_t value);
SetCr3:
        mov cr3, rdi
        ret

        extern kernel_main_stack
        extern KernelMainNewStack

        global KernelMain
KernelMain:
        mov rsp, kernel_main_stack + 1024 * 1024
        call KernelMainNewStack
        .fin:
        hlt
        jmp .fin
