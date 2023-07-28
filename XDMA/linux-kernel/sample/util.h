//#define DEBUG_PRINT
#ifdef DEBUG_PRINT
#define printf_debug(...) printf(__VA_ARGS__)
#else
#define printf_debug(...) {}
#endif

void dump_memory(void* buf, int len, char* str);
