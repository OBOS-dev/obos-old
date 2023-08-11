const int roglobal_variable = 0xDEADBEEF;
int global_variable = 0;

extern void print(const char* string);

__asm__("print: ;"
        "mov $0, %eax;"
        "int $0x50;"
        "ret;");

int _start(void(*printf)(const char* format, ...))
{
    int a = roglobal_variable;
    a |= (1 << 22);
    global_variable = a;
    //printf("global_variable=%p, roglobal_variable=%p, a=%p.\r\n&global_variable=%p, &roglobal_variable=%p, &a=%p\r\n", global_variable, roglobal_variable, a, 
    //                                                                                                               &global_variable, &roglobal_variable, &a);
    print("Hello, syscalls!\r\n");
    return global_variable;
}
