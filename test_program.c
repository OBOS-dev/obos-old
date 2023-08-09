const int roglobal_variable = 0xDEADBEEF;
int global_variable = 0;

int _start(void(*printf)(const char* format, ...))
{
    int a = roglobal_variable;
    a |= (1 << 22);
    global_variable = a;
    // Why have I done this?
    printf("global_variable=%p, roglobal_variable=%p, a=%p.\r\n&global_variable=%p, &roglobal_variable=%p, &a=%p\r\n", global_variable, roglobal_variable, a, 
                                                                                                                   &global_variable, &roglobal_variable, &a);
    return global_variable;
}
