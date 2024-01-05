#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "const.h"

#define PG_INVALID (~(uintptr_t)0) //<! invalid phypage addr
#define PG_MASK_P  0x1             //<! P
#define PG_MASK_RW 0x2             //<! R/W
#define PG_MASK_US 0x4             //<! U/S
#define PG_NP      0               //<! not present
#define PG_P       PG_MASK_P       //<! present
#define PG_RX      0               //<! read & executable
#define PG_RWX     PG_MASK_RW      //<! read & write & executable
#define PG_S       0               //<! supervisor
#define PG_U       PG_MASK_US      //<! user

/*!
 * \brief free pde table according to the given cr3
 *
 * \param cr3 page table phyaddr
 *
 * \return succeed or not, always return true currently
 */
bool pg_free_pde(u32 cr3);

/*!
 * \brief unmap all pte table in the given cr3(pde) table
 *
 * \param cr3 page table phyaddr
 * \param free whether to free the phy page held by laddr
 *
 * \return succeed or not, always return true currently
 */
bool pg_unmap_pte(u32 cr3, bool free);

/*!
 * \brief unmap laddr in page table
 *
 * \param cr3 page table phyaddr
 * \param laddr linear address to unmap
 * \param free whether to free the phy page held by laddr
 *
 * \return succeed or not, always return true currently
 */
bool pg_unmap_laddr(u32 cr3, u32 laddr, bool free);

/*!
 * \brief map laddr in page table
 *
 * \param laddr linear address to map
 * \param phyaddr expected phy page assigned to laddr, if PG_INVALID, try malloc
 * internally
 * \param pde_attr attribute for newly created pde, if already present, it will
 * not change
 * \param pte_attr attribute for newly created pte, overwrite even if present
 *
 * \return succeed or not, always return true currently
 *
 * \note since tlb flush is a slow op, map action will not actively refresh the
 * tlb cache, so remind to flush it at an appropriate time
 */
bool pg_map_laddr(u32 cr3, u32 laddr, u32 phyaddr, u32 pde_attr, u32 pte_attr);

/*!
 * \brief map range of laddr in page table
 *
 * \param laddr_base base linear address to map
 * \param laddr_limit limit of mapping range (lower than)
 * \param pde_attr attribute for newly created pde, if already present, it will
 * not change
 * \param pte_attr attribute for newly created pte, overwrite even if present
 *
 * \return succeed or not, always return true currently
 *
 * \note phy page assigned to laddr is always internally allocated, and take
 * care of recycle work of your allocated phy page. AGAIN, the same as
 * pg_map_laddr, pg_map_laddr_range do not actively flush the tlb cache
 */
bool pg_map_laddr_range(
    u32 cr3, u32 laddr_base, u32 laddr_limit, u32 pde_attr, u32 pte_attr);

/*!
 * \brief flush tlb cache
 */
void pg_refresh();

/*!
 * \brief create page table and map the kernel space
 *
 * \return phyaddr of page table (cr3)
 */
u32 pg_create_and_init();

//! NOTE: address of cr3 pde pte always located in kernel memory space, since we
//! always mapping it for the program in the initilization stage, access to
//! these should never cause a page fault, otherwise maybe something wrong in
//! the page table setup stage.

static u32  pg_offset(u32 laddr);
static u32  pg_pde_index(u32 laddr);
static u32  pg_pte_index(u32 laddr);
static u32  pg_frame_phyaddr(u32 entry);
static u32 *pg_pde_ptr(u32 cr3, u32 laddr);
static u32 *pg_pte_ptr(u32 pde, u32 laddr);
static u32  pg_pde(u32 cr3, u32 laddr);
static u32  pg_pte(u32 pde, u32 laddr);
static u32  pg_laddr_phyaddr(u32 cr3, u32 laddr);
static u32  pg_pde_attr(u32 cr3, u32 laddr, u32 mask);
static u32  pg_pte_attr(u32 pde, u32 laddr, u32 mask);
static bool pg_pde_exist(u32 cr3, u32 laddr);
static bool pg_pte_exist(u32 pte, u32 laddr);

static inline u32 pg_offset(u32 laddr) {
    return laddr & 0xfff;
}

static inline u32 pg_pde_index(u32 laddr) {
    return (laddr >> 22) & 0x3ff;
}

static inline u32 pg_pte_index(u32 laddr) {
    return (laddr >> 12) & 0x3ff;
}

static inline u32 pg_frame_phyaddr(u32 entry) {
    return entry & 0xfffff000;
}

static inline u32 *pg_pde_ptr(u32 cr3, u32 laddr) {
    return (u32 *)K_PHY2LIN(pg_frame_phyaddr(cr3)) + pg_pde_index(laddr);
}

static inline u32 *pg_pte_ptr(u32 pde, u32 laddr) {
    return (u32 *)K_PHY2LIN(pg_frame_phyaddr(pde)) + pg_pte_index(laddr);
}

static inline u32 pg_pde(u32 cr3, u32 laddr) {
    return *pg_pde_ptr(cr3, laddr);
}

static inline u32 pg_pte(u32 pde, u32 laddr) {
    return *pg_pte_ptr(pde, laddr);
}

static inline u32 pg_laddr_phyaddr(u32 cr3, u32 laddr) {
    u32 pde = pg_pde(cr3, laddr);
    u32 pte = pg_pte(pde, laddr);
    return pg_frame_phyaddr(pte) | pg_offset(laddr);
}

static inline u32 pg_pde_attr(u32 cr3, u32 laddr, u32 mask) {
    return pg_pde(cr3, laddr) & mask;
}

static inline u32 pg_pte_attr(u32 pde, u32 laddr, u32 mask) {
    return pg_pte(pde, laddr) & mask;
}

static inline bool pg_pde_exist(u32 cr3, u32 laddr) {
    return pg_pde_attr(cr3, laddr, PG_MASK_P) == PG_P;
}

static inline bool pg_pte_exist(u32 pte, u32 laddr) {
    return pg_pte_attr(pte, laddr, PG_MASK_P) == PG_P;
}
