#include <unios/sync.h>
#include <unios/memory.h>
#include <unios/schedule.h>
#include <unios/assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <atomic.h>
#include <stddef.h>

#define NR_KRNL_OBJS 16

enum krnl_obj_status {
    FLAG_USED = 0x80000000,
    FLAG_LOCK = 0x40000000,
    FLAG_BUSY = 0x20000000,
};

typedef struct krnl_obj_s {
    uint32_t id;
    int      user_id;
    int      status;
    void    *func;
} krnl_obj_t;

//! NOTE: only support spin lock currently
static krnl_obj_t krnl_obj_table[NR_KRNL_OBJS];

static handle_t krnlobj_lookup(int user_id) {
    for (int i = 0; i < NR_KRNL_OBJS; ++i) {
        if (krnl_obj_table[i].user_id == user_id) {
            return (handle_t)krnl_obj_table[i].id;
        }
    }
    return (handle_t)-1;
}

static handle_t krnlobj_create(int user_id) {
    int index = 0;
    while (index < NR_KRNL_OBJS) {
        if (krnl_obj_table[index].user_id == user_id) { return (handle_t)-1; }
        if (compare_exchange_strong(&krnl_obj_table[index].status, 0, FLAG_USED)
            == 0) {
            break;
        }
        ++index;
    }

    if (index == NR_KRNL_OBJS) { return (handle_t)-1; }

    krnl_obj_table[index].id      = index;
    krnl_obj_table[index].func    = kmalloc(sizeof(uint32_t));
    krnl_obj_table[index].user_id = user_id;
    return (handle_t)krnl_obj_table[index].id;
}

static void krnlobj_destroy(handle_t handle) {
    uint32_t id = (uint32_t)handle;
    assert(id >= 0 && id < NR_KRNL_OBJS);

    int old = 0;
    while (true) {
        old = compare_exchange_strong(
            &krnl_obj_table[id].status, FLAG_USED, FLAG_BUSY);
        assert(old & FLAG_USED);
        if (old == FLAG_USED) { break; }
    }

    kfree(krnl_obj_table[id].func);
    krnl_obj_table[id].user_id = 0;
    old = compare_exchange_strong(&krnl_obj_table[id].status, FLAG_BUSY, 0);
    assert(old == FLAG_BUSY);
}

static void krnlobj_lock(int user_id) {
    uint32_t id = (uint32_t)krnlobj_lookup(user_id);
    assert(id >= 0 && id < NR_KRNL_OBJS);

    int old = 0;
    while (true) {
        old = compare_exchange_strong(
            &krnl_obj_table[id].status, FLAG_USED, FLAG_USED | FLAG_BUSY);
        assert(old & FLAG_USED);
        if (old == FLAG_USED) { break; }
    }

    while (compare_exchange_strong(
               &krnl_obj_table[id].status,
               FLAG_USED | FLAG_BUSY,
               FLAG_USED | FLAG_BUSY | FLAG_LOCK)
           != (FLAG_USED | FLAG_BUSY)) {
        schedule();
    }
}

static void krnlobj_unlock(int user_id) {
    uint32_t id = (uint32_t)krnlobj_lookup(user_id);
    assert(id >= 0 && id < NR_KRNL_OBJS);
    int old = 0;
    while (true) {
        old = compare_exchange_strong(
            &krnl_obj_table[id].status,
            FLAG_USED | FLAG_BUSY | FLAG_LOCK,
            FLAG_USED);
        assert(old & FLAG_USED);
        if (old != (FLAG_USED | FLAG_BUSY | FLAG_LOCK)) {
            schedule();
        } else {
            break;
        }
    }
}

int do_krnlobj_request(int req, void *arg) {
    switch (req) {
        case KRNLOBJ_CREATE: {
            return (uint32_t)krnlobj_create((int)arg);
        } break;
        case KRNLOBJ_LOOKUP: {
            return (uint32_t)krnlobj_lookup((int)arg);
        } break;
        case KRNLOBJ_DESTROY: {
            krnlobj_destroy((handle_t)arg);
            return 0;
        } break;
        case KRNLOBJ_LOCK: {
            krnlobj_lock((int)arg);
            return 0;
        } break;
        case KRNLOBJ_UNLOCK: {
            krnlobj_unlock((int)arg);
            return 0;
        } break;
        default: {
            unreachable();
        } break;
    }
}

void rwlock_wait_rd(rwlock_t *lock) {
    assert(lock != NULL);
    while (!rwlock_try_lock_rd(lock)) {}
}

void rwlock_wait_wr(rwlock_t *lock) {
    assert(lock != NULL);
    while (!rwlock_try_lock_wr(lock)) {}
}

void rwlock_wait_rd_or(rwlock_t *lock, void (*callback)()) {
    assert(lock != NULL);
    while (!rwlock_try_lock_rd(lock)) {
        if (callback != NULL) { callback(); }
    }
}

void rwlock_wait_wr_or(rwlock_t *lock, void (*callback)()) {
    assert(lock != NULL);
    while (!rwlock_try_lock_wr(lock)) {
        if (callback != NULL) { callback(); }
    }
}

bool rwlock_try_lock_wr(rwlock_t *lock) {
    assert(lock != NULL);
    return compare_exchange_strong(lock, 0, -1) == 0;
}

bool rwlock_try_lock_rd(rwlock_t *lock) {
    assert(lock != NULL);
    int old = compare_exchange_strong(lock, 0, 1);
    if (old == 0) { return true; }
    if (old < 0) { return false; }
    return compare_exchange_strong(lock, old, old + 1) == old;
}

void rwlock_leave(rwlock_t *lock) {
    assert(lock != NULL);
    if (compare_exchange_strong(lock, 0, 0) <= 0) {
        //! hold wr lock
        int old = compare_exchange_strong(lock, -1, 0);
        assert(old == -1);
    } else {
        //! hold rd lock
        fetch_sub(lock, 1);
    }
}
