#pragma once

#include <compiler.h>
#include <stddef.h>

#define typeof_member(T, m) typeof(((T *)0)->m)

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 * WARNING: any const qualifier of @ptr is lost.
 */
#define container_of(ptr, type, member)                 \
    ({                                                  \
        void *__mptr = (void *)(ptr);                   \
        static_assert(                                  \
            is_same_type(*(ptr), ((type *)0)->member)   \
                || is_same_type(*(ptr), void),          \
            "pointer type mismatch in container_of()"); \
        ((type *)(__mptr - offsetof(type, member)));    \
    })

/**
 * container_of_const - cast a member of a structure out to the containing
 *			structure and preserve the const-ness of the pointer
 * @ptr:		the pointer to the member
 * @type:		the type of the container struct this is embedded in.
 * @member:		the name of the member within the struct.
 */
#define container_of_const(ptr, type, member)               \
    _Generic(                                               \
        ptr,                                                \
        const typeof(*(ptr)) *: (                           \
            (const type *)container_of(ptr, type, member)), \
        default: ((type *)container_of(ptr, type, member)))
