#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define CHECK_BIT(var, pos) !!((var) & (1 << (pos)))
#define MAX_BUF_SIZE 1400

struct rtp_header {
    unsigned int version:2;     /* protocol version */              /*always same*/
    unsigned int padding:1;     /* padding flag */                  /*always same*/
    unsigned int extension:1;   /* header extension flag */         /*always same*/
    unsigned int cc:4;          /* CSRC count */                    /*always same*/
    unsigned int marker:1;      /* marker bit */                    /*always same*/
    unsigned int pt:7;          /* payload type */                  /*GLOBAL INFO*/
    uint16_t sequenceNum;       /* sequence number */               /*IMPORTANT*/
    uint32_t ts;                /* timestamp */                     /*IMPORTANT*/
    uint32_t ssrc;              /* synchronization source */        /*GLOBAL INFO*/
} __attribute__((packed));;

struct prompegFEC_header {
    uint16_t SNBase;            /*SNBase Low bit*/                  /*IMPORTANT*/
    uint16_t lengthRecovery;    /*length recovery*/                 /*always same*/
    unsigned int extension:1;   /*extension*/                       /*always same*/
    unsigned int ptRecovery:7;  /*payload type recovery*/           /*always same*/
    unsigned int mask:24;       /*mask bit*/                        /*always same*/
    uint32_t tsRecovery;        /*timestamp recovery*/              /*IMPORTANT*/
    unsigned int x:1;           /*X bit*/                           /*always same*/
    unsigned int dimension:1;   /*dimension, Col=0 Row=1*/
    unsigned int type:3;        /*FEC type*/                        /*always same*/
    unsigned int index:3;       /*index*/                           /*always same*/
    uint8_t offset;             /*offset, Col=L Row=1*/
    uint8_t NA;                 /*NA, Col=D Row=L*/
    uint8_t SNBaseExtension;    /*SNBase extension bit*/            /*always same*/
} __attribute__((packed));;


unsigned int fec_parse(unsigned char *raw , int fecLength) {
    unsigned int raw_offset = 0;
    unsigned int paysize;

    struct prompegFEC_header fec_h;

    fec_h.SNBase = \
        (raw[raw_offset    ] <<  8) |
        (raw[raw_offset + 1]);
    raw_offset += 2;

    fec_h.lengthRecovery = \
        (raw[raw_offset    ] <<  8) |
        (raw[raw_offset + 1]);
    raw_offset += 2;

    fec_h.extension = CHECK_BIT(raw[raw_offset] , 7);
    fec_h.ptRecovery = raw[raw_offset] & 0x7f;
    raw_offset ++;

    fec_h.mask = \
        (raw[raw_offset    ] << 16) |
        (raw[raw_offset + 1] <<  8) |
        (raw[raw_offset + 2]);
    raw_offset += 3;

    fec_h.tsRecovery = \
        (raw[raw_offset    ] << 24) |
        (raw[raw_offset + 1] << 16) |
        (raw[raw_offset + 2] <<  8) |
        (raw[raw_offset + 3]);
    raw_offset += 4;

    fec_h.x = CHECK_BIT(raw[raw_offset] , 7);
    fec_h.dimension = CHECK_BIT(raw[raw_offset] , 6);
    fec_h.type = raw[raw_offset] & 0x38;
    fec_h.index = raw[raw_offset] & 0x07;
    raw_offset++;

    fec_h.offset = raw[raw_offset];
    raw_offset++;

    fec_h.NA = raw[raw_offset];
    raw_offset++;

    fec_h.SNBaseExtension = raw[raw_offset];
    raw_offset++;

    paysize = fecLength - raw_offset;

    printf("   >> FEC\n");
    printf("  SNBase          : %i\n" , fec_h.SNBase);
    printf("  Length Recovery : %i\n" , fec_h.lengthRecovery);
    printf("  Extension       : %i\n" , fec_h.extension);
    printf("  PT Recovery     : %i\n" , fec_h.ptRecovery);
    printf("  Mask            : %i\n" , fec_h.mask);
    printf("  TS Recovery     : %i\n" , fec_h.tsRecovery);
    printf("  X               : %i\n" , fec_h.x);
    printf("  D               : %u\n" , fec_h.dimension);
    printf("  Type            : %u\n" , fec_h.type);
    printf("  Index           : %u\n" , fec_h.index);
    printf("  Offset          : %u\n" , fec_h.offset);
    printf("  NA              : %u\n" , fec_h.NA);
    printf("  SNBase Extension: %u\n" , fec_h.SNBaseExtension);
    printf("  Payload Size    : %u\n" , paysize);

    return raw_offset;
}

unsigned int rtp_parse(unsigned char *raw , int rtpLength) {
    unsigned int raw_offset = 0;
    unsigned int paysize;
    /*unsigned char payload[MAX_BUF_SIZE];*/
    struct rtp_header rtp_h;

    rtp_h.version = raw[raw_offset] >> 6;
    rtp_h.padding = CHECK_BIT(raw[raw_offset] , 5);
    rtp_h.extension = CHECK_BIT(raw[raw_offset] , 4);
    rtp_h.cc = raw[raw_offset] & 0x0F;
    raw_offset++;

    rtp_h.marker = CHECK_BIT(raw[raw_offset] , 7);
    rtp_h.pt = raw[raw_offset] & 0x7f;
    raw_offset++;

    rtp_h.sequenceNum = \
        (raw[raw_offset    ] <<  8) |
        (raw[raw_offset + 1]);
    raw_offset += 2;

    rtp_h.ts = \
        (raw[raw_offset    ] << 24) |
        (raw[raw_offset + 1] << 16) |
        (raw[raw_offset + 2] <<  8) |
        (raw[raw_offset + 3]);
    raw_offset += 4;

    rtp_h.ssrc = \
        (raw[raw_offset    ] << 24) |
        (raw[raw_offset + 1] << 16) |
        (raw[raw_offset + 2] <<  8) |
        (raw[raw_offset + 3]);
    raw_offset += 4;

    paysize = (rtpLength - raw_offset);
    /*
    memset(payload, '\0', sizeof(payload));
    memcpy(&payload, raw + raw_offset, paysize);*/

    printf("   >> RTP\n");
    printf("      Version     : %i\n" , rtp_h.version);
    printf("      Padding     : %i\n" , rtp_h.padding);
    printf("      Extension   : %i\n" , rtp_h.extension);
    printf("      CC          : %i\n" , rtp_h.cc);
    printf("      Marker      : %i\n" , rtp_h.marker);
    printf("      Payload Type: %i\n" , rtp_h.pt);
    printf("      Sequence    : %i\n" , rtp_h.sequenceNum);
    printf("      Timestamp   : %u\n" , rtp_h.ts);
    printf("      SSRC        : %u\n" , rtp_h.ssrc);
    printf("      Payload Size: %u\n" , paysize);

    if(rtp_h.pt == 96) {
        fec_parse(raw + raw_offset , paysize);
    }

    return raw_offset;
}
