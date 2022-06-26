#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sched.h>
#include <string.h>
#include "random.h"
#include "api-gcc.h"

int main() {
    atomic_t refcnt;    
    atomic_set(&refcnt, 3);

    int ret = atomic_dec_and_test(&refcnt);
    printf("refcnt -> %d, ret -> %d\n", refcnt, ret);
    // refcnt -> 2, ret -> 0
    // 减1之后的值是否等于0

    ret = atomic_dec_and_test(&refcnt);
    printf("refcnt -> %d, ret -> %d\n", refcnt, ret);
    // refcnt -> 1, ret -> 0
    // 减1之后的值是否等于0

    ret = atomic_dec_and_test(&refcnt);
    printf("refcnt -> %d, ret -> %d\n", refcnt, ret);
    // refcnt -> 0, ret -> 1
    // 减1之后的值是否等于0

    {
        atomic_set(&refcnt, 3);
        int oldv = 3;
        int newv = 4;
        int ret = atomic_cmpxchg(&refcnt, oldv, newv);
        printf("refcnt -> %d, ret -> %d\n", refcnt, ret);
        // refcnt -> 4, ret -> 3
        // refcnt刚开始等于oldv，然后修改成newv, 返回oldv, oldv等于ret
    }

    {
        atomic_set(&refcnt, 3);
        int oldv = 4;
        int newv = 5;
        int ret = atomic_cmpxchg(&refcnt, oldv, newv);
        printf("refcnt -> %d, ret -> %d\n", refcnt, ret);
        // refcnt -> 3, ret -> 3 
        // refcnt刚开始不等于oldv，do nothing, 返回oldv, oldv不等于ret
    }

    return 0;
}

