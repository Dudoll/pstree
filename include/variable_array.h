#ifndef __JOEL_VARIABLE_ARRAY_H_
#define __JOEL_VARIABLE_ARRAY_H_

#include <stdint.h>
#include "jerrno.h"

struct variable_array {
    void *data;
    uint32_t typesize;
    uint32_t capacity;
    uint32_t size;
};

#define VARIABLE_INIT_SIZE 16
/* the new array capacity is 1.5x than older one */
#define ARRAY_NEW_SIZE(x) ((x) * 3 / 2)
/* check whether the variable array is full */
#define ARRAY_FULL(va) (va->size >= va->capacity)
/* get the element index in array */
#define ARRAY_INDEX(arr, element) (element - (typeof(element))arr->data)
/* loop the variable array */
#define ARRAY_LOOP(p, va) for(p = (typeof(p))va->data; p < (typeof(p))va->data + va->size; p += 1)


struct variable_array *variable_array_init(uint32_t typesize);

jerrno_t variable_array_add(struct variable_array *va, void *i_data);

void variable_array_release(struct variable_array *va);

void *variable_array_get(struct variable_array *va, uint32_t index);

#endif // define __JOEL_VARIABLE_ARRAY_H_