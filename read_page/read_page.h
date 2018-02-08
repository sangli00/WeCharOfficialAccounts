#ifndef __READ_PAGE_H_
#define __READ_PAGE_H_
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

#define FLEXIBLE_ARRAY_MEMBER
#define BLOCK_SIZE 8192
typedef uintptr_t Datum;

typedef unsigned char uint8;    /* == 8 bits */
typedef unsigned short uint16;    /* == 16 bits */
typedef unsigned int uint32;    /* == 32 bits */
typedef signed char int8;        /* == 8 bits */
typedef signed short int16;        /* == 16 bits */
typedef signed int int32;        /* == 32 bits */
typedef long int int64;
typedef uint16 LocationIndex;
typedef uint32 TransactionId;
typedef uint32 CommandId;
typedef unsigned int Oid;
typedef uint16 OffsetNumber;
typedef uint8 bits8;            /* >= 8 bits */
typedef uint16 bits16;            /* >= 16 bits */
typedef uint32 bits32;            /* >= 32 bits */

typedef char *Pointer;
#define DatumGetPointer(X) ((Pointer) (X))

#ifndef bool
typedef char bool;
#endif

#ifndef true
#define true    ((bool) 1)
#endif

#ifndef false
#define false    ((bool) 0)
#endif

typedef struct ItemIdData
{
    unsigned    lp_off:15,        /* offset to tuple (from start of page) */
                lp_flags:2,        /* state of item pointer, see below */
                lp_len:15;        /* byte length of tuple */
} ItemIdData;

typedef ItemIdData *ItemId;

typedef struct
{
    uint32        xlogid;            /* high bits */
    uint32        xrecoff;        /* low bits */
} PageXLogRecPtr;

typedef struct PageHeaderData
{
    /* XXX LSN is member of *any* block, not only page-organized ones */
    PageXLogRecPtr pd_lsn;        /* LSN: next byte after last byte of xlog
                                 * record for last change to this page */
    uint16        pd_checksum;    /* checksum */
    uint16        pd_flags;        /* flag bits, see below */
    LocationIndex pd_lower;        /* offset to start of free space */
    LocationIndex pd_upper;        /* offset to end of free space */
    LocationIndex pd_special;    /* offset to start of special space */
    uint16        pd_pagesize_version;
    TransactionId pd_prune_xid; /* oldest prunable XID, or zero if none */
    ItemIdData    pd_linp[FLEXIBLE_ARRAY_MEMBER]; /* line pointer array */
} PageHeaderData;

typedef PageHeaderData *PageHeader;

#define SizeOfPageHeaderData (offsetof(PageHeaderData, pd_linp))

#define PageGetMaxOffsetNumber(page) \
        (((PageHeader) (page))->pd_lower <= SizeOfPageHeaderData ? 0 : \
         ((((PageHeader) (page))->pd_lower - SizeOfPageHeaderData) \
          / sizeof(ItemIdData)))

/**TUPLE */
typedef struct HeapTupleFields
{
    TransactionId t_xmin;        /* inserting xact ID */
    TransactionId t_xmax;        /* deleting or locking xact ID */

    union
    {
        CommandId    t_cid;        /* inserting or deleting command ID, or both */
        TransactionId t_xvac;    /* old-style VACUUM FULL xact ID */
    }            t_field3;
} HeapTupleFields;

typedef struct DatumTupleFields
{
    int32        datum_len_;        /* varlena header (do not touch directly!) */

    int32        datum_typmod;    /* -1, or identifier of a record type */

    Oid            datum_typeid;    /* composite type OID, or RECORDOID */

    /*
 *  *      * Note: field ordering is chosen with thought that Oid might someday
 *   *           * widen to 64 bits.
 *    *                */
} DatumTupleFields;

typedef struct BlockIdData
{
    uint16        bi_hi;
    uint16        bi_lo;
} BlockIdData;

typedef BlockIdData *BlockId;

typedef struct ItemPointerData
{
    BlockIdData ip_blkid;
    OffsetNumber ip_posid;
}
/* If compiler understands packed and aligned pragmas, use those */
/*
 * #if defined(pg_attribute_packed) && defined(pg_attribute_aligned)
 * pg_attribute_packed()
 * pg_attribute_aligned(2)
 * #endif*/
ItemPointerData;

struct HeapTupleHeaderData
{
    union
    {
        HeapTupleFields t_heap;
        DatumTupleFields t_datum;
    }            t_choice;

    ItemPointerData t_ctid;        /* current TID of this or newer tuple (or a
                                 * speculative insertion token) */

    /* Fields below here must match MinimalTupleData! */

    uint16        t_infomask2;    /* number of attributes + various flags */

    uint16        t_infomask;        /* various flag bits, see below */

    uint8        t_hoff;            /* sizeof header incl. bitmap, padding */

    /* ^ - 23 bytes - ^ */

    bits8        t_bits[FLEXIBLE_ARRAY_MEMBER];    /* bitmap of NULLs */

    /* MORE DATA FOLLOWS AT END OF STRUCT */
};

typedef struct HeapTupleHeaderData HeapTupleHeaderData;

typedef HeapTupleHeaderData *HeapTupleHeader;

typedef struct HeapTupleData
{
    uint32        t_len;            /* length of *t_data */
    ItemPointerData t_self;        /* SelfItemPointer */
    Oid            t_tableOid;        /* table the tuple came from */
    HeapTupleHeader t_data;        /* -> tuple header and data */
} HeapTupleData;

#define HEAPTUPLESIZE    MAXALIGN(sizeof(HeapTupleData))

/*VARDATA*/
typedef enum vartag_external
{
    VARTAG_INDIRECT = 1,
    VARTAG_EXPANDED_RO = 2,
    VARTAG_EXPANDED_RW = 3,
    VARTAG_ONDISK = 18
} vartag_external;

struct varlena
{
    char        vl_len_[4];        /* Do not touch this field directly! */
    char        vl_dat[FLEXIBLE_ARRAY_MEMBER];    /* Data content is here */
};

typedef struct
{
    uint8        va_header;
    char        va_data[FLEXIBLE_ARRAY_MEMBER]; /* Data begins here */
} varattrib_1b;

typedef struct
{
    uint8        va_header;        /* Always 0x80 or 0x01 */
    uint8        va_tag;            /* Type of datum */
    char        va_data[FLEXIBLE_ARRAY_MEMBER]; /* Type-specific data */
} varattrib_1b_e;

typedef struct varatt_indirect
{
    struct varlena *pointer;    /* Pointer to in-memory varlena */
}            varatt_indirect;

typedef struct varatt_external
{
    int32        va_rawsize;        /* Original data size (includes header) */
    int32        va_extsize;        /* External saved size (doesn't) */
    Oid            va_valueid;        /* Unique ID of value within TOAST table */
    Oid            va_toastrelid;    /* RelID of TOAST table containing it */
}            varatt_external;

typedef union
{
    struct                        /* Normal varlena (4-byte length) */
    {
        uint32        va_header;
        char        va_data[FLEXIBLE_ARRAY_MEMBER];
    }            va_4byte;
    struct                        /* Compressed-in-line format */
    {
        uint32        va_header;
        uint32        va_rawsize; /* Original data size (excludes header) */
        char        va_data[FLEXIBLE_ARRAY_MEMBER]; /* Compressed data */
    }            va_compressed;
} varattrib_4b;

typedef struct varlena bytea;
typedef struct varlena text;
typedef struct varlena BpChar;    /* blank-padded char, ie SQL char(n) */
typedef struct varlena VarChar; /* var-length char, ie SQL varchar(n) */

#define VARATT_IS_COMPRESSED(PTR)            VARATT_IS_4B_C(PTR)
#define VARATT_IS_4B_C(PTR) \
    ((((varattrib_1b *) (PTR))->va_header & 0x03) == 0x02)

#define VARATT_IS_EXTERNAL(PTR)                VARATT_IS_1B_E(PTR)
#define VARATT_IS_1B_E(PTR) \
    ((((varattrib_1b *) (PTR))->va_header) == 0x01)

#define VARATT_IS_1B(PTR) \
    ((((varattrib_1b *) (PTR))->va_header & 0x01) == 0x01)

#define VARSIZE_1B(PTR) \
    ((((varattrib_1b *) (PTR))->va_header >> 1) & 0x7F)


#define VARSIZE_ANY_EXHDR(PTR) \
    (VARATT_IS_1B_E(PTR) ? VARSIZE_EXTERNAL(PTR)-VARHDRSZ_EXTERNAL : \
     (VARATT_IS_1B(PTR) ? VARSIZE_1B(PTR)-VARHDRSZ_SHORT : \
      VARSIZE_4B(PTR)-VARHDRSZ))

#define TrapMacro(condition, errorType) (true)

#define VARTAG_IS_EXPANDED(tag) \
    (((tag) & ~1) == VARTAG_EXPANDED_RO)

#define VARTAG_SIZE(tag) \
    ((tag) == VARTAG_INDIRECT ? sizeof(varatt_indirect) : \
     VARTAG_IS_EXPANDED(tag) ? (8)  : \
     (tag) == VARTAG_ONDISK ? sizeof(varatt_external) : \
     TrapMacro(true, "unrecognized TOAST vartag"))

#define VARTAG_1B_E(PTR) \
    (((varattrib_1b_e *) (PTR))->va_tag)
#define VARTAG_EXTERNAL(PTR)                VARTAG_1B_E(PTR)

#define VARHDRSZ_EXTERNAL        offsetof(varattrib_1b_e, va_data)
#define VARHDRSZ_SHORT            offsetof(varattrib_1b, va_data)
#define VARHDRSZ        ((int32) sizeof(int32))


#define VARSIZE_EXTERNAL(PTR)                (VARHDRSZ_EXTERNAL + VARTAG_SIZE(VARTAG_EXTERNAL(PTR)))

#define VARSIZE_4B(PTR) \
    ((((varattrib_4b *) (PTR))->va_4byte.va_header >> 2) & 0x3FFFFFFF)

#define VARDATA_1B(PTR)        (((varattrib_1b *) (PTR))->va_data)
#define VARDATA_4B(PTR)        (((varattrib_4b *) (PTR))->va_4byte.va_data)

#define VARDATA_ANY(PTR) \
     (VARATT_IS_1B(PTR) ? VARDATA_1B(PTR) : VARDATA_4B(PTR))

#define MAXIMUM_ALIGNOF 8
#define TYPEALIGN(ALIGNVAL,LEN)  \
    (((uintptr_t) (LEN) + ((ALIGNVAL) - 1)) & ~((uintptr_t) ((ALIGNVAL) - 1)))
#define MAXALIGN(LEN)            TYPEALIGN(MAXIMUM_ALIGNOF, (LEN))


#endif
