#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#define MAX(x, y) ((int)(x) > (int)(y) ? (int)(x) : (int)(y))

#define ACTIVE_PART_FLAG      0x80
#define DPTENT_CHS_GET_H(loc) ((loc)[0])
#define DPTENT_CHS_GET_S(loc) ((loc)[1] & 0x3f)
#define DPTENT_CHS_GET_C(loc) ((uint16_t)((loc)[1] & 0xc0) | (loc)[2])

typedef struct dptent_s {
    uint8_t  boot_flag;
    uint8_t  start_loc[3];
    uint8_t  part_type;
    uint8_t  end_loc[3];
    uint32_t lba_start;
    uint32_t total_sects;
} dptent_t;

typedef struct partent_s {
    bool     active;
    uint8_t  type;
    uint16_t start_chs[3];
    uint16_t end_chs[3];
    uint32_t start_sect;
    size_t   total_sects;
} partent_t;

void parse_dptent(partent_t *ent, dptent_t *dptent) {
    assert(ent != NULL);
    assert(dptent != NULL);
    ent->active       = dptent->boot_flag == ACTIVE_PART_FLAG;
    ent->type         = dptent->part_type;
    ent->start_chs[0] = DPTENT_CHS_GET_C(dptent->start_loc);
    ent->start_chs[1] = DPTENT_CHS_GET_H(dptent->start_loc);
    ent->start_chs[2] = DPTENT_CHS_GET_S(dptent->start_loc);
    ent->end_chs[0]   = DPTENT_CHS_GET_C(dptent->end_loc);
    ent->end_chs[1]   = DPTENT_CHS_GET_H(dptent->end_loc);
    ent->end_chs[2]   = DPTENT_CHS_GET_S(dptent->end_loc);
    ent->start_sect   = dptent->lba_start;
    ent->total_sects  = dptent->total_sects;
}

void dump_partent(char *buf, int n, partent_t *ent, bool multiline) {
    assert(ent != NULL);
    char fmtc   = multiline ? '\n' : ' ';
    int  identw = multiline ? 2 : 0;
    char sep[8] = {};
    sprintf(sep, "%c%*s", fmtc, identw, "");
    int totalfmt = sprintf(
        buf,
        "{"
        "%sactive: %s,"
        "%stype: 0x%02x,"
        "%sstart location: CHS(%d, %d, %d),"
        "%send location: CHS(%d, %d, %d),"
        "%sfirst sector: %d,"
        "%slast sector: %d,"
        "%stotal sectors: %d,%c"
        "}",
        sep,
        ent->active ? "true" : "false",
        sep,
        ent->type,
        sep,
        ent->start_chs[0],
        ent->start_chs[1],
        ent->start_chs[2],
        sep,
        ent->end_chs[0],
        ent->end_chs[1],
        ent->end_chs[2],
        sep,
        ent->start_sect,
        sep,
        MAX(ent->start_sect + ent->total_sects - 1, 0),
        sep,
        ent->total_sects,
        fmtc);
    assert(totalfmt < n);
}

void dump_partent_json(
    char *buf, int n, partent_t *ent, bool multiline, bool raw_loc) {
    assert(ent != NULL);
    char        fmtc        = multiline ? '\n' : ' ';
    int         identw      = multiline ? 2 : 0;
    uint16_t   *chs[2]      = {ent->start_chs, ent->end_chs};
    const char *slocfmt     = raw_loc ? "\"%d,%d,%d\"" : "[ %d, %d, %d ]";
    char        sloc[2][32] = {};
    char        sep[8]      = {};
    sprintf(sloc[0], slocfmt, chs[0][0], chs[0][1], chs[0][2]);
    sprintf(sloc[1], slocfmt, chs[1][0], chs[1][1], chs[1][2]);
    sprintf(sep, "%c%*s", fmtc, identw, "");
    int totalfmt = sprintf(
        buf,
        "{"
        "%s\"active\": %s,"
        "%s\"type\": \"0x%02x\","
        "%s\"start_chs\": %s,"
        "%s\"end_chs\": %s,"
        "%s\"first_sect\": %d,"
        "%s\"total_sects\": %d%c"
        "}",
        sep,
        ent->active ? "true" : "false",
        sep,
        ent->type,
        sep,
        sloc[0],
        sep,
        sloc[1],
        sep,
        ent->start_sect,
        sep,
        ent->total_sects,
        fmtc);
    assert(totalfmt < n);
}

bool arg_match(const char *arg, const char *opt, const char *lopt) {
    if (opt != NULL && strcmp(arg, opt) == 0) { return true; }
    if (lopt != NULL && strcmp(arg, lopt) == 0) { return true; }
    return false;
}

enum dump_type {
    Default,
    Json,
    Raw,
};

int main(int argc, char *argv[]) {
    const char *image     = NULL;
    bool        multiline = false;
    int         dump_type = Default;
    bool        pack      = false;
    bool        raw_loc   = false;
    bool        help      = false;

    for (int i = 1; i < argc; ++i) {
        if (i == 1) { image = argv[i]; }
        if (arg_match(argv[i], "-m", "--multiline")) {
            multiline = true;
        } else if (arg_match(argv[i], "-j", "--json")) {
            dump_type = Json;
        } else if (arg_match(argv[i], "-p", "--pack")) {
            pack = true;
        } else if (arg_match(argv[i], NULL, "--raw-loc")) {
            raw_loc = true;
        } else if (arg_match(argv[i], "-r", "--raw")) {
            dump_type = Raw;
        } else if (arg_match(argv[i], "-h", "--help")) {
            help = true;
            break;
        }
    }

    if (help) {
        printf(
            "Usage: %s <image> [option] ...\n"
            "Options:\n"
            "  -h, --help      Display this information.\n"
            "  -m, --multiline Dump in multiline mode.\n"
            "  -r, --raw       Dump raw data.\n"
            "  -j, --json      Dump json.\n"
            "  -p, --pack      Pack json result into an array.\n"
            "      --raw-loc   Represent CHS location with raw text in json.\n",
            argv[0]);
        return 0;
    }

    if (image == NULL) {
        fprintf(stderr, "usage: %s <image> [option] ...\n", argv[0]);
        return -1;
    }

    FILE *fp = fopen(image, "rb");
    if (fp == NULL) {
        fprintf(stderr, "cannot open %s\n", image);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint8_t mbr[512]  = {};
    bool    valid_mbr = true;
    do {
        if (size < 512) { break; }
        int total_rd = 0;
        if (size > 0) { total_rd = fread(mbr, 512, 1, fp); }
        if (total_rd != 512) { break; }
        if (!(mbr[510] == 0x55 && mbr[511] == 0xaa)) { break; }
        valid_mbr = true;
    } while (0);
    fclose(fp);
    if (!valid_mbr) {
        fprintf(stderr, "%s not contains a valid MBR\n", image);
        return -1;
    }

    dptent_t *ent      = (void *)&mbr[446];
    char      buf[512] = {};
    for (int i = 0; i < 4; ++i) {
        partent_t e = {};
        parse_dptent(&e, &ent[i]);
        if (dump_type == Json) {
            dump_partent_json(buf, sizeof(buf), &e, multiline, raw_loc);
            if (pack && i == 0) { putchar('['); }
            if (pack) {
                printf("%s%s", buf, i == 3 ? "" : multiline ? ",\n" : ", ");
            } else {
                puts(buf);
            }
            if (pack && i == 3) { putchar(']'); }
        } else if (dump_type == Raw) {
            printf(
                "%d %d %d %d %d %d %d %d %d %d\n",
                e.active,
                e.type,
                e.start_chs[0],
                e.start_chs[1],
                e.start_chs[2],
                e.end_chs[0],
                e.end_chs[1],
                e.end_chs[2],
                e.start_sect,
                e.total_sects);
        } else {
            dump_partent(buf, sizeof(buf), &e, multiline);
            printf("PART %d %s\n", i, buf);
        }
    }
    return 0;
}
