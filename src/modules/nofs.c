/**
 * \file nofs.c
 * \brief Library for accessing a NoFS (NoFileSystem) on the memory card
 * \author Martin Matysiak
 */

#include "modules/nofs.h"

/// Index of the currently active sector
uint32_t fCurrentSector = 0;
/// Index which points to the current end of data inside the current sector
uint16_t fCurrentByte = 0;
/// Will be incremented upon every writeString call
uint8_t fWriteCount = 0;
/// Buffer which holds the currently active sector
char sectorBuf[NOFS_BUFFER_SIZE];

void nofs_init() { 
    /*
        Steps of initialization:
        1) Call initialization of underlying SDMMC interface
        2) Read the first sector
        3) Check if sector starts with NOFS_HEADER (display error if not)
        4) Get position of last scan for writing position
        5) Jump to this position and scan for NOFS_TERMINAL from there on
        6) Jump back to first sector and update last writing position
        7) Jump forward to writing position, initialization finished.
    */
    
    // Step 1
    sdmmc_init();
    
    // Step 2
    sdmmc_readSector(0, sectorBuf);
    
    // Step 3
    if (!strStartsWith(sectorBuf, NOFS_HEADER)) {
        error(ERROR_NOFS);
    }
    
    // Step 4
    for (uint8_t i = 0; i < 4; i++) {
        fCurrentSector += sectorBuf[NOFS_HEADER_LENGTH + i] << ((3 - i) * 8); // MSB first
    }
    
    // Step 5

    // As we're only interested in the very first byte of each sector, we try to
    // change the block size. If it succeeds, scanning will be A LOT faster, if
    // not, it'll still work.
    sdmmc_changeBlockLength(1);
    sdmmc_readSector(fCurrentSector, sectorBuf);

    // Search for NOFS_TERMINAL which has to be located in byte 0 of a sector
    while(sectorBuf[0] != NOFS_TERMINAL) {
        sdmmc_readSector(++fCurrentSector, sectorBuf);
    }

    // Change block size back to default. Won't have any effect if the previous
    // change failed.
    sdmmc_changeBlockLength(0);

    // Now get the sector in front of this one and look for the last 
    // byte with actual data
    sdmmc_readSector(--fCurrentSector, sectorBuf);

    while((sectorBuf[fCurrentByte] != NOFS_TERMINAL) && (fCurrentByte < NOFS_BUFFER_SIZE)) {
        fCurrentByte++;
    }

    // Check if we have the edge case that the sector was filled 
    // up to the very last byte
    if (fCurrentByte == NOFS_BUFFER_SIZE) {
        // Set start marker to next sector, byte 0
        fCurrentSector++;
        fCurrentByte = 0;
    }
    
    // Step 6
    sdmmc_readSector(0, sectorBuf);

    for (uint8_t i = 0; i < 4; i++) {
        sectorBuf[NOFS_HEADER_LENGTH + i] = (fCurrentSector >> ((3 - i) * 8)) & 0xFF; // MSB first
    }

    sdmmc_writeSector(0, sectorBuf);
    
    // Step 7
    sdmmc_readSector(fCurrentSector, sectorBuf);
}

void nofs_writeString(char* pString) {
    // Copy data into the buffer. If the buffer is full, write it onto the
    // memory card and create a new empty one
    uint8_t i = 0;

    while (pString[i]) {
        if(fCurrentByte < NOFS_BUFFER_SIZE) {
            sectorBuf[fCurrentByte++] = pString[i++];
        } else {   
            nofs_flush();
            fCurrentSector++;
            fCurrentByte = 0;
        }
    }

    // Set the new end of data
    if (fCurrentByte == NOFS_BUFFER_SIZE) {
        nofs_flush();
        fCurrentSector++;
        fCurrentByte = 0;
    }

    sectorBuf[fCurrentByte] = ETX;
}

void nofs_flush() {
    // Remember the first byte as we will replace it with the NOFS_TERMINAL
    // temporarily to write the current+1 sector
    char temp = sectorBuf[0];
    sectorBuf[0] = NOFS_TERMINAL;
    sdmmc_writeSector(fCurrentSector + 1, sectorBuf);
    
    _delay_ms(10);
    
    // Now write the actual current sector
    sectorBuf[0] = temp;
    sdmmc_writeSector(fCurrentSector, sectorBuf);
}