#include <stdio.h>
#include <pb.h>
#include <uuid.h>

typedef union 
{
    uint8_t raw[16];
    struct
    {
        uint32_t time_low;
        uint16_t time_mid;
        uint16_t time_hi_and_version;
        uint8_t clock_seq_hi_and_res;
        uint8_t clock_seq_low;
        uint8_t node[6];
    } __packed;
} uuid_t;

uint32_t uuid_to_string(uint8_t *uuid, char *out)
{
    uuid_t *u = (uuid_t *) uuid;

    snprintf(out,40, "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        u->time_low,
        u->time_mid,
        u->time_hi_and_version,
        u->clock_seq_hi_and_res,
        u->clock_seq_low,
        u->node[0],
        u->node[1],
        u->node[2],
        u->node[3],
        u->node[4],
        u->node[5]);

    

    return PB_OK;
}