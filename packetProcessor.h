#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define CHECK_BIT(var, pos) !!((var) & (1 << (pos)))
/*
    ==RTP packet==
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |V=2|P|X|  CC   |M|     PT      |       sequence number         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                       timestamp(4bytes)                       |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |       synchronization source (SSRC) identifier (fixed)        |
    +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
    |                       payload(1316bytes)                      |
    |                             ....                              |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
-------------------------------------------------------------------------------
    ==FEC packet==
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                      RTP header(12bytes)                      |
    |                             ....                              |
    |                             ....                              |
    |-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |         SNBase low bits       |        Length Recovery        |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |E| PT recovery |                    Mask                       |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                           TS recovery                         |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |X|D|type |index|    Offset     |       NA      |SNBase ext bits|
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                       payload(1316bytes)                      |
    |                             ....                              |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
typedef struct rtpHeader_ {
    unsigned int version:2;     /* protocol version */              /*always same*/
    unsigned int padding:1;     /* padding flag */                  /*always same*/
    unsigned int extension:1;   /* header extension flag */         /*always same*/
    unsigned int cc:4;          /* CSRC count */                    /*always same*/

    unsigned int marker:1;      /* marker bit */                    /*always same*/
    unsigned int pt:7;          /* payload type */                                      /*GLOBAL INFO*/

    uint16_t sequenceNum;       /* sequence number */                                   /*IMPORTANT*/
    uint32_t ts;                /* timestamp */                                         /*IMPORTANT*/
    uint32_t ssrc;              /* synchronization source */                            /*GLOBAL INFO*/
} __attribute__((packed , scalar_storage_order("big-endian"))) rtpHeader_;

typedef struct fecHeader_ {
    uint16_t SNBase;            /*SNBase Low bit*/                                      /*IMPORTANT*/
    uint16_t lengthRecovery;    /*length recovery*/                 /*always same*/

    unsigned int extension:1;   /*extension*/                       /*always same*/
    unsigned int ptRecovery:7;  /*payload type recovery*/           /*always same*/

    unsigned int mask:24;       /*mask bit*/                        /*always same*/

    uint32_t tsRecovery;        /*timestamp recovery*/                                  /*IMPORTANT*/

    unsigned int x:1;           /*X bit*/                           /*always same*/
    unsigned int dimension:1;   /*dimension, Col=0 Row=1*/                              /*區分row,col*/
    unsigned int type:3;        /*FEC type*/                        /*always same*/
    unsigned int index:3;       /*index*/                           /*always same*/

    uint8_t offset;             /*offset, Col=L Row=1*/                                 /**/
    uint8_t NA;                 /*NA, Col=D Row=L*/                                     /**/
    uint8_t SNBaseExtension;    /*SNBase extension bit*/            /*always same*/
} __attribute__((packed , scalar_storage_order("big-endian"))) fecHeader_;

typedef struct rtpPacket_ {
    rtpHeader_ rtpHeader;
    uint8_t payload[1316];
} __attribute__((packed , scalar_storage_order("big-endian"))) rtpPacket_; /*1328 bytes*/

typedef struct fecPacket_ {
    rtpHeader_ rtpHeader;
    fecHeader_ fecHeader;
    uint8_t payload[1316];
} __attribute__((packed , scalar_storage_order("big-endian"))) fecPacket_; /*1344 bytes*/

void print_rtpPacket (rtpPacket_ *rtpPacket) {
    printf("   >> RTP\n");
    printf("      Version     : %i\n" , rtpPacket->rtpHeader.version);
    printf("      Padding     : %i\n" , rtpPacket->rtpHeader.padding);
    printf("      Extension   : %i\n" , rtpPacket->rtpHeader.extension);
    printf("      CC          : %i\n" , rtpPacket->rtpHeader.cc);
    printf("      Marker      : %i\n" , rtpPacket->rtpHeader.marker);
    printf("      Payload Type: %i\n" , rtpPacket->rtpHeader.pt);
    printf("      Sequence    : %i\n" , rtpPacket->rtpHeader.sequenceNum);
    printf("      Timestamp   : %u\n" , rtpPacket->rtpHeader.ts);
    printf("      SSRC        : %u\n" , rtpPacket->rtpHeader.ssrc);
}
void print_fecPacket (fecPacket_ *fecPacket) {
    printf("   >> RTP\n");
    printf("      Version     : %i\n" , fecPacket->rtpHeader.version);
    printf("      Padding     : %i\n" , fecPacket->rtpHeader.padding);
    printf("      Extension   : %i\n" , fecPacket->rtpHeader.extension);
    printf("      CC          : %i\n" , fecPacket->rtpHeader.cc);
    printf("      Marker      : %i\n" , fecPacket->rtpHeader.marker);
    printf("      Payload Type: %i\n" , fecPacket->rtpHeader.pt);
    printf("      Sequence    : %i\n" , fecPacket->rtpHeader.sequenceNum);
    printf("      Timestamp   : %u\n" , fecPacket->rtpHeader.ts);
    printf("      SSRC        : %u\n" , fecPacket->rtpHeader.ssrc);

    printf("   >> FEC\n");
    printf("  SNBase          : %i\n" , fecPacket->fecHeader.SNBase);
    printf("  Length Recovery : %i\n" , fecPacket->fecHeader.lengthRecovery);
    printf("  Extension       : %i\n" , fecPacket->fecHeader.extension);
    printf("  PT Recovery     : %i\n" , fecPacket->fecHeader.ptRecovery);
    printf("  Mask            : %i\n" , fecPacket->fecHeader.mask);
    printf("  TS Recovery     : %i\n" , fecPacket->fecHeader.tsRecovery);
    printf("  X               : %i\n" , fecPacket->fecHeader.x);
    printf("  D               : %u\n" , fecPacket->fecHeader.dimension);
    printf("  Type            : %u\n" , fecPacket->fecHeader.type);
    printf("  Index           : %u\n" , fecPacket->fecHeader.index);
    printf("  Offset          : %u\n" , fecPacket->fecHeader.offset);
    printf("  NA              : %u\n" , fecPacket->fecHeader.NA);
    printf("  SNBase Extension: %u\n" , fecPacket->fecHeader.SNBaseExtension);
}
