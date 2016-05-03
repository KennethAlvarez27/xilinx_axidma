/**
 * @file version_portability.h
 * @date Saturday, April 09, 2016 at 10:08:33 AM EDT
 * @author Brandon Perez (bmperez)
 * @author Jared Choi (jaewonch)
 *
 * This header file deals with portability for the module across the 3.x and 4.x
 * versions of Linux. This file checks the version and updates the definitions
 * appropiately.
 *
 * @bug No known bugs.
 **/

#ifndef VERSION_PORTABILITY_H_
#define VERSION_PORTABILITY_H_

#include <linux/version.h>          // Linux version macros

/*----------------------------------------------------------------------------
 * Linux 4.x Compatbility
 *----------------------------------------------------------------------------*/

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)

#warning("This driver only supports Linux 3.x and 4.x versions. Linux 5.x " \
          "version and greater is untested")

#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)

#include <linux/dma/xilinx_dma.h>   // Xilinx DMA config structures (diff path)
#include <linux/dmaengine.h>        // Definitions for DMA structures and types

// DMA_SUCCESS was renamed to DMA_COMPLETE (indicates a DMA transaction is done)
#define DMA_SUCCESS                 DMA_COMPLETE

// The skip destination unmap DMA control option was removed in 4.x
#define DMA_COMPL_SKIP_DEST_UNMAP   0

/* The xilinx_dma_config structure was removed in 4.x, so we create a dummy one
 * here. AXI DMA no longer implements slave config, so this is ignored. */
struct xilinx_dma_config {
    int dummy;
};

/* Setup the config structure for DMA. In 4.x, the DMA config structure was
 * removed, so we can safely just set it to zero here. */
static inline
void axidma_setup_dma_config(struct xilinx_dma_config *dma_config,
                             enum dma_transfer_direction direction)
{
    // Silence the compiler, in case this function is not used
    (void)axidma_setup_dma_config;

    dma_config->dummy = 0;
    return;
}

/* Setup the config structure for VDMA. In 4.x, the VDMA config no longer has
 * vsize, hsize, and stride fields. */
static inline
void axidma_setup_vdma_config(struct xilinx_vdma_config *dma_config, int width,
                              int height, int depth)
{
    // Silence the compiler, in case this function is not used
    (void)axidma_setup_vdma_config;

    dma_config->frm_dly = 0;            // Number of frames to delay
    dma_config->gen_lock = 0;           // No genlock, VDMA runs freely
    dma_config->master = 0;             // VDMA is the genlock master
    dma_config->frm_cnt_en = 0;         // No interrupts based on frame count
    dma_config->park = 0;               // Continuously process all frames
    dma_config->park_frm = 0;           // Frame to stop (park) at (N/A)
    dma_config->coalesc = 0;            // No transfer completion interrupts
    dma_config->delay = 0;              // Disable the delay counter interrupt
    dma_config->reset = 0;              // Don't reset the channel
    dma_config->ext_fsync = 0;          // VDMA handles synchronizes itself
    return;
}

/*----------------------------------------------------------------------------
 * Linux 3.x Compatibility
 *----------------------------------------------------------------------------*/

#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3,0,0)

#include <linux/amba/xilinx_dma.h>  // Xilinx DMA config structures
#include <linux/dmaengine.h>        // Definitions for DMA structures and types

// Setup the config structure for DMA
static inline
void axidma_setup_dma_config(struct xilinx_dma_config *dma_config,
                             enum dma_transfer_direction direction)
{
    // Silence the compiler, in case this function is not used
    (void)axidma_setup_dma_config;

    dma_config->direction = direction;  // Either to memory or from memory
    dma_config->coalesc = 1;            // Interrupt for one transfer completion
    dma_config->delay = 0;              // Disable the delay counter interrupt
    dma_config->reset = 0;              // Don't reset the DMA engine
    return;
}

// Setup the config structure for VDMA
static inline
void axidma_setup_vdma_config(struct xilinx_vdma_config *dma_config, int width,
                              int height, int depth)
{
    // Silence the compiler, in case this function is not used
    (void)axidma_setup_vdma_config;

    dma_config->vsize = height;         // Height of the image (in lines)
    dma_config->hsize = width * depth;  // Width of the image (in bytes)
    dma_config->stride = width * depth; // Number of bytes to process per line
    dma_config->frm_dly = 0;            // Number of frames to delay
    dma_config->gen_lock = 0;           // No genlock, VDMA runs freely
    dma_config->master = 0;             // VDMA is the genlock master
    dma_config->frm_cnt_en = 0;         // No interrupts based on frame count
    dma_config->park = 0;               // Continuously process all frames
    dma_config->park_frm = 0;           // Frame to stop (park) at (N/A)
    dma_config->coalesc = 0;            // No transfer completion interrupts
    dma_config->delay = 0;              // Disable the delay counter interrupt
    dma_config->reset = 0;              // Don't reset the channel
    dma_config->ext_fsync = 0;          // VDMA handles synchronizes itself
    return;
}

#else

#error("This driver only supports Linux 3.x and 4.x versions. Linux 2.x " \
       "version and lower is untested.")

#endif /* LINUX_VERSION_CODE */

#endif /* VERSION_PORTABILITY_H_ */
