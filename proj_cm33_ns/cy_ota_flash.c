/*****************************************************************************
* File Name        : cy_ota_flash.c
*
* Description      : This file contains flash operations implementation
*
* Related Document : See README.md
*
*******************************************************************************
* (c) 2024-2025, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
* 
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/
/* Header file includes */
#include <stdlib.h>
#include "cy_pdl.h"
#include "cy_ota_flash.h"

#include "cycfg.h"
#include "mtb_serial_memory.h"
#include "cybsp_hw_config.h"

/**********************************************************************************************************************************
 * local defines
 **********************************************************************************************************************************/
/* This defines if External Flash (SMIF) will be used for Upgrade Slots */




#define SECTOR_ADDR                 (0x40000U)  /* Offset to the start of external memory that belongs to the sector for which size is returned. */
#define SMIF_DATA_QUAD              (0x04U)
#define DATA_WIDTH_PINS             (0x04U)
#define CY_FLASH_ERASE_SIZE         (0x40000UL) /* Erase Size for External Flash and XIP area */
#define CY_FLASH_SIZEOF_ROW         (512UL)


/**********************************************************************************************************************************
 * local variables & data
 **********************************************************************************************************************************/
static mtb_serial_memory_t sm_obj;
static cy_stc_smif_mem_context_t context;
static cy_stc_smif_mem_info_t smif_mem_info;

/**********************************************************************************************************************************
 * Internal Functions
 **********************************************************************************************************************************/
/**
 * @brief Initializes flash, QSPI flash, or any other external memory type
 *
 * @return  CY_RSLT_SUCCESS on success
 *          CY_RSLT_TYPE_ERROR on failure
 */
cy_rslt_t cy_ota_mem_init( void )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

       /* Initialize the QSPI block */
#if ((DATA_WIDTH_PINS) == (SMIF_DATA_QUAD))
    result = mtb_serial_memory_setup(&sm_obj, MTB_SERIAL_MEMORY_CHIP_SELECT_1, CYBSP_SMIF_CORE_0_XSPI_FLASH_hal_config.base, CYBSP_SMIF_CORE_0_XSPI_FLASH_hal_config.clock, &context, &smif_mem_info, &smif0BlockConfig);
    if (result != CY_RSLT_SUCCESS)
    {
        printf("\nmtb_serial_memory_setup() failed in %s : line %d", __func__, __LINE__);
    }
#else
    printf("not supported for other data width");
#endif

    if(result == CY_RSLT_SUCCESS)
    {
        printf("External Memory initialized w/ SFDP.");
    } else 
    {
        printf("External Memory initialization w/ SFDP FAILED");
    }

    return result;
}

/**
 * @brief Read from flash, QSPI flash, or any other external memory type
 *
 * @param[in]   mem_type   Memory type @ref cy_ota_mem_type_t
 * @param[in]   addr       Starting address to read from.
 * @param[out]  data       Pointer to the buffer to store the data read from the memory.
 * @param[in]   len        Number of data bytes to read.
 *
 * @return  CY_RSLT_SUCCESS on success
 *          CY_RSLT_TYPE_ERROR on failure
 */
cy_rslt_t cy_ota_mem_read( cy_ota_mem_type_t mem_type, uint32_t addr, void *data, size_t len )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    if(mem_type == CY_OTA_MEM_TYPE_EXTERNAL_FLASH)
    {
        if(addr >= CY_XIP_PORT0_S_SBUS_BASE)
        {
            addr -= CY_XIP_PORT0_S_SBUS_BASE;
        }
        else if(addr >= CY_XIP_PORT0_NS_SBUS_BASE)
        {
            addr -= CY_XIP_PORT0_NS_SBUS_BASE;
        }
        else
        {
            //Nothing to do
        }

        result = mtb_serial_memory_read(&sm_obj, addr, len, data);

        return result;
    }
    else
    {
        printf("%s() READ not supported for memory type %d\n", __func__, (int)mem_type);
        return CY_RSLT_TYPE_ERROR;
    }

}

/**
 * @brief Write to flash, QSPI flash, or any other external memory type
 *
 * @param[in]   mem_type   Memory type @ref cy_ota_mem_type_t
 * @param[in]   addr       Starting address to write to.
 * @param[in]   data        Pointer to the buffer to conaitning the write data
 * @param[in]   len        Number of data bytes to write.
 *
 * @return  CY_RSLT_SUCCESS on success
 *          CY_RSLT_TYPE_ERROR on failure
 */
static cy_rslt_t cy_ota_mem_write_row_size( cy_ota_mem_type_t mem_type, uint32_t addr, void *data, size_t len )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    if(mem_type == CY_OTA_MEM_TYPE_EXTERNAL_FLASH)
    {
        if(addr >= CY_XIP_PORT0_S_SBUS_BASE)
        {
            addr -= CY_XIP_PORT0_S_SBUS_BASE;
        }
        else if(addr >= CY_XIP_PORT0_NS_SBUS_BASE)
        {
            addr -= CY_XIP_PORT0_NS_SBUS_BASE;
        }
        else
        {
            //Nothing to do
        }

        result = mtb_serial_memory_write(&sm_obj, addr, len, data);

        return result;
    }
    else
    {
        printf("%s() Write not supported for memory type %d\n", __func__, (int)mem_type);
        return CY_RSLT_TYPE_ERROR;
    }

}

/**
 * @brief Write to flash, QSPI flash, or any other external memory type
 *
 * @param[in]   mem_type   Memory type @ref cy_ota_mem_type_t
 * @param[in]   addr       Starting address to write to.
 * @param[in]   data       Pointer to the buffer containing the data to be written.
 * @param[in]   len        Number of bytes to write.
 *
 * @return  CY_RSLT_SUCCESS on success
 *          CY_RSLT_TYPE_ERROR on failure
 */
cy_rslt_t cy_ota_mem_write( cy_ota_mem_type_t mem_type, uint32_t addr, void *data, size_t len )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /**
     * This is used if a block is < Block size to satisfy requirements
     * of flash_area_write(). "static" so it is not on the stack.
     */
    static uint8_t block_buffer[CY_FLASH_SIZEOF_ROW];
    uint32_t chunk_size = 0;

    uint32_t bytes_to_write = len;
    uint32_t curr_addr = addr;
    uint8_t *curr_src = data;

    while(bytes_to_write > 0x0U)
    {
        chunk_size = bytes_to_write;
        if(chunk_size > CY_FLASH_SIZEOF_ROW)
        {
            chunk_size = CY_FLASH_SIZEOF_ROW;
        }

        /* Is the chunk_size smaller than a flash row? */
        if((chunk_size % CY_FLASH_SIZEOF_ROW) != 0x0U)
        {
            uint32_t row_offset = 0;
            uint32_t row_base = 0;

            row_base   = (curr_addr / CY_FLASH_SIZEOF_ROW) * CY_FLASH_SIZEOF_ROW;
            row_offset = curr_addr - row_base;

            if((row_offset + chunk_size) > CY_FLASH_SIZEOF_ROW)
            {
                chunk_size = (CY_FLASH_SIZEOF_ROW - row_offset);
            }

            /* we will read a CY_FLASH_SIZEOF_ROW byte block, write the new data into the block, then write the whole block */
            result = cy_ota_mem_read(mem_type, row_base, (void *)(&block_buffer[0]), sizeof(block_buffer));
            if(result != CY_RSLT_SUCCESS)
            {
                 return CY_RSLT_TYPE_ERROR;
            }
            memcpy (&block_buffer[row_offset], curr_src, chunk_size);

            result = cy_ota_mem_write_row_size(mem_type, row_base, (void *)(&block_buffer[0]), sizeof(block_buffer));
            if(result != CY_RSLT_SUCCESS)
            {
                return CY_RSLT_TYPE_ERROR;
            }
        }
        else
        {
            result = cy_ota_mem_write_row_size(mem_type, curr_addr, curr_src, chunk_size);
            if(result != CY_RSLT_SUCCESS)
            {
                return CY_RSLT_TYPE_ERROR;
            }
        }

        curr_addr += chunk_size;
        curr_src += chunk_size;
        bytes_to_write -= chunk_size;
    }

    return CY_RSLT_SUCCESS;
}

/**
 * @brief Erase flash, QSPI flash, or any other external memory type
 *
 * @param[in]   mem_type   Memory type @ref cy_ota_mem_type_t
 * @param[in]   addr       Starting address to begin erasing.
 * @param[in]   len        Number of bytes to erase.
 *
 * @return  CY_RSLT_SUCCESS
 *          CY_RSLT_TYPE_ERROR
 */
cy_rslt_t cy_ota_mem_erase( cy_ota_mem_type_t mem_type, uint32_t addr, size_t len )
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    if(mem_type == CY_OTA_MEM_TYPE_EXTERNAL_FLASH)
    {
        uint32_t offset=0;

        if(addr >= CY_XIP_PORT0_S_SBUS_BASE)
        {
            offset= addr - CY_XIP_PORT0_S_SBUS_BASE;
        }
        else if(addr >= CY_XIP_PORT0_NS_SBUS_BASE)
        {
            offset= addr - CY_XIP_PORT0_NS_SBUS_BASE;
        }
        else
        {
            //Nothing to do
        }
        result = mtb_serial_memory_erase(&sm_obj, offset, len);

        return result;
    }
    else
    {
        printf("%s() Erase not supported for memory type %d\n", __func__, (int)mem_type);
        result = CY_RSLT_TYPE_ERROR;
        return result;
    }

}

/**
 * @brief To get page size for programming flash, QSPI flash, or any other external memory type
 *
 * @param[in]   mem_type   Memory type @ref cy_ota_mem_type_t
 * @param[in]   addr       Address that belongs to the sector for which programming page size needs to be returned.
 *
 * @return    Page size in bytes.
 */
size_t cy_ota_mem_get_prog_size ( cy_ota_mem_type_t mem_type, uint32_t addr )
{
    if(mem_type == CY_OTA_MEM_TYPE_EXTERNAL_FLASH)
    {
        return mtb_serial_memory_get_prog_size(&sm_obj, addr);
    }
    else
    {
        return 0;
    }
}

/**
 * @brief To get sector size of flash, QSPI flash, or any other external memory type
 *
 * @param[in]   mem_type   Memory type @ref cy_ota_mem_type_t
 * @param[in]   addr       Address that belongs to the sector for which sector erase size needs to be returned.
 *
 * @return    Sector size in bytes.
 */
size_t cy_ota_mem_get_erase_size ( cy_ota_mem_type_t mem_type, uint32_t addr )
{
    if(mem_type == CY_OTA_MEM_TYPE_EXTERNAL_FLASH)
    {
        return mtb_serial_memory_get_erase_size(&sm_obj, SECTOR_ADDR);
    }
    else
    {
        return 0;
    }
}
