#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include "variable_array.h"
#include "jerrno.h"

void variable_array_release(struct variable_array *va)
{
    if (va == NULL) {
        return;
    }
    free(va->data);
    free(va);
}

void *variable_array_get(struct variable_array *va, uint32_t index)
{
    return (void *)(((char *)(va->data)) + index * va->typesize);
}

static jerrno_t expend_space(struct variable_array *va)
{
    void *new_data = realloc(va->data, va->typesize * ARRAY_NEW_SIZE(va->capacity));
    if (new_data == NULL) {
        fprintf(stderr, "[%s] remalloc failed\n", __func__);
        return E_NOMEM;
    }

    va->data = new_data;
    va->capacity = ARRAY_NEW_SIZE(va->capacity);

    return OK;
}

struct variable_array *variable_array_init(uint32_t typesize)
{
    struct variable_array *va = (struct variable_array *)malloc(sizeof(struct variable_array));
    if (va == NULL) {
        fprintf(stderr, "[%s] malloc variable array error\n", __func__);
        return NULL;
    }

    va->typesize = typesize;
    uint32_t malloc_size = va->typesize * VARIABLE_INIT_SIZE;
    va->data = malloc(malloc_size);
    if (va->data == NULL) {
        fprintf(stderr, "[%s] malloc data error\n", __func__);
        free(va);
        return NULL;
    }
    va->capacity = VARIABLE_INIT_SIZE;
    va->size = 0;

    return va;
}

jerrno_t variable_array_add(struct variable_array *va, void *i_data)
{
    jerrno_t ret;
    /* va can not add more elements since va->data is full */
    if (ARRAY_FULL(va)) {
        ret = expend_space(va);
    }

    (void)memcpy(variable_array_get(va, va->size), i_data, va->typesize);
    va->size++;
    return OK;
}
