#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define CHECK_BIT(var, pos) !!((var) & (1 << (pos)))
#define MAX_BUF_SIZE 1400

struct rtp_header {
    unsigned int version:2;     /* protocol version */
    unsigned int padding:1;     /* padding flag */
    unsigned int extension:1;   /* header extension flag */
    unsigned int cc:4;          /* CSRC count */
    unsigned int marker:1;      /* marker bit */
    unsigned int pt:7;          /* payload type */
    uint16_t sequenceNum;       /* sequence number */
    uint32_t ts;                /* timestamp */
    uint32_t ssrc;              /* synchronization source */
} __attribute__((packed));;

struct prompegFEC_header {
    uint16_t SNBase;
    uint16_t lengthRecovery;
    unsigned int extension:1;
    unsigned int ptRecovery:7;
    unsigned int mask:24;
    uint32_t tsRecovery;
    unsigned int x:1;
    unsigned int dimension:1;
    unsigned int type:3;
    unsigned int index:3;
    uint8_t offset;
    uint8_t NA;
    uint8_t SNBaseExtension;
} __attribute__((packed));;

unsigned int rtp_parse(unsigned char *raw , int rtpLength) {
    unsigned int raw_offset = 0;
    unsigned int paysize;
    /*unsigned char payload[MAX_BUF_SIZE];*/
    struct rtp_header rtp_h;

    rtp_h.version = raw[raw_offset] >> 6;
    rtp_h.padding = CHECK_BIT(raw[raw_offset], 5);
    rtp_h.extension = CHECK_BIT(raw[raw_offset], 4);
    rtp_h.cc = raw[raw_offset] & 0xFF;

    /* next byte */
    raw_offset++;

    rtp_h.marker = CHECK_BIT(raw[raw_offset], 8);
    rtp_h.pt = raw[raw_offset] & 0x7f;

    /* next byte */
    raw_offset++;

    /* Sequence number */
    rtp_h.sequenceNum = raw[raw_offset] * 256 + raw[raw_offset + 1];
    raw_offset += 2;

    /* time stamp */
    rtp_h.ts = \
        (raw[raw_offset    ] << 24) |
        (raw[raw_offset + 1] << 16) |
        (raw[raw_offset + 2] <<  8) |
        (raw[raw_offset + 3]);
    raw_offset += 4;

    /* ssrc / source identifier */
    rtp_h.ssrc = \
        (raw[raw_offset    ] << 24) |
        (raw[raw_offset + 1] << 16) |
        (raw[raw_offset + 2] <<  8) |
        (raw[raw_offset + 3]);
    raw_offset += 4;

    /* Payload size */
    paysize = (rtpLength - raw_offset);
    /*
    memset(payload, '\0', sizeof(payload));
    memcpy(&payload, raw + raw_offset, paysize);*/

    printf("   >> RTP\n");
    printf("      Version     : %i\n", rtp_h.version);
    printf("      Padding     : %i\n", rtp_h.padding);
    printf("      Extension   : %i\n", rtp_h.extension);
    printf("      CC          : %i\n", rtp_h.cc);
    printf("      Marker      : %i\n", rtp_h.marker);
    printf("      Payload Type: %i\n", rtp_h.pt);
    printf("      Sequence    : %i\n", rtp_h.sequenceNum);
    printf("      Timestamp   : %u\n", rtp_h.ts);
    printf("      SSRC        : %u\n", rtp_h.ssrc);
    printf("      Payload Size: %u\n", paysize);

    return raw_offset;
}
