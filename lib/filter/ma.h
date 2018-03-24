#ifndef LIBPAS_FILTER_MA_H
#define LIBPAS_FILTER_MA_H

#include <stdlib.h>
#include "../app.h"
#include "../dstructure.h"
#include "../tsv.h"

typedef struct {
    int id;
    float *buf;
    int length;
    int c_length;
    int i;
} FilterMA;

#define FILTER_MA_INITIALIZER {.buf=NULL, .length=0, .c_length=0, .i=0}

DEC_LIST(FilterMA)
DEC_FUN_LIST_GET_BY_ID(FilterMA)

extern int fma_init(FilterMA *item, int id, int length);
extern int fma_initList(FilterMAList *list, const char *config_path);
extern void fma_calc(float *v, void *filter);
extern void fma_freeList(FilterMAList *list);
extern void fma_free(FilterMA *item);

#endif

