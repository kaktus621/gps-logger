/**
  ########## gLogger Mini ##########
    
	Von Martin Matysiak
	    mail@k621.de
      www.k621.de

  Methoden zum Umgang mit dem ST22 GPS-Modul von Perthold Engineering.
  Klasse auf absolute Grundfunktionen reduziert. Datenausgabe erfolgt
  ungeparst als ASCII-String, keine Pr�fung auf Validit�t o.�..

  ########## Licensing ##########

  Please take a look at the LICENSE file for detailed information.
*/

#include "modules/gps.h"

uint8_t gps_init() {

  //UART-Schnittstelle initalisieren
  uart_init(UART_CONFIGURE(UART_ASYNC, UART_8BIT, UART_1STOP, UART_NOPAR), 
    UART_CALCULATE_BAUD(F_CPU, GPS_BAUDRATE));

  _delay_s(1);

  //Grundeinstellungen vornehmen
  unsigned char commands[8] = {
    0x01, // GGA 1Hz
    0x00, //Keine GSA
    0x00, //Keine GSV
    0x00, //Keine GLL
    0x00, //Keine RMC
    0x00, //Keine VTG
    0x00, //Keine ZDA
    0x00}; //In SRAM&FLASH

  gps_setParam(GPS_SET_NMEA, commands, 8);

  _delay_ms(50);

  unsigned char rate[2] = {
    0x01, //1 Hertz
    0x00}; //In SRAM

  gps_setParam(GPS_SET_UPDATE_RATE, rate, 2);

  _delay_ms(50);

  unsigned char pps[2] = {
    0x01,  //1PPS bei 3D Fix
    0x00}; //In SRAM

  gps_setParam(GPS_SET_1PPS, pps, 2);

  _delay_ms(50);

  return TRUE;
}

unsigned char gps_calculateCS(const unsigned char* pPayload, uint16_t pLength) {
  unsigned char checkSum = 0;
  for(uint8_t i = 0; i < pLength; i++) {
    checkSum ^= pPayload[i];
  }
  return checkSum;
}

unsigned char gps_setParam(unsigned char pCommand, unsigned char* pData, uint16_t pLength) {
  //Startsequenz senden (2 byte)
  uart_setChar(0xA0);
  uart_setChar(0xA1);
 
  //Payload Length senden (2 byte) == Length + 1 wegen Msg ID
  uart_setChar(((pLength+1) & 0xFF00) >> 8);
  uart_setChar((pLength+1) & 0x00FF);

  //Payload senden

  //MsgID (1 byte)
  uart_setChar(pCommand);

  //Data (pLength byte)
  for(uint8_t i = 0; i < pLength; i++) {
    uart_setChar(pData[i]);
  }

  //Checksum (1 byte)
  uart_setChar(gps_calculateCS(pData, pLength) ^ pCommand);

  //Stopsequenz senden (2 byte)
  uart_setChar(CR);
  uart_setChar(LF);

  return GPS_ACK;
}

uint8_t gps_getNmeaSentence(char* pOutput, uint8_t pMaxLength) {
  while(uart_getChar() != '$') {
    // Energie verbrennen
    _delay_ms(1);
  }

  // Daten bis zu LF kopieren
  uint8_t i = 0;
  pOutput[i++] = '$';

  char inChar;
  uint8_t commaCount = 0;
  uint8_t checkNow = FALSE;
  uint8_t gpsFix = FALSE;

  do {
    while(!uart_hasData()) {
      // Energie verbrennen
    }
    inChar = uart_getChar();

    // TRUE wenn letztes Zeichen das 6. Komma war
    if (checkNow == TRUE) {
      if (inChar != '0') {
        gpsFix = TRUE;
      }
      checkNow = FALSE;
    }

    // Kommas z�hlen, nach dem 6. Komma folgt bei GGA Meldungen die Validit�tsanzeige
    if (inChar == ',') {
      if(++commaCount == 6) {
        checkNow = TRUE;
      }
    }

    pOutput[i++] = inChar;
  } while((inChar != LF) && (i < (pMaxLength-1)));

  pOutput[i] = 0;
  return gpsFix;
}
