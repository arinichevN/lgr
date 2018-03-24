#ifndef LIBPAS_LREDUCTION_H
#define LIBPAS_LREDUCTION_H

#include <stdlib.h>
#include "app.h"
#include "dstructure.h"
#include "tsv.h"

typedef struct {
    int id;
    float min_in;
    float max_in;
    float min_out;
    float max_out;
} LReduction;

DEC_LIST(LReduction)
DEC_FUN_LIST_GET_BY_ID(LReduction)

extern int initLReduction(LReductionList *list, const char *config_path);

extern void lreduct(float *out, LReduction *x);

#endif

