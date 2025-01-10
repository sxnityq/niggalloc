#include <unistd.h>
#include <stdarg.h>


void nig_itoa(int val, char *dst)
{
    int v = val;
    int i, p;
    char tmp[32];

    for(i = 0; 10 <= v; i++){
        tmp[i] = '0' + (v % 10);
        v /= 10;
    }
    tmp[i] = '0' + v;

    for(p = 0; i >= 0; i--, p++){
        dst[p] = tmp[i];
    }
    dst[p] = '\0'; 
}


void niga_print(char *format, int count, ...)
{
    char *p;
    char dst[32];
    va_list ap;
    
    va_start(ap, count);

    p = format;

    while(*p != '\0'){
        if (*p == '%'){
            switch (*(p+1))
            {
            case 'd':
                nig_itoa(va_arg(ap, int), dst);
                for (char *i = dst; *i != '\0'; i++){
                    write(STDOUT_FILENO, i, 1);
                }
                p++;
                break;
            /* case 'p':
                nig_itoa((int)(char *) va_arg(ap, void *), dst);
                for (char *i = dst; *i != '\0'; i++){
                    write(STDOUT_FILENO, i, 1);
                }
                p++;
                break; */
            }
        } else {
            write(STDOUT_FILENO, p, 1);
        }
        p++;
    }
}