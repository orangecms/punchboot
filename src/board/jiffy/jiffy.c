/**
 * Punch BOOT
 *
 * Copyright (C) 2018 Jonas Persson <jonpe960@gmail.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */


#include <board.h>
#include <plat.h>
#include <tinyprintf.h>
#include <io.h>
#include <gpt.h>
#include <image.h>

#include <plat/imx6ul/imx_regs.h>
#include <plat/imx6ul/imx_uart.h>
#include <plat/imx6ul/ehci.h>
#include <plat/imx6ul/usdhc.h>
#include <plat/imx6ul/gpt.h>
#include <plat/imx6ul/caam.h>
#include <plat/imx6ul/ocotp.h>
#include <plat/imx6ul/hab.h>
#include "board_config.h"

const uint8_t part_type_config[] = {0xF7, 0xDD, 0x45, 0x34, 0xCC, 0xA5, 0xC6, 0x45, 
                                0xAA, 0x17, 0xE4, 0x10, 0xA5, 0x42, 0xBD, 0xB8};

const uint8_t part_type_system_a[] = {0x59, 0x04, 0x49, 0x1E, 0x6D, 0xE8, 0x4B, 0x44, 
                            0x82, 0x93, 0xD8, 0xAF, 0x0B, 0xB4, 0x38, 0xD1};

const uint8_t part_type_system_b[] = { 0x3C, 0x29, 0x85, 0x3F, 0xFB, 0xC6, 0xD0, 
                        0x42, 0x9E, 0x1A, 0xAC, 0x6B, 0x35, 0x60, 0xC3, 0x04,};


const uint8_t part_type_root_a[] = { 0x1C, 0x29, 0x85, 0x3F, 0xFB, 0xC6, 0xD0, 
                        0x42, 0x9E, 0x1A, 0xAC, 0x6B, 0x35, 0x60, 0xC3, 0x04,};

const uint8_t part_type_root_b[] = { 0x2C, 0x29, 0x85, 0x3F, 0xFB, 0xC6, 0xD0, 
                        0x42, 0x9E, 0x1A, 0xAC, 0x6B, 0x35, 0x60, 0xC3, 0x04,};

static struct ehci_device ehcidev = 
{
    .base = EHCI_PHY_BASE,
};

static struct usb_device usbdev =
{
    .platform_data = &ehcidev,
};

uint32_t board_usb_init(struct usb_device **dev)
{
    uint32_t reg;
    /* Enable USB PLL */
    /* TODO: Add reg defs */
    reg = pb_read32(0x020C8000+0x10);
    reg |= (1<<6);
    pb_write32(reg, 0x020C8000+0x10);

    /* Power up USB */
    /* TODO: Add reg defs */
    pb_write32 ((1 << 31) | (1 << 30), 0x020C9038);
    pb_write32(0xFFFFFFFF, 0x020C9008);
 

    *dev = &usbdev;
    return PB_OK;
}



/*
 * TODO: Make sure that all of these clocks are running at
 *       maximum rates
 *
 * --- Root clocks and their maximum rates ---
 *
 * ARM_CLK_ROOT                        528 MHz
 * MMDC_CLK_ROOT / FABRIC_CLK_ROOT     396 MHz
 * AXI_CLK_ROOT                        264 MHz
 * AHB_CLK_ROOT                        132 MHz
 * PERCLK_CLK_ROOT                     66  MHz
 * IPG_CLK_ROOT                        66  MHz
 * USDHCn_CLK_ROOT                     198 MHz
 *
 *
 */

uint32_t board_init(void)
{

    /* Configure UART */
    pb_write32(0, 0x020E0094);
    pb_write32(0, 0x020E0098);
    pb_write32(UART_PAD_CTRL, 0x020E0320);
    pb_write32(UART_PAD_CTRL, 0x020E0324);

    imx_uart_init(UART_PHYS);

    init_printf(NULL, &plat_uart_putc);
 

   /* Configure NAND_DATA2 as GPIO4 4 Input with PU, 
    *
    * This is used to force recovery mode
    * */
    pb_write32(5, 0x020E0188); 
    pb_write32(0x2000 | (1 << 14) | (1 << 12), 0x020E0414);

    /* Configure pinmux for usdhc1 */
    pb_write32(0, 0x020E0000+0x1C0); /* CLK MUX */
    pb_write32(0, 0x020E0000+0x1BC); /* CMD MUX */
    pb_write32(0, 0x020E0000+0x1C4); /* DATA0 MUX */
    pb_write32(0, 0x020E0000+0x1C8); /* DATA1 MUX */
    pb_write32(0, 0x020E0000+0x1CC); /* DATA2 MUX */
    pb_write32(0, 0x020E0000+0x1D0); /* DATA3 MUX */
 
    pb_write32(1, 0x020E0000+0x1A8); /* DATA4 MUX */
    pb_write32(1, 0x020E0000+0x1AC); /* DATA5 MUX */
    pb_write32(1, 0x020E0000+0x1B0); /* DATA6 MUX */
    pb_write32(1, 0x020E0000+0x1B4); /* DATA7 MUX */
    pb_write32(1, 0x020E0000+0x1A4); /* RESET MUX */

    usdhc_emmc_init();

    if (hab_secureboot_active())
        LOG_INFO("Secure boot active");
    else
        LOG_INFO("Secure boot disabled");

    if (hab_has_no_errors() == PB_OK)
        LOG_INFO("No HAB errors found");
    else
        LOG_ERR("HAB is reporting errors");


    return PB_OK;
}

uint8_t board_force_recovery(void) 
{
    uint8_t force_recovery = false;
    uint32_t boot_fuse = 0x0;


    if ( (pb_read32(0x020A8008) & (1 << 4)) == 0)
        force_recovery = true;

    ocotp_read(0, 5, &boot_fuse);
 
    if (boot_fuse != 0x0000C060) 
    {
        force_recovery = true;
        tfp_printf ("OTP not set, forcing recovery mode\n\r");
    }

    return force_recovery;
}

uint32_t board_get_uuid(uint8_t *uuid) 
{
    uint32_t *uuid_ptr = (uint32_t *) uuid;

    ocotp_read(15, 4, &uuid_ptr[0]);
    ocotp_read(15, 5, &uuid_ptr[1]);
    ocotp_read(15, 6, &uuid_ptr[2]);
    ocotp_read(15, 7, &uuid_ptr[3]);

    return PB_OK;
}

uint32_t board_get_boardinfo(struct board_info *info) 
{
    UNUSED(info);
    return PB_OK;
}


uint32_t board_write_uuid(uint8_t *uuid, uint32_t key) 
{
    uint32_t *uuid_ptr = (uint32_t *) uuid;
    uint8_t tmp_uuid[16];

    if (key != BOARD_OTP_WRITE_KEY)
        return PB_ERR;

    board_get_uuid(tmp_uuid);

    for (int i = 0; i < 16; i++) 
    {
        if (tmp_uuid[i] != 0) 
        {
            LOG_ERR ("Can't write UUID, fuses already programmed\n\r");
            return PB_ERR;
        }
    }
    ocotp_write(15, 4, uuid_ptr[0]);
    ocotp_write(15, 5, uuid_ptr[1]);
    ocotp_write(15, 6, uuid_ptr[2]);
    ocotp_write(15, 7, uuid_ptr[3]);

    return PB_OK;
}

uint32_t board_write_standard_fuses(uint32_t key) 
{
    if (key != BOARD_OTP_WRITE_KEY) 
        return PB_ERR;

    /* Enable EMMC0 BOOT */
    ocotp_write(0, 5, 0x0000c060);
    ocotp_write(0, 6, 0x00000010);
    return PB_OK;
}

uint32_t board_write_boardinfo(struct board_info *info, uint32_t key) 
{
    UNUSED(info);
    UNUSED(key);
 
    return PB_OK;
}

uint32_t board_write_gpt_tbl() 
{
    gpt_init_tbl(1, plat_get_lastlba());
    gpt_add_part(0, 1, part_type_config, "Config");
    gpt_add_part(1, 32768, part_type_system_a, "System A");
    gpt_add_part(2, 32768, part_type_system_b, "System B");

    gpt_add_part(3, 512000, part_type_root_a, "Root A");
    gpt_add_part(4, 512000, part_type_root_b, "Root B");
    return gpt_write_tbl();
}

void board_boot(struct pb_pbi *pbi)
{

    struct pb_component_hdr *dtb = 
            pb_image_get_component(pbi, PB_IMAGE_COMPTYPE_DT);

    struct pb_component_hdr *linux = 
            pb_image_get_component(pbi, PB_IMAGE_COMPTYPE_LINUX);

    LOG_INFO(" LINUX %lX, DTB %lX", linux->load_addr_low, dtb->load_addr_low);
    
    volatile uint32_t dtb_addr = dtb->load_addr_low;
    volatile uint32_t linux_addr = linux->load_addr_low;


    asm volatile(   "mov r0, #0" "\n\r"
                    "mov r1, #0xFFFFFFFF" "\n\r"
                    "mov r2, %0" "\n\r"
                    :
                    : "r" (dtb_addr));

    asm volatile(  "mov pc, %0" "\n\r"
                    :
                    : "r" (linux_addr));
}
