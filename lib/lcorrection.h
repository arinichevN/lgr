#ifndef LIBPAS_LCORRECTION_H
#define LIBPAS_LCORRECTION_H

#include <stdlib.h>
#include "app.h"
#include "dstructure.h"
#include "tsv.h"

typedef struct {
    int id;
    float factor;
    float delta;
} LCorrection;

DEC_LIST(LCorrection)
DEC_FUN_LIST_GET_BY_ID(LCorrection)

extern int initLCorrection(LCorrectionList *list, const char *config_path);
extern void lcorrect(float *out, LCorrection *x);
#endif

