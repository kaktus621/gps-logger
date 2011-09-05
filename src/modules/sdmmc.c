/**
  ########## gLogger Mini ##########
    
	Von Martin Matysiak
	    mail@k621.de
      www.k621.de

  SDMMC - Eine Befehlsbibliothek zum Umgang mit Speicherkarten

  ########## Licensing ##########

  Please take a look at the LICENSE file for detailed information.
*/

#include "modules/sdmmc.h"

uint8_t sdmmc_init() {
  if (spi_init()) {
    // SD-Karte in SPI-Modus versetzen,
    // Daten�beretragung vorbereiten

    // Platzhalter f�r SPI Antworten und Timeout-Erkennung
    uint8_t response = 0;
    uint8_t retry = 0;

    // System "einschwingen" lassen   
    for(uint8_t i = 0; i < 20; i++) {
      spi_writeByte(0xFF);
    }

    // Kommando 0 senden (Wechsel in SPI Mode)
    while (response != 1) {
      response = sdmmc_writeCommand(0, 0, 0x95);

      if (retry++ == 0xFF) {
        CLEAR_CS();
        return FALSE;
      }
    }

    // Kommando 1 senden (Karte initalisieren)
    response = 0xFF;
    retry = 0;

    while (response != 0) {
      response = sdmmc_writeCommand(1, 0, 0xFF);

      if (retry++ == 0xFF) {
        CLEAR_CS();
        return FALSE;
      }
    }

    CLEAR_CS();

    // Switch to higher SPI frequency once initialized
    spi_highspeed();

    return TRUE;
  }

  return FALSE;
}

uint8_t sdmmc_writeCommand(uint8_t pCommand, uint32_t pArgument, uint8_t pCrc) {
  // Das zu sendende Kommando besteht aus 6 Byte:
  // Byte 1: 0b01xxxxxx wobei xx = pCommand
  // Byte 2-5: pArgument
  // Byte 6: 0byyyyyyy1 wobei yy = CRC7

  // Sequenz "0b01" erzeugen
  pCommand &= ~(1 << 7);
  pCommand |= (1 << 6);

  // Pr�fsumme berechnen (TODO, momentan konstante Werte)
  
  // Letztes Bit der Summe == 1
  pCrc |= 1;

  // Dummy-Takte generieren
  CLEAR_CS();
  spi_writeByte(0xFF);
  SET_CS();

  // ...und weg damit!
  spi_writeByte(pCommand);
  spi_writeByte((uint8_t)((pArgument & 0xFF000000) >> 24));
  spi_writeByte((uint8_t)((pArgument & 0x00FF0000) >> 16));
  spi_writeByte((uint8_t)((pArgument & 0x0000FF00) >> 8));
  spi_writeByte((uint8_t)(pArgument & 0x000000FF));
  spi_writeByte(pCrc);

  // Antwort holen
  uint8_t result = 0xFF;
  uint8_t retry = 0;

  while(result == 0xFF) {
    result = spi_readByte();
    if (retry++ == 0xFF) {
      result = 0;
      break;
    }
  }
  
  return result;
}

uint8_t sdmmc_writeSector(uint32_t pSectorNum, char* pInput, uint16_t pByteCount) {

  SET_CS();

  // Kommando 24 senden
  if (sdmmc_writeCommand(24, pSectorNum << 9, 0xFF) != 0) {
    CLEAR_CS();
    return FALSE;
  }

  // Dummy-Clocks senden
  for(uint8_t i = 0; i < 100; i++) {
    spi_readByte();
  }

  // Startbyte senden
  spi_writeByte(0xFE);

  // Daten senden
  for(uint16_t i = 0; i < pByteCount; i++) {
    spi_writeByte(pInput[i]);
  }

  // CRC senden (wird nicht gepr�ft, da SPI Modus)
  spi_writeByte(0xFF);
  spi_writeByte(0xFF);

  // Antwort holen
  if ((spi_readByte() & 0x1F) != 0x05) {
    CLEAR_CS();
    return FALSE;
  }

  // Warten bis SD-Karte wieder bereit f�r neue Anfragen
  while (spi_readByte() != 0xFF) {
    // Energie verbrennen
  }

  CLEAR_CS();
  return TRUE;
}

uint8_t sdmmc_readSector(uint32_t pSectorNum, char* pOutput, uint16_t pByteCount) {
  
  SET_CS();

  // Kommando 17 senden (CRC Pr�fung im SPI Modus deaktiviert)
  // Adresse wird in Bytes angegeben, also pSectorNum * 512 == pSectorNum << 9
  if (sdmmc_writeCommand(17, pSectorNum << 9, 0xFF) != 0) {
    CLEAR_CS();
    return FALSE;
  }

  // Auf Startbyte warten
  uint8_t response = 0;
  uint8_t retry = 0;

  while(response != 0xFE) {
    response = spi_readByte();
    if (retry++ == 0xFF) {
      CLEAR_CS();
      return FALSE;
    }
  }

  // Block lesen
  for (uint16_t i = 0; i < pByteCount; i++) {
    pOutput[i] = spi_readByte();
  }

  // 16-Bit CRC ignorieren
  spi_readByte();
  spi_readByte();

  // Fertig
  CLEAR_CS();
  return TRUE;
}