/*
 * Copyright (c) 2020 Embedded Planet
 * Copyright (c) 2020 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"

#include "bootutil/bootutil.h"
#include "bootutil/image.h"
#include "FlashIAP/FlashIAPBlockDevice.h"
#include "blockdevice/SlicingBlockDevice.h"
#include "drivers/InterruptIn.h"

#if COMPONENT_SPIF
#include "SPIFBlockDevice.h"
#endif

#if COMPONENT_NUSD
#include "NuSDFlashSimBlockDevice.h"
#endif

#define TRACE_GROUP "main"
#include "mbed-trace/mbed_trace.h"

mbed::BlockDevice* get_secondary_bd(void) {
#if TARGET_NUVOTON
#   if TARGET_NUMAKER_IOT_M467_FLASHIAP || \
       TARGET_NUMAKER_IOT_M467_FLASHIAP_TEST
    static FlashIAPBlockDevice fbd(MCUBOOT_PRIMARY_SLOT_START_ADDR + MCUBOOT_SLOT_SIZE, MCUBOOT_SLOT_SIZE);
    return &fbd;
#   elif TARGET_NUMAKER_IOT_M467_FLASHIAP_DUALBANK || \
       TARGET_NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST
    static FlashIAPBlockDevice fbd(0x80000, MCUBOOT_SLOT_SIZE);
    return &fbd;
#   elif TARGET_NUMAKER_IOT_M467_SPIF || \
         TARGET_NUMAKER_IOT_M467_SPIF_TEST
    /* Whether or not QE bit is set, explicitly disable WP/HOLD functions for safe. */
    static mbed::DigitalOut onboard_spi_wp(PI_13, 1);
    static mbed::DigitalOut onboard_spi_hold(PI_12, 1);
    static SPIFBlockDevice spif_bd(MBED_CONF_SPIF_DRIVER_SPI_MOSI,
                                   MBED_CONF_SPIF_DRIVER_SPI_MISO,
                                   MBED_CONF_SPIF_DRIVER_SPI_CLK,
                                   MBED_CONF_SPIF_DRIVER_SPI_CS);
    static mbed::SlicingBlockDevice sliced_bd(&spif_bd, 0x0, MCUBOOT_SLOT_SIZE);
    return &sliced_bd;
#   elif TARGET_NUMAKER_IOT_M487_SPIF || \
         TARGET_NUMAKER_IOT_M487_SPIF_TEST
    /* Whether or not QE bit is set, explicitly disable WP/HOLD functions for safe. */
    static mbed::DigitalOut onboard_spi_wp(PC_5, 1);
    static mbed::DigitalOut onboard_spi_hold(PC_4, 1);
    static SPIFBlockDevice spif_bd(MBED_CONF_SPIF_DRIVER_SPI_MOSI,
                                   MBED_CONF_SPIF_DRIVER_SPI_MISO,
                                   MBED_CONF_SPIF_DRIVER_SPI_CLK,
                                   MBED_CONF_SPIF_DRIVER_SPI_CS);
    static mbed::SlicingBlockDevice sliced_bd(&spif_bd, 0x0, MCUBOOT_SLOT_SIZE);
    return &sliced_bd;
#   elif TARGET_NUMAKER_IOT_M467_NUSD || \
         TARGET_NUMAKER_IOT_M467_NUSD_TEST || \
         TARGET_NUMAKER_IOT_M487_NUSD || \
         TARGET_NUMAKER_IOT_M487_NUSD_TEST
    /* For NUSD, use the flash-simulate variant to fit MCUboot flash map backend. */
    static NuSDFlashSimBlockDevice nusd_flashsim;
    static mbed::SlicingBlockDevice sliced_bd(&nusd_flashsim, 0x0, MCUBOOT_SLOT_SIZE);
    return &sliced_bd;
#   else
#   error("Target not support: Block device for secondary slot")
#   endif
#else
    mbed::BlockDevice* default_bd = mbed::BlockDevice::get_default_instance();
    static mbed::SlicingBlockDevice sliced_bd(default_bd, 0x0, MCUBOOT_SLOT_SIZE);
    return &sliced_bd;
#endif
}

/* For Nuvoton targets, wake up from sleep through GPIO interrupt
 *
 * For Nuvoton targets, GPIO interrupt must enable to wake up from sleep.
 * Due to active low, 'fall' rather than 'rise' is chosen to escape from
 * sleep-loop smoothly.
 */
#if TARGET_NUVOTON
void press_handler()
{
    /* N/A */
}
#endif

int main()
{
    // Enable traces from relevant trace groups
    mbed_trace_init();
    mbed_trace_include_filters_set("main,MCUb,BL");

#if !HAS_IMAGE_CONFIRMED
    /**
     *  Do whatever is needed to verify the firmware is okay
     *  (eg: self test, connect to server, etc)
     *
     *  And then mark that the update succeeded
     */
    //run_self_test();
    int ret = boot_set_confirmed();
    if (ret == 0) {
        tr_info("Boot confirmed");
    } else {
        tr_error("Failed to confirm boot: %d", ret);
    }
#else
    // Image should have been marked as OK through imgtool --confirm parameter in signing,
    // or boot_set_pending(true) in previous boot. Otherwise, it will roll back in next boot.
    int ret = 0;
#endif

    InterruptIn btn(DEMO_BUTTON);

#if TARGET_NUVOTON
     btn.fall(&press_handler);
#endif

    // Get the current version from the mcuboot header information
    const struct image_header *header = (const struct image_header *) MCUBOOT_PRIMARY_SLOT_START_ADDR;
    if (header->ih_magic == IMAGE_MAGIC) {
        tr_info("Hello version %d.%d.%d+%lu",
                header->ih_ver.iv_major,
                header->ih_ver.iv_minor,
                header->ih_ver.iv_revision,
                header->ih_ver.iv_build_num);
    } else {
        tr_error("Invalid MCUboot header magic");
    }

    // Erase secondary slot
    // On the first boot, the secondary BlockDevice needs to be clean
    // If the first boot is not normal, please run the erase step, then reboot

    tr_info("> Press button to erase secondary slot");

#if DEMO_BUTTON_ACTIVE_LOW
    while (btn) {
#else
    while (!btn) {
#endif
        sleep();
    }

    BlockDevice *secondary_bd = get_secondary_bd();
    ret = secondary_bd->init();
    if (ret == 0) {
        tr_info("Secondary BlockDevice inited");
    } else {
        tr_error("Cannot init secondary BlockDevice: %d", ret);
    }

    tr_info("Erasing secondary BlockDevice...");
    ret = secondary_bd->erase(0, secondary_bd->size());
    if (ret == 0) {
        tr_info("Secondary BlockDevice erased");
    } else {
        tr_error("Cannot erase secondary BlockDevice: %d", ret);
    }

    tr_info("> Press button to copy update image to secondary BlockDevice");

#if DEMO_BUTTON_ACTIVE_LOW
    while (btn) {
#else
    while (!btn) {
#endif
        sleep();
    }

#if HAS_OTA_DL_SIM_SLOT
    // Copy the update image from internal flash to secondary BlockDevice
    // This is a "hack" that requires you to preload the update image into `mcuboot.primary-slot-address` + 0x40000

#if TARGET_NUVOTON
#   if TARGET_NUMAKER_IOT_M467_FLASHIAP_TEST
    // Update image preload region: Immediately following primary/secondary slots
    FlashIAPBlockDevice fbd(MCUBOOT_PRIMARY_SLOT_START_ADDR + MCUBOOT_SLOT_SIZE * 2, MCUBOOT_SLOT_SIZE);
#   elif TARGET_NUMAKER_IOT_M467_FLASHIAP_DUALBANK_TEST
    // Update image preload region: Immediately following secondary slot
    FlashIAPBlockDevice fbd(0x80000 + MCUBOOT_SLOT_SIZE, MCUBOOT_SLOT_SIZE);
#   elif TARGET_NUMAKER_IOT_M467_SPIF_TEST || \
         TARGET_NUMAKER_IOT_M467_NUSD_TEST || \
         TARGET_NUMAKER_IOT_M487_SPIF_TEST || \
         TARGET_NUMAKER_IOT_M487_NUSD_TEST
    // Update image preload region: Immediately following primary slot
    FlashIAPBlockDevice fbd(MCUBOOT_PRIMARY_SLOT_START_ADDR + MCUBOOT_SLOT_SIZE, MCUBOOT_SLOT_SIZE);
#   else
#   error("Target not support: Prepare for update candidate")
#   endif
#else
    FlashIAPBlockDevice fbd(MCUBOOT_PRIMARY_SLOT_START_ADDR + 0x40000, 0x20000);
#endif
    ret = fbd.init();
    if (ret == 0) {
        tr_info("FlashIAPBlockDevice inited");
    } else {
        tr_error("Cannot init FlashIAPBlockDevice: %d", ret);
    }

    static uint8_t buffer[0x1000];
#if TARGET_NUVOTON
    for (size_t offset = 0; offset < MCUBOOT_SLOT_SIZE; offset+= sizeof(buffer)) {
#else
    for (size_t offset = 0; offset < 0x20000; offset+= sizeof(buffer)) {
#endif
        ret = fbd.read(buffer, offset, sizeof(buffer));
        if (ret != 0) {
            tr_error("Failed to read FlashIAPBlockDevice at offset %u", offset);
        }
        ret = secondary_bd->program(buffer, offset, sizeof(buffer));
        if (ret != 0) {
            tr_error("Failed to program secondary BlockDevice at offset %u", offset);
        }
    }
#else
    // FIXME: Copy update image to secondary slot in custom way
    tr_info("Copy update image to secondary slot in custom way...");
    ThisThread::sleep_for(2000ms);
    tr_info("Copy update image to secondary slot in custom way...DONE");
#endif

#if !HAS_IMAGE_TRAILER && !HAS_IMAGE_CONFIRMED
    // Activate the image in the secondary BlockDevice

    tr_info("> Image copied to secondary BlockDevice, press button to activate");

#if DEMO_BUTTON_ACTIVE_LOW
    while (btn) {
#else
    while (!btn) {
#endif
        sleep();
    }

    ret = boot_set_pending(false);
    if (ret == 0) {
        tr_info("> Secondary image pending, reboot to update");
    } else {
        tr_error("Failed to set secondary image pending: %d", ret);
    }
#else
    // Image should have had trailer/confirmed through imgtool --pad/--confirm parameter in signing.
    tr_info("> Image copied to secondary BlockDevice, reboot to update");
#endif
}
