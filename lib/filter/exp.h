#ifndef LIBPAS_FILTER_EXP_H
#define LIBPAS_FILTER_EXP_H

#include <stdlib.h>
#include "../app.h"
#include "../dstructure.h"
#include "../tsv.h"

typedef struct {
    int id;
    float a;
    float vp;
    int f;
} FilterEXP;

#define FILTER_EXP_INITIALIZER {.f=0}

DEC_LIST(FilterEXP)
DEC_FUN_LIST_GET_BY_ID(FilterEXP)

extern int fexp_init(FilterEXP *item, int id, float a);
extern int fexp_initList(FilterEXPList *list, const char *config_path);
extern void fexp_calc(float *v, void *filter);
extern void fexp_freeList(FilterEXPList *list);
extern void fexp_free(FilterEXP *item);

#endif

