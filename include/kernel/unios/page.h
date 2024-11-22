#pragma once

#include <unios/layout.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h> // IWYU pragma: keep

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
bool pg_free_page_table(uint32_t cr3);

/*!
 * \brief unmap all pte table in the given cr3(pde) table
 *
 * \param cr3 page table phyaddr
 * \param free whether to free the phy page held by laddr
 *
 * \return succeed or not, always return true currently
 */
bool pg_clear_page_table(uint32_t cr3, bool free);

/*!
 * \brief unmap laddr in page table
 *
 * \param cr3 page table phyaddr
 * \param laddr linear address to unmap
 * \param free whether to free the phy page held by laddr
 *
 * \return succeed or not, always return true currently
 */
bool pg_unmap_laddr(uint32_t cr3, uint32_t laddr, bool free);

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
 *
 * \attention no internal rollback supported, that means if run out of phy pages
 * occurs on the way, the succeeded part will not be cancelled, and you should
 * explicitly deal with the mess if you need it
 */
bool pg_map_laddr(
    uint32_t cr3,
    uint32_t laddr,
    uint32_t phyaddr,
    uint32_t pde_attr,
    uint32_t pte_attr);

/*!
 * \brief unmap range of laddr in page table
 *
 * \param laddr_base base linear address to unmap
 * \param laddr_limit limit of unmapping range (lower than)
 * \param free whether to free the phy page held by laddr
 *
 * \return succeed or not, always return true currently
 */
bool pg_unmap_laddr_range(
    uint32_t cr3, uint32_t laddr_base, uint32_t laddr_limit, bool free);

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
    uint32_t cr3,
    uint32_t laddr_base,
    uint32_t laddr_limit,
    uint32_t pde_attr,
    uint32_t pte_attr);

/*!
 * \brief flush tlb cache
 */
void pg_refresh();

/*!
 * \brief create page table and map the kernel space
 *
 * \param [out] p_cr3 pointer to phyaddr of page table (cr3)
 *
 * \return successful or not
 */
bool pg_create_and_init(uint32_t *p_cr3);

//! NOTE: address of cr3 pde pte always located in kernel memory space, since we
//! always mapping it for the program in the initilization stage, access to
//! these should never cause a page fault, otherwise maybe something wrong in
//! the page table setup stage.

static uint32_t  pg_offset(uint32_t laddr);
static uint32_t  pg_pde_index(uint32_t laddr);
static uint32_t  pg_pte_index(uint32_t laddr);
static uint32_t  pg_frame_phyaddr(uint32_t entry);
static uint32_t *pg_pde_ptr(uint32_t cr3, uint32_t laddr);
static uint32_t *pg_pte_ptr(uint32_t pde, uint32_t laddr);
static uint32_t  pg_pde(uint32_t cr3, uint32_t laddr);
static uint32_t  pg_pte(uint32_t pde, uint32_t laddr);
static uint32_t  pg_pde_attr(uint32_t cr3, uint32_t laddr, uint32_t mask);
static uint32_t  pg_pte_attr(uint32_t pde, uint32_t laddr, uint32_t mask);
static bool      pg_pde_exist(uint32_t cr3, uint32_t laddr);
static bool      pg_pte_exist(uint32_t pde, uint32_t laddr);

uint32_t pg_laddr_phyaddr(uint32_t cr3, uint32_t laddr);
bool     pg_addr_pte_exist(uint32_t cr3, uint32_t laddr);

#define PGBASE_CALL __attribute__((optimize("O3"), always_inline)) inline

PGBASE_CALL uint32_t pg_offset(uint32_t laddr) {
    return laddr & 0xfff;
}

PGBASE_CALL uint32_t pg_pde_index(uint32_t laddr) {
    return (laddr >> 22) & 0x3ff;
}

PGBASE_CALL uint32_t pg_pte_index(uint32_t laddr) {
    return (laddr >> 12) & 0x3ff;
}

PGBASE_CALL uint32_t pg_frame_phyaddr(uint32_t entry) {
    return entry & 0xfffff000;
}

PGBASE_CALL uint32_t *pg_pde_ptr(uint32_t cr3, uint32_t laddr) {
    return (uint32_t *)K_PHY2LIN(pg_frame_phyaddr(cr3)) + pg_pde_index(laddr);
}

PGBASE_CALL uint32_t *pg_pte_ptr(uint32_t pde, uint32_t laddr) {
    return (uint32_t *)K_PHY2LIN(pg_frame_phyaddr(pde)) + pg_pte_index(laddr);
}

PGBASE_CALL uint32_t pg_pde(uint32_t cr3, uint32_t laddr) {
    return *pg_pde_ptr(cr3, laddr);
}

PGBASE_CALL uint32_t pg_pte(uint32_t pde, uint32_t laddr) {
    return *pg_pte_ptr(pde, laddr);
}

PGBASE_CALL uint32_t pg_pde_attr(uint32_t cr3, uint32_t laddr, uint32_t mask) {
    return pg_pde(cr3, laddr) & mask;
}

PGBASE_CALL uint32_t pg_pte_attr(uint32_t pde, uint32_t laddr, uint32_t mask) {
    return pg_pte(pde, laddr) & mask;
}

PGBASE_CALL bool pg_pde_exist(uint32_t cr3, uint32_t laddr) {
    return pg_pde_attr(cr3, laddr, PG_MASK_P) == PG_P;
}

PGBASE_CALL bool pg_pte_exist(uint32_t pde, uint32_t laddr) {
    return pg_pte_attr(pde, laddr, PG_MASK_P) == PG_P;
}
