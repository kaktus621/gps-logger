/**
  ########## gLogger Mini ##########
    
	Von Martin Matysiak
	    mail@k621.de
      www.k621.de

  Befehlsbibliothek f�r das SPI-Protkoll

  ########## Licensing ##########

  Please take a look at the LICENSE file for detailed information.
*/

#ifndef SPI_H
  #define SPI_H

  #define SPI_PORT PORTB
  #define SPI_PORT_DIR DDRB

  #define SPI_CS PB2
  #define SPI_MOSI PB3
  #define SPI_MISO PB4
  #define SPI_SCK PB5

  #define SET_CS() SPI_PORT &= ~(1 << SPI_CS)
  #define CLEAR_CS() SPI_PORT |= (1 << SPI_CS)

  #include "global.h"
  
  uint8_t spi_init();

  void spi_writeByte(uint8_t pByte);
  
  uint8_t spi_readByte();
#endif