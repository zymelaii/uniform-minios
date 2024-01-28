#include <unios/syscall.h>
#include <unios/memory.h>
#include <unios/proc.h>
#include <unios/fs_const.h>
#include <unios/hd.h>
#include <unios/layout.h>
#include <unios/interrupt.h>
#include <unios/kstate.h>
#include <unios/schedule.h>
#include <unios/tracing.h>
#include <arch/x86.h>
#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include <atomic.h>
#include <string.h>
#include <time.h>
#include <math.h>

struct part_ent PARTITION_ENTRY;

static HDQueue      hdque;
static volatile int hd_int_waiting_flag;
static u8           hd_status;
static u8           hdbuf[SECTOR_SIZE * 2];
hd_info_t           hd_info[1];

static void init_hd_queue(HDQueue *hdq);
static void in_hd_queue(HDQueue *hdq, RWInfo *p);
static int  out_hd_queue(HDQueue *hdq, RWInfo **p);
static void hd_rdwt_real(RWInfo *p);

static void get_part_table(int drive, int sect_nr, struct part_ent *entry);
static void partition(int device, int style);
static void print_hdinfo(hd_info_t *hdi);
static void hd_identify(int drive);
static void print_identify_info(u16 *hdinfo);
static void hd_cmd_out(struct hd_cmd *cmd);

static void inform_int();
static void interrupt_wait();
static void hd_handler(int irq);
static int  waitfor(int mask, int val, int timeout);
//~xw

#define DRV_OF_DEV(dev)                     \
 (dev <= MAX_PRIM ? dev / NR_PRIM_PER_DRIVE \
                  : (dev - MINOR_hd1a) / NR_SUB_PER_DRIVE)

/*****************************************************************************
 *                                init_hd
 *****************************************************************************/
/**
 * <Ring 1> Check hard drive, set IRQ handler, enable IRQ and initialize data
 *          structures.
 *****************************************************************************/
void init_hd() {
    put_irq_handler(AT_WINI_IRQ, hd_handler);
    enable_irq(CASCADE_IRQ);
    enable_irq(AT_WINI_IRQ);

    const int n = sizeof(hd_info) / sizeof(hd_info_t);
    for (int i = 0; i < n; i++) { memset(&hd_info[i], 0, sizeof(hd_info_t)); }
    hd_info[0].open_cnt = 0;

    init_hd_queue(&hdque);
}

void hd_open(int drive) {
    kdebug("read hd info");

    /* Get the number of drives from the BIOS data area */
    // u8 * pNrDrives = (u8*)(0x475);
    hd_identify(drive);

    if (hd_info[drive].open_cnt++ == 0) {
        partition(drive * (NR_PART_PER_DRIVE + 1), P_PRIMARY);
        print_hdinfo(&hd_info[drive]);
    }

    kdebug("read hd info done");
}

void hd_close(int device) {
    int drive = DRV_OF_DEV(device);
    hd_info[drive].open_cnt--;
}

void hd_rdwt(MESSAGE *p) {
    static u32 lock = 0;
    lock_or(&lock, schedule);
    int drive = DRV_OF_DEV(p->DEVICE);

    u64 pos = p->POSITION;

    // We only allow to R/W from a SECTOR boundary:

    u32 sect_nr = (u32)(pos >> SECTOR_SIZE_SHIFT); // pos / SECTOR_SIZE
    int logidx  = (p->DEVICE - MINOR_hd1a) % NR_SUB_PER_DRIVE;
    sect_nr     += p->DEVICE < MAX_PRIM ? hd_info[drive].primary[p->DEVICE].base
                                        : hd_info[drive].logical[logidx].base;

    struct hd_cmd cmd;
    cmd.features = 0;
    cmd.count    = (p->CNT + SECTOR_SIZE - 1) / SECTOR_SIZE;
    cmd.lba_low  = sect_nr & 0xFF;
    cmd.lba_mid  = (sect_nr >> 8) & 0xFF;
    cmd.lba_high = (sect_nr >> 16) & 0xFF;
    cmd.device   = MAKE_DEVICE_REG(1, drive, (sect_nr >> 24) & 0xF);
    cmd.command  = (p->type == DEV_READ) ? ATA_READ : ATA_WRITE;
    hd_cmd_out(&cmd);

    int   bytes_left = p->CNT;
    void *la         = (void *)va2la(p->PROC_NR, p->BUF);

    while (bytes_left) {
        int bytes = min(SECTOR_SIZE, bytes_left);
        if (p->type == DEV_READ) {
            interrupt_wait();
            insw(REG_DATA, hdbuf, SECTOR_SIZE);
            memcpy(la, hdbuf, bytes);
        } else {
            if (!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT)) {
                abort("hd writing error.");
            }

            memcpy(hdbuf, la, bytes);
            outsw(REG_DATA, hdbuf, SECTOR_SIZE);
            interrupt_wait();
        }
        bytes_left -= SECTOR_SIZE;
        la         += SECTOR_SIZE;
    }
    release(&lock);
}

// added by xw, 18/8/26
void hd_service() {
    RWInfo *rwinfo;

    while (1) {
        // the hd queue is not empty when out_hd_queue return 1.
        while (out_hd_queue(&hdque, &rwinfo)) {
            hd_rdwt_real(rwinfo);
            rwinfo->proc->pcb.stat = READY;
        }
        yield();
    }
}

static void hd_rdwt_real(RWInfo *p) {
    int drive = DRV_OF_DEV(p->msg->DEVICE);

    u64 pos = p->msg->POSITION;

    // We only allow to R/W from a SECTOR boundary:

    u32 sect_nr = (u32)(pos >> SECTOR_SIZE_SHIFT); // pos / SECTOR_SIZE
    int logidx  = (p->msg->DEVICE - MINOR_hd1a) % NR_SUB_PER_DRIVE;
    sect_nr     += p->msg->DEVICE < MAX_PRIM
                     ? hd_info[drive].primary[p->msg->DEVICE].base
                     : hd_info[drive].logical[logidx].base;

    struct hd_cmd cmd;
    cmd.features = 0;
    cmd.count    = (p->msg->CNT + SECTOR_SIZE - 1) / SECTOR_SIZE;
    cmd.lba_low  = sect_nr & 0xFF;
    cmd.lba_mid  = (sect_nr >> 8) & 0xFF;
    cmd.lba_high = (sect_nr >> 16) & 0xFF;
    cmd.device   = MAKE_DEVICE_REG(1, drive, (sect_nr >> 24) & 0xF);
    cmd.command  = (p->msg->type == DEV_READ) ? ATA_READ : ATA_WRITE;
    hd_cmd_out(&cmd);

    int   bytes_left = p->msg->CNT;
    void *la         = p->kbuf; // attention here!

    while (bytes_left) {
        int bytes = min(SECTOR_SIZE, bytes_left);
        if (p->msg->type == DEV_READ) {
            interrupt_wait();
            insw(REG_DATA, hdbuf, SECTOR_SIZE);
            memcpy(la, hdbuf, bytes);
        } else {
            if (!waitfor(STATUS_DRQ, STATUS_DRQ, HD_TIMEOUT)) {
                abort("hd writing error.");
            }
            memcpy(hdbuf, la, bytes);
            outsw(REG_DATA, hdbuf, SECTOR_SIZE);
            interrupt_wait();
        }
        bytes_left -= SECTOR_SIZE;
        la         += SECTOR_SIZE;
    }
}

void hd_rdwt_sched(MESSAGE *p) {
    RWInfo rwinfo;
    int    size = p->CNT;
    void  *buffer;

    buffer      = kmalloc(size);
    rwinfo.msg  = p;
    rwinfo.kbuf = buffer;
    rwinfo.proc = p_proc_current;

    if (p->type == DEV_READ) {
        in_hd_queue(&hdque, &rwinfo);
        p_proc_current->pcb.channel = &hdque;
        p_proc_current->pcb.stat    = SLEEPING;
        schedule();
        memcpy(p->BUF, buffer, p->CNT);
    } else {
        memcpy(buffer, p->BUF, p->CNT);
        in_hd_queue(&hdque, &rwinfo);
        p_proc_current->pcb.channel = &hdque;
        p_proc_current->pcb.stat    = SLEEPING;
        schedule();
    }

    kfree(buffer);
}

void init_hd_queue(HDQueue *hdq) {
    hdq->front = hdq->rear = NULL;
}

static void in_hd_queue(HDQueue *hdq, RWInfo *p) {
    p->next = NULL;
    if (hdq->rear == NULL) { // put in the first node
        hdq->front = hdq->rear = p;
    } else {
        hdq->rear->next = p;
        hdq->rear       = p;
    }
}

static int out_hd_queue(HDQueue *hdq, RWInfo **p) {
    if (hdq->rear == NULL) return 0; // empty

    *p = hdq->front;
    if (hdq->front == hdq->rear) { // put out the last node
        hdq->front = hdq->rear = NULL;
    } else {
        hdq->front = hdq->front->next;
    }
    return 1; // not empty
}

//~xw

/*****************************************************************************
 *                                hd_ioctl
 *****************************************************************************/
/**
 * <Ring 1> This routine handles the DEV_IOCTL message.
 *
 * @param p  Ptr to the MESSAGE.
 *****************************************************************************/
void hd_ioctl(MESSAGE *p) {
    int device = p->DEVICE;
    int drive  = DRV_OF_DEV(device);

    hd_info_t *hdi = &hd_info[drive];

    if (p->REQUEST == DIOCTL_GET_GEO) {
        void *dst = va2la(p->PROC_NR, p->BUF);
        void *src = va2la(
            proc2pid(p_proc_current),
            device < MAX_PRIM
                ? &hdi->primary[device]
                : &hdi->logical[(device - MINOR_hd1a) % NR_SUB_PER_DRIVE]);

        memcpy(dst, src, sizeof(struct part_info));
    } else {
        unreachable();
    }
}

/*****************************************************************************
 *                                get_part_table
 *****************************************************************************/
/**
 * <Ring 1> Get a partition table of a drive.
 *
 * @param drive   Drive nr (0 for the 1st disk, 1 for the 2nd, ...)n
 * @param sect_nr The sector at which the partition table is located.
 * @param entry   Ptr to part_ent struct.
 *****************************************************************************/
static void get_part_table(int drive, int sect_nr, struct part_ent *entry) {
    struct hd_cmd cmd;
    cmd.features = 0;
    cmd.count    = 1;
    cmd.lba_low  = sect_nr & 0xFF;
    cmd.lba_mid  = (sect_nr >> 8) & 0xFF;
    cmd.lba_high = (sect_nr >> 16) & 0xFF;
    cmd.device   = MAKE_DEVICE_REG(
        1, /* LBA mode*/
        drive,
        (sect_nr >> 24) & 0xF);
    cmd.command = ATA_READ;
    hd_cmd_out(&cmd);
    interrupt_wait();

    insw(REG_DATA, hdbuf, SECTOR_SIZE);
    memcpy(
        entry,
        hdbuf + PARTITION_TABLE_OFFSET,
        sizeof(struct part_ent) * NR_PART_PER_DRIVE);
}

// added by mingxuan 2020-10-27
static void
    get_fs_flags(int drive, int sect_nr, struct fs_flags *fs_flags_buf) {
    struct hd_cmd cmd;
    cmd.features = 0;
    cmd.count    = 1;
    cmd.lba_low  = sect_nr & 0xFF;
    cmd.lba_mid  = (sect_nr >> 8) & 0xFF;
    cmd.lba_high = (sect_nr >> 16) & 0xFF;
    cmd.device   = MAKE_DEVICE_REG(
        1, /* LBA mode*/
        drive,
        (sect_nr >> 24) & 0xF);
    cmd.command = ATA_READ;
    hd_cmd_out(&cmd);
    interrupt_wait();

    insw(REG_DATA, hdbuf, SECTOR_SIZE);
    memcpy(fs_flags_buf, hdbuf, sizeof(struct fs_flags));
}

/*****************************************************************************
 *                                partition
 *****************************************************************************/
/**
 * <Ring 1> This routine is called when a device is opened. It reads the
 * partition table(s) and fills the hd_info struct.
 *
 * @param device Device nr.
 * @param style  P_PRIMARY or P_EXTENDED.
 *****************************************************************************/
static void partition(int device, int style) {
    int        i;
    int        drive = DRV_OF_DEV(device);
    hd_info_t *hdi   = &hd_info[drive];

    struct part_ent part_tbl[NR_SUB_PER_DRIVE];

    if (style == P_PRIMARY) {
        get_part_table(drive, drive, part_tbl);

        int nr_prim_parts = 0;
        for (i = 0; i < NR_PART_PER_DRIVE; i++) { /* 0~3 */
            if (part_tbl[i].sys_id == NO_PART) continue;

            nr_prim_parts++;
            int dev_nr                = i + 1; /* 1~4 */
            hdi->primary[dev_nr].base = part_tbl[i].start_sect;
            hdi->primary[dev_nr].size = part_tbl[i].nr_sects;

            // added by mingxuan 2020-10-27
            struct fs_flags fs_flags_buf;
            get_fs_flags(
                drive,
                hdi->primary[dev_nr].base + 1,
                &fs_flags_buf); // hdi->primary[dev_nr].base + 1 beacause of
                                // orange and fat32 is in 2nd sector, mingxuan
            if (fs_flags_buf.orange_flag == 0x11) // Orange's Magic
                hdi->primary[dev_nr].fs_type = ORANGE_TYPE;
            else if (
                fs_flags_buf.fat32_flag1 == 0x534f4453
                && fs_flags_buf.fat32_flag2 == 0x302e35) // FAT32 flags
                hdi->primary[dev_nr].fs_type = FAT32_TYPE;
            // added end, mingxuan 2020-10-27

            if (part_tbl[i].sys_id == EXT_PART) /* extended */
                partition(device + dev_nr, P_EXTENDED);
        }
    } else if (style == P_EXTENDED) {
        int j              = device % NR_PRIM_PER_DRIVE; /* 1~4 */
        int ext_start_sect = hdi->primary[j].base;
        int s              = ext_start_sect;
        int nr_1st_sub     = (j - 1) * NR_SUB_PER_PART; /* 0/16/32/48 */

        for (i = 0; i < NR_SUB_PER_PART; i++) {
            int dev_nr = nr_1st_sub + i; /* 0~15/16~31/32~47/48~63 */

            get_part_table(drive, s, part_tbl);

            hdi->logical[dev_nr].base = s + part_tbl[0].start_sect;
            hdi->logical[dev_nr].size = part_tbl[0].nr_sects;

            // added by mingxuan 2020-10-29
            struct fs_flags fs_flags_buf;
            get_fs_flags(
                drive,
                hdi->logical[dev_nr].base + 1,
                &fs_flags_buf); // hdi->primary[dev_nr].base + 1 beacause of
                                // orange and fat32 is in 2nd sector, mingxuan
            if (fs_flags_buf.orange_flag == 0x11) // Orange's Magic
                hdi->logical[dev_nr].fs_type = ORANGE_TYPE;
            else if (
                fs_flags_buf.fat32_flag1 == 0x534f4453
                && fs_flags_buf.fat32_flag2 == 0x302e35) // FAT32 flags
                hdi->logical[dev_nr].fs_type = FAT32_TYPE;
            // added end, mingxuan 2020-10-29

            s = ext_start_sect + part_tbl[1].start_sect;

            /* no more logical partitions
               in this extended partition */
            if (part_tbl[1].sys_id == NO_PART) break;
        }
    } else {
        unreachable();
    }
}

/*****************************************************************************
 *                                print_hdinfo
 *****************************************************************************/
/**
 * <Ring 1> Print disk info.
 *
 * @param hdi  Ptr to hd_info_t.
 *****************************************************************************/
static void print_hdinfo(hd_info_t *hdi) {
    int i;
    for (i = 0; i < NR_PART_PER_DRIVE + 1; i++) {
        kinfo(
            "%*sPART_%d: base %d, size: %d (in sector)",
            i == 0 ? 0 : 2,
            "",
            i,
            hdi->primary[i].base,
            hdi->primary[i].size);
    }
    for (i = 0; i < NR_SUB_PER_DRIVE; i++) {
        if (hdi->logical[i].size == 0) continue;
        kinfo(
            "%*s%d: base %d, size %d (in sector)",
            4,
            "",
            i,
            hdi->logical[i].base,
            hdi->logical[i].size);
    }
}

/*****************************************************************************
 *                                hd_identify
 *****************************************************************************/
/**
 * <Ring 1> Get the disk information.
 *
 * @param drive  Drive Nr.
 *****************************************************************************/
static void hd_identify(int drive) {
    struct hd_cmd cmd;
    cmd.device  = MAKE_DEVICE_REG(0, drive, 0);
    cmd.command = ATA_IDENTIFY;
    hd_cmd_out(&cmd);
    interrupt_wait();
    insw(REG_DATA, hdbuf, SECTOR_SIZE);

    print_identify_info((u16 *)hdbuf);

    u16 *hdinfo = (u16 *)hdbuf;

    hd_info[drive].primary[0].base = 0;
    /* Total Nr of User Addressable Sectors */
    hd_info[drive].primary[0].size = ((int)hdinfo[61] << 16) + hdinfo[60];
}

/*****************************************************************************
 *                            print_identify_info
 *****************************************************************************/
/**
 * <Ring 1> Print the hdinfo retrieved via ATA_IDENTIFY command.
 *
 * @param hdinfo  The buffer read from the disk i/o port.
 *****************************************************************************/
static void print_identify_info(u16 *hdinfo) {
    int  i, k;
    char s[64];

    struct iden_info_ascii {
        int   idx;
        int   len;
        char *desc;
    } iinfo[] = {
        {10, 20, "HD SN"   }, /* Serial number in ASCII */
        {27, 40, "HD Model"}  /* Model number in ASCII */
    };

    kinfo("HD Identity {");

    for (k = 0; k < sizeof(iinfo) / sizeof(iinfo[0]); k++) {
        char *p = (char *)&hdinfo[iinfo[k].idx];
        for (i = 0; i < iinfo[k].len / 2; i++) {
            s[i * 2 + 1] = *p++;
            s[i * 2]     = *p++;
        }
        s[i * 2] = 0;
        kinfo("  %s: %s", iinfo[k].desc, s);
    }

    int capabilities      = hdinfo[49];
    int cmd_set_supported = hdinfo[83];
    int sectors           = ((int)hdinfo[61] << 16) + hdinfo[60];

    kinfo("  LBA supported: %s", capabilities & 0x0200 ? "YES" : "NO");
    kinfo("  LBA48 supported: %s", cmd_set_supported & 0x0400 ? "YES" : "NO");
    kinfo("  HD size: %d MB", sectors * 512 / 1000000);
    kinfo("}");
}

/*****************************************************************************
 *                                hd_cmd_out
 *****************************************************************************/
/**
 * <Ring 1> Output a command to HD controller.
 *
 * @param cmd  The command struct ptr.
 *****************************************************************************/
static void hd_cmd_out(struct hd_cmd *cmd) {
    /**
     * For all commands, the host must first check if BSY=1,
     * and should proceed no further unless and until BSY=0
     */
    if (!waitfor(STATUS_BSY, 0, HD_TIMEOUT)) { abort("hd error"); }

    /* Activate the Interrupt Enable (nIEN) bit */
    outb(REG_DEV_CTRL, 0);
    /* Load required parameters in the Command Block Registers */
    outb(REG_FEATURES, cmd->features);
    outb(REG_NSECTOR, cmd->count);
    outb(REG_LBA_LOW, cmd->lba_low);
    outb(REG_LBA_MID, cmd->lba_mid);
    outb(REG_LBA_HIGH, cmd->lba_high);
    outb(REG_DEVICE, cmd->device);
    /* Write the command code to the Command Register */
    outb(REG_CMD, cmd->command);
}

/*****************************************************************************
 *                                interrupt_wait
 *****************************************************************************/
/**
 * <Ring 1> Wait until a disk interrupt occurs.
 *
 *****************************************************************************/
static void interrupt_wait() {
    while (hd_int_waiting_flag) {}
    hd_int_waiting_flag = 1;
}

/*****************************************************************************
 *                                waitfor
 *****************************************************************************/
/**
 * <Ring 1> Wait for a certain status.
 *
 * @param mask    Status mask.
 * @param val     Required status.
 * @param timeout Timeout in milliseconds.
 *
 * @return One if sucess, zero if timeout.
 *****************************************************************************/
static int waitfor(int mask, int val, int timeout) {
    int t0 = do_get_ticks();
    while (true) {
        int t1 = do_get_ticks();
        if (clock_from_sysclk(t1 - t0) >= timeout) { break; }
        if ((inb(REG_STATUS) & mask) == val) { return 1; }
    }
    return 0;
}

/*****************************************************************************
 *                                hd_handler
 *****************************************************************************/
/**
 * <Ring 0> Interrupt handler.
 *
 * @param irq  IRQ nr of the disk interrupt.
 *****************************************************************************/
static void hd_handler(int irq) {
    /*
     * Interrupts are cleared when the host
     *   - reads the Status Register,
     *   - issues a reset, or
     *   - writes to the Command Register.
     */
    hd_status = inb(REG_STATUS);
    inform_int();

    /* There is two stages - in kernel intializing or in process running.
     * Some operation shouldn't be valid in kernel intializing stage.
     * added by xw, 18/6/1
     */
    if (kstate_on_init) { return; }

    // some operation only for process

    return;
}

/*****************************************************************************
 *                                inform_int
 *****************************************************************************/
static void inform_int() {
    hd_int_waiting_flag = 0;
    return;
}
