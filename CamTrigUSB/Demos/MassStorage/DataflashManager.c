/*
             LUFA Library
     Copyright (C) Dean Camera, 2009.
              
  dean [at] fourwalledcubicle [dot] com
      www.fourwalledcubicle.com
*/

/*
  Copyright 2009  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, and distribute this software
  and its documentation for any purpose and without fee is hereby
  granted, provided that the above copyright notice appear in all
  copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Functions to manage the physical dataflash media, including reading and writing of
 *  blocks of data. These functions are called by the SCSI layer when data must be stored
 *  or retrieved to/from the physical storage media. If a different media is used (such
 *  as a SD card or EEPROM), functions similar to these will need to be generated.
 */

#define  INCLUDE_FROM_DATAFLASHMANAGER_C
#include "DataflashManager.h"

/** Writes blocks (OS blocks, not Dataflash pages) to the storage medium, the board dataflash IC(s), from
 *  the pre-selected data OUT endpoint. This routine reads in OS sized blocks from the endpoint and writes
 *  them to the dataflash in Dataflash page sized blocks.
 *
 *  \param BlockAddress  Data block starting address for the write sequence
 *  \param TotalBlocks   Number of blocks of data to write
 */
void DataflashManager_WriteBlocks(const uint32_t BlockAddress, uint16_t TotalBlocks)
{
	uint16_t CurrDFPage          = ((BlockAddress * VIRTUAL_MEMORY_BLOCK_SIZE) / DATAFLASH_PAGE_SIZE);
	uint16_t CurrDFPageByte      = ((BlockAddress * VIRTUAL_MEMORY_BLOCK_SIZE) % DATAFLASH_PAGE_SIZE);
	uint8_t  CurrDFPageByteDiv16 = (CurrDFPageByte >> 4);

	/* Copy selected dataflash's current page contents to the dataflash buffer */
	Dataflash_SelectChipFromPage(CurrDFPage);
	Dataflash_SendByte(DF_CMD_MAINMEMTOBUFF1);
	Dataflash_SendAddressBytes(CurrDFPage, 0);
	Dataflash_WaitWhileBusy();

	/* Send the dataflash buffer write command */
	Dataflash_ToggleSelectedChipCS();
	Dataflash_SendByte(DF_CMD_BUFF1WRITE);
	Dataflash_SendAddressBytes(0, CurrDFPageByte);

	/* Wait until endpoint is ready before continuing */
	while (!(Endpoint_ReadWriteAllowed()));

	while (TotalBlocks)
	{
		uint8_t BytesInBlockDiv16 = 0;
		
		/* Write an endpoint packet sized data block to the dataflash */
		while (BytesInBlockDiv16 < (VIRTUAL_MEMORY_BLOCK_SIZE >> 4))
		{
			/* Check if the endpoint is currently empty */
			if (!(Endpoint_ReadWriteAllowed()))
			{
				/* Clear the current endpoint bank */
				Endpoint_ClearCurrentBank();
				
				/* Wait until the host has sent another packet */
				while (!(Endpoint_ReadWriteAllowed()));
			}

			/* Check if end of dataflash page reached */
			if (CurrDFPageByteDiv16 == (DATAFLASH_PAGE_SIZE >> 4))
			{
				/* Write the dataflash buffer contents back to the dataflash page */
				Dataflash_ToggleSelectedChipCS();
				Dataflash_SendByte(DF_CMD_BUFF1TOMAINMEMWITHERASE);
				Dataflash_SendAddressBytes(CurrDFPage, 0);

				/* Reset the dataflash buffer counter, increment the page counter */
				CurrDFPageByteDiv16 = 0;
				CurrDFPage++;

				/* Select the next dataflash chip based on the new dataflash page index */
				Dataflash_SelectChipFromPage(CurrDFPage);
				Dataflash_WaitWhileBusy();

#if (DATAFLASH_PAGE_SIZE > VIRTUAL_MEMORY_BLOCK_SIZE)
				/* If less than one dataflash page remaining, copy over the existing page to preserve trailing data */
				if ((TotalBlocks * (VIRTUAL_MEMORY_BLOCK_SIZE >> 4)) < (DATAFLASH_PAGE_SIZE >> 4))
				{
					/* Copy selected dataflash's current page contents to the dataflash buffer */
					Dataflash_ToggleSelectedChipCS();
					Dataflash_SendByte(DF_CMD_MAINMEMTOBUFF1);
					Dataflash_SendAddressBytes(CurrDFPage, 0);
					Dataflash_WaitWhileBusy();
				}
#endif

				/* Send the dataflash buffer write command */
				Dataflash_ToggleSelectedChipCS();
				Dataflash_SendByte(DF_CMD_BUFF1WRITE);
				Dataflash_SendAddressBytes(0, 0);
			}

			/* Write one 16-byte chunk of data to the dataflash */
			Dataflash_SendByte(Endpoint_Read_Byte());
			Dataflash_SendByte(Endpoint_Read_Byte());
			Dataflash_SendByte(Endpoint_Read_Byte());
			Dataflash_SendByte(Endpoint_Read_Byte());
			Dataflash_SendByte(Endpoint_Read_Byte());
			Dataflash_SendByte(Endpoint_Read_Byte());
			Dataflash_SendByte(Endpoint_Read_Byte());
			Dataflash_SendByte(Endpoint_Read_Byte());
			Dataflash_SendByte(Endpoint_Read_Byte());
			Dataflash_SendByte(Endpoint_Read_Byte());
			Dataflash_SendByte(Endpoint_Read_Byte());
			Dataflash_SendByte(Endpoint_Read_Byte());
			Dataflash_SendByte(Endpoint_Read_Byte());
			Dataflash_SendByte(Endpoint_Read_Byte());
			Dataflash_SendByte(Endpoint_Read_Byte());
			Dataflash_SendByte(Endpoint_Read_Byte());
			
			/* Increment the dataflash page 16 byte block counter */
			CurrDFPageByteDiv16++;

			/* Increment the block 16 byte block counter */
			BytesInBlockDiv16++;

			/* Check if the current command is being aborted by the host */
			if (IsMassStoreReset)
			  return;			
		}
			
		/* Decrement the blocks remaining counter and reset the sub block counter */
		TotalBlocks--;
	}

	/* Write the dataflash buffer contents back to the dataflash page */
	Dataflash_ToggleSelectedChipCS();
	Dataflash_SendByte(DF_CMD_BUFF1TOMAINMEMWITHERASE);
	Dataflash_SendAddressBytes(CurrDFPage, 0x00);
	Dataflash_WaitWhileBusy();

	/* If the endpoint is empty, clear it ready for the next packet from the host */
	if (!(Endpoint_ReadWriteAllowed()))
	  Endpoint_ClearCurrentBank();

	/* Deselect all dataflash chips */
	Dataflash_DeselectChip();
}

/** Reads blocks (OS blocks, not Dataflash pages) from the storage medium, the board dataflash IC(s), into
 *  the pre-selected data IN endpoint. This routine reads in Dataflash page sized blocks from the Dataflash
 *  and writes them in OS sized blocks to the endpoint.
 *
 *  \param BlockAddress  Data block starting address for the read sequence
 *  \param TotalBlocks   Number of blocks of data to read
 */
void DataflashManager_ReadBlocks(const uint32_t BlockAddress, uint16_t TotalBlocks)
{
	uint16_t CurrDFPage          = ((BlockAddress * VIRTUAL_MEMORY_BLOCK_SIZE) / DATAFLASH_PAGE_SIZE);
	uint16_t CurrDFPageByte      = ((BlockAddress * VIRTUAL_MEMORY_BLOCK_SIZE) % DATAFLASH_PAGE_SIZE);
	uint8_t  CurrDFPageByteDiv16 = (CurrDFPageByte >> 4);

	/* Send the dataflash main memory page read command */
	Dataflash_SelectChipFromPage(CurrDFPage);
	Dataflash_SendByte(DF_CMD_MAINMEMPAGEREAD);
	Dataflash_SendAddressBytes(CurrDFPage, CurrDFPageByte);
	Dataflash_SendByte(0x00);
	Dataflash_SendByte(0x00);
	Dataflash_SendByte(0x00);
	Dataflash_SendByte(0x00);
	
	/* Wait until endpoint is ready before continuing */
	while (!(Endpoint_ReadWriteAllowed()));
	
	while (TotalBlocks)
	{
		uint8_t BytesInBlockDiv16 = 0;
		
		/* Write an endpoint packet sized data block to the dataflash */
		while (BytesInBlockDiv16 < (VIRTUAL_MEMORY_BLOCK_SIZE >> 4))
		{
			/* Check if the endpoint is currently full */
			if (!(Endpoint_ReadWriteAllowed()))
			{
				/* Clear the endpoint bank to send its contents to the host */
				Endpoint_ClearCurrentBank();
				
				/* Wait until the endpoint is ready for more data */
				while (!(Endpoint_ReadWriteAllowed()));
			}
			
			/* Check if end of dataflash page reached */
			if (CurrDFPageByteDiv16 == (DATAFLASH_PAGE_SIZE >> 4))
			{
				/* Reset the dataflash buffer counter, increment the page counter */
				CurrDFPageByteDiv16 = 0;
				CurrDFPage++;

				/* Select the next dataflash chip based on the new dataflash page index */
				Dataflash_SelectChipFromPage(CurrDFPage);
				
				/* Send the dataflash main memory page read command */
				Dataflash_SendByte(DF_CMD_MAINMEMPAGEREAD);
				Dataflash_SendAddressBytes(CurrDFPage, 0);
				Dataflash_SendByte(0x00);
				Dataflash_SendByte(0x00);
				Dataflash_SendByte(0x00);
				Dataflash_SendByte(0x00);
			}	

			/* Read one 16-byte chunk of data from the dataflash */
			Endpoint_Write_Byte(Dataflash_ReceiveByte());
			Endpoint_Write_Byte(Dataflash_ReceiveByte());
			Endpoint_Write_Byte(Dataflash_ReceiveByte());
			Endpoint_Write_Byte(Dataflash_ReceiveByte());
			Endpoint_Write_Byte(Dataflash_ReceiveByte());
			Endpoint_Write_Byte(Dataflash_ReceiveByte());
			Endpoint_Write_Byte(Dataflash_ReceiveByte());
			Endpoint_Write_Byte(Dataflash_ReceiveByte());
			Endpoint_Write_Byte(Dataflash_ReceiveByte());
			Endpoint_Write_Byte(Dataflash_ReceiveByte());
			Endpoint_Write_Byte(Dataflash_ReceiveByte());
			Endpoint_Write_Byte(Dataflash_ReceiveByte());
			Endpoint_Write_Byte(Dataflash_ReceiveByte());
			Endpoint_Write_Byte(Dataflash_ReceiveByte());
			Endpoint_Write_Byte(Dataflash_ReceiveByte());
			Endpoint_Write_Byte(Dataflash_ReceiveByte());
			
			/* Increment the dataflash page 16 byte block counter */
			CurrDFPageByteDiv16++;
			
			/* Increment the block 16 byte block counter */
			BytesInBlockDiv16++;

			/* Check if the current command is being aborted by the host */
			if (IsMassStoreReset)
			  return;
		}
		
		/* Decrement the blocks remaining counter */
		TotalBlocks--;
	}
	
	/* If the endpoint is full, send its contents to the host */
	if (!(Endpoint_ReadWriteAllowed()))
	  Endpoint_ClearCurrentBank();

	/* Deselect all dataflash chips */
	Dataflash_DeselectChip();
}

/** Disables the dataflash memory write protection bits on the board Dataflash ICs, if enabled. */
void DataflashManager_ResetDataflashProtections(void)
{
	/* Select first dataflash chip, send the read status register command */
	Dataflash_SelectChip(DATAFLASH_CHIP1);
	Dataflash_SendByte(DF_CMD_GETSTATUS);
	
	/* Check if sector protection is enabled */
	if (Dataflash_ReceiveByte() & DF_STATUS_SECTORPROTECTION_ON)
	{
		Dataflash_ToggleSelectedChipCS();

		/* Send the commands to disable sector protection */
		Dataflash_SendByte(DF_CMD_SECTORPROTECTIONOFF[0]);
		Dataflash_SendByte(DF_CMD_SECTORPROTECTIONOFF[1]);
		Dataflash_SendByte(DF_CMD_SECTORPROTECTIONOFF[2]);
		Dataflash_SendByte(DF_CMD_SECTORPROTECTIONOFF[3]);
	}
	
	/* Select second dataflash chip (if present on selected board), send read status register command */
	#if (DATAFLASH_TOTALCHIPS == 2)
	Dataflash_SelectChip(DATAFLASH_CHIP2);
	Dataflash_SendByte(DF_CMD_GETSTATUS);
	
	/* Check if sector protection is enabled */
	if (Dataflash_ReceiveByte() & DF_STATUS_SECTORPROTECTION_ON)
	{
		Dataflash_ToggleSelectedChipCS();

		/* Send the commands to disable sector protection */
		Dataflash_SendByte(DF_CMD_SECTORPROTECTIONOFF[0]);
		Dataflash_SendByte(DF_CMD_SECTORPROTECTIONOFF[1]);
		Dataflash_SendByte(DF_CMD_SECTORPROTECTIONOFF[2]);
		Dataflash_SendByte(DF_CMD_SECTORPROTECTIONOFF[3]);
	}
	#endif
	
	/* Deselect current dataflash chip */
	Dataflash_DeselectChip();
}
