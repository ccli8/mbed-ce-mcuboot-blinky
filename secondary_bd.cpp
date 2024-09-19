/*
 * default_bd.cpp
 *
 *  Created on: Jul 30, 2020
 *      Author: gdbeckstein
 */

#include "BlockDevice.h"

#include "SlicingBlockDevice.h"

#if COMPONENT_SPIF
#include "SPIFBlockDevice.h"
#endif

#if COMPONENT_QSPIF
#include "QSPIFBlockDevice.h"
#endif

#if COMPONENT_DATAFLASH
#include "DataFlashBlockDevice.h"
#endif

#if COMPONENT_SD
#include "SDBlockDevice.h"

#if (STATIC_PINMAP_READY)
const spi_pinmap_t static_spi_pinmap = get_spi_pinmap(MBED_CONF_SD_SPI_MOSI, MBED_CONF_SD_SPI_MISO, MBED_CONF_SD_SPI_CLK, NC);
#endif
#endif

#if DEVICE_FLASH
#include "FlashIAP/FlashIAPBlockDevice.h"
#endif

#if COMPONENT_NUSD
#include "NuSDFlashSimBlockDevice.h"
#endif

BlockDevice *BlockDevice::get_default_instance()
{
#if COMPONENT_SPIF

    static SPIFBlockDevice default_bd;

    return &default_bd;

#elif COMPONENT_QSPIF

    static QSPIFBlockDevice default_bd;

    return &default_bd;

#elif COMPONENT_DATAFLASH

    static DataFlashBlockDevice default_bd;

    return &default_bd;

#elif COMPONENT_SD

#if (STATIC_PINMAP_READY)
    static SDBlockDevice default_bd(
        static_spi_pinmap,
        MBED_CONF_SD_SPI_CS
    );
#else
    static SDBlockDevice default_bd;
#endif

    return &default_bd;

#else

    return nullptr;

#endif

}

/**
 * You can override this function to suit your hardware/memory configuration
 * By default it simply returns what is returned by BlockDevice::get_default_instance();
 */
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
         TARGET_NUMAKER_IOT_M467_SPIF_TEST || \
         TARGET_NUMAKER_IOT_M467_SPIF_BOOT80K || \
         TARGET_NUMAKER_IOT_M467_SPIF_BOOT80K_TEST
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
    // In this case, our flash is much larger than a single image so
    // slice it into the size of an image slot
    mbed::BlockDevice* default_bd = mbed::BlockDevice::get_default_instance();
    MBED_ASSERT(default_bd);
    static mbed::SlicingBlockDevice sliced_bd(default_bd, 0x0, MCUBOOT_SLOT_SIZE);
    return &sliced_bd;
#endif
}

