/* Gravador de Códigos da memória Flash do nrf24le1 por SPI
  Ítalo Gustavo Sampaio Fernandes

  SPI MODE 0 - de acordo com os diagramas, mas não está explicitamente escrito


  Pinos para conectar:
        24pin   32pin       Arduino(DUE) -  PIC32
  FCSN    P05     P11     ->  10      -
  FMISO   P04     P10     ->  ICSP-1  -
  FMOSI   P03     P07     ->  ICSP-4  -
  FSCK    P02     P05     ->  ICSP-3  -


        ->   Arduino - PIC32
  VCC     ->  3.3V     -
  GND     ->  GND      -   GND
  PROG    ->  9
  RESET   ->  8

*/

#include <SPI.h>

SPISettings settings_nRF_SPI(8000000, MSBFIRST, SPI_MODE0);
/******************************************************************************/
//Comandos via interface SPI:
#define CF_WREN     0x06    //Set BIT WEN
#define CF_WRDIS    0x04    //Reset BIT WEN
#define CF_RDSR     0x05
#define CF_WRSR     0x01
#define CF_READ     0x03    //2 bytes
#define CF_PROGRAM  0x02    //2 bytes - WEN must be set
#define CF_ERASE_PAGE   0x52    //1 byte - WEN must be set
#define CF_ERASE_ALL    0x62 //NOTE: DSYS should ALWAYS be read out and stored before
#define CF_RDFPCR   0x89
#define CF_RDISMB   0x85
#define CF_ENDEBUG  0x86

#define FCSN_pin  10
#define PROG_pin  9
#define RESET_pin 8

#define NULL_SPI 0x00

/******************************************************************************/
#define DEBUG_MODE

#ifdef DEBUG_MODE
#define DEBUG_PRINT(x)  Serial.write(x)
#define DEBUG_PRINTBIN(x)  Serial.print(x,BIN)
#define DEBUG_PRINTHEX(x)  Serial.print(x,HEX)
#endif

#ifndef DEBUG_MODE
#define DEBUG_PRINT(x)
#define DEBUG_PRINTBIN(x)
#define DEBUG_PRINTHEX(x)
#endif

uint8_t temporary_page_vector[512];
uint8_t infopageData[512];
uint8_t hex_code[0x4800];


#define SERIAL_ERROR_CODE 0xFF
#define SERIAL_MSG_CODE 0xFE
#define SERIAL_PAGE_CODE 0xFD
void INIT_SERIAL_MSG(uint8_t qnt) {
  Serial.write('S'); Serial.write(SERIAL_MSG_CODE); Serial.write(qnt);
}

uint8_t hex_code1[944] = {
0x02, 0x00, 0xF3, 0x8E, 0x1E, 0x8F, 0x1F, 0xC2, 0xE8, 0x7D, 0x0E, 0x7F, 0x20, 0x12, 0x03, 0x45, 
0x7B, 0xFF, 0x7A, 0x03, 0x79, 0x9B, 0x75, 0x15, 0x05, 0x7F, 0x2A, 0x12, 0x02, 0x6D, 0x7B, 0x00, 
0x7A, 0x00, 0x79, 0x42, 0x85, 0x1F, 0x15, 0x7F, 0xA0, 0x12, 0x02, 0x6D, 0xD2, 0xE8, 0xA2, 0x04, 
0x72, 0x05, 0x50, 0xFA, 0x02, 0x03, 0x58, 0xAD, 0x07, 0xAC, 0x06, 0x12, 0x03, 0x00, 0x12, 0x03, 
0x00, 0xED, 0x1D, 0x70, 0x01, 0x1C, 0x14, 0x4C, 0x70, 0xF1, 0x22, 0x02, 0x01, 0x7F, 0x8B, 0x08, 
0x8A, 0x09, 0x89, 0x0A, 0x7C, 0x07, 0xC2, 0xE8, 0xD2, 0xEA, 0xD2, 0xB9, 0xC2, 0xE8, 0xD2, 0xEA, 
0xE5, 0x10, 0x14, 0x60, 0x0F, 0x14, 0x60, 0x14, 0x14, 0x60, 0x16, 0x24, 0x03, 0x70, 0x15, 0x53, 
0x04, 0xF9, 0x80, 0x10, 0x53, 0x04, 0xFB, 0x43, 0x04, 0x02, 0x80, 0x08, 0x53, 0x04, 0xFD, 0x80, 
0x00, 0x43, 0x04, 0x04, 0xE5, 0x0F, 0x60, 0x0E, 0x14, 0x60, 0x10, 0x14, 0x70, 0x13, 0x53, 0x04, 
0xF7, 0x43, 0x04, 0x20, 0x80, 0x0B, 0x53, 0x04, 0xD7, 0x80, 0x06, 0x53, 0x04, 0xDF, 0x43, 0x04, 
0x08, 0xAB, 0x0B, 0xAA, 0x0C, 0xA9, 0x0D, 0x75, 0x15, 0x05, 0x7F, 0x30, 0x12, 0x02, 0x6D, 0xAB, 
0x08, 0xAA, 0x09, 0xA9, 0x0A, 0x75, 0x15, 0x05, 0x7F, 0x2A, 0x12, 0x02, 0x6D, 0x7D, 0x01, 0x7F, 
0x22, 0x12, 0x03, 0x45, 0xE4, 0xFD, 0x7F, 0x21, 0x12, 0x03, 0x45, 0xE4, 0xFD, 0x7F, 0x24, 0x12, 
0x03, 0x45, 0xAD, 0x0E, 0x7F, 0x25, 0x12, 0x03, 0x45, 0xAD, 0x04, 0x7F, 0x26, 0x12, 0x03, 0x45, 
0x7D, 0x01, 0x7F, 0x3C, 0x12, 0x03, 0x45, 0x7D, 0x07, 0x7F, 0x3D, 0x12, 0x03, 0x45, 0xD2, 0xAF, 
0x02, 0x03, 0x58, 0x78, 0x7F, 0xE4, 0xF6, 0xD8, 0xFD, 0x75, 0x81, 0x62, 0x02, 0x01, 0x3A, 0x02, 
0x03, 0x8D, 0xE4, 0x93, 0xA3, 0xF8, 0xE4, 0x93, 0xA3, 0x40, 0x03, 0xF6, 0x80, 0x01, 0xF2, 0x08, 
0xDF, 0xF4, 0x80, 0x29, 0xE4, 0x93, 0xA3, 0xF8, 0x54, 0x07, 0x24, 0x0C, 0xC8, 0xC3, 0x33, 0xC4, 
0x54, 0x0F, 0x44, 0x20, 0xC8, 0x83, 0x40, 0x04, 0xF4, 0x56, 0x80, 0x01, 0x46, 0xF6, 0xDF, 0xE4, 
0x80, 0x0B, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x90, 0x03, 0x95, 0xE4, 0x7E, 0x01, 
0x93, 0x60, 0xBC, 0xA3, 0xFF, 0x54, 0x3F, 0x30, 0xE5, 0x09, 0x54, 0x1F, 0xFE, 0xE4, 0x93, 0xA3, 
0x60, 0x01, 0x0E, 0xCF, 0x54, 0xC0, 0x25, 0xE0, 0x60, 0xA8, 0x40, 0xB8, 0xE4, 0x93, 0xA3, 0xFA, 
0xE4, 0x93, 0xA3, 0xF8, 0xE4, 0x93, 0xA3, 0xC8, 0xC5, 0x82, 0xC8, 0xCA, 0xC5, 0x83, 0xCA, 0xF0, 
0xA3, 0xC8, 0xC5, 0x82, 0xC8, 0xCA, 0xC5, 0x83, 0xCA, 0xDF, 0xE9, 0xDE, 0xE7, 0x80, 0xBE, 0xC0, 
0xE0, 0xC0, 0x83, 0xC0, 0x82, 0xC0, 0xD0, 0x75, 0xD0, 0x00, 0xC0, 0x00, 0xC0, 0x01, 0xC0, 0x02, 
0xC0, 0x03, 0xC0, 0x05, 0xC0, 0x06, 0xC0, 0x07, 0x7F, 0x07, 0x12, 0x03, 0x69, 0x8F, 0x20, 0x30, 
0x06, 0x31, 0x7B, 0x00, 0x7A, 0x00, 0x79, 0x22, 0x75, 0x1A, 0x20, 0x7F, 0x61, 0x12, 0x02, 0x9F, 
0xE4, 0xFD, 0x7F, 0xE2, 0x12, 0x03, 0x45, 0xD2, 0x08, 0x7F, 0x60, 0x12, 0x03, 0x69, 0x8F, 0x62, 
0xE5, 0x62, 0xD3, 0x94, 0x20, 0x40, 0x0C, 0x75, 0x62, 0x00, 0xE4, 0xFD, 0x7F, 0xE2, 0x12, 0x03, 
0x45, 0xC2, 0x08, 0x30, 0x04, 0x07, 0xE4, 0xFD, 0x7F, 0xE1, 0x12, 0x03, 0x45, 0x30, 0x05, 0x07, 
0xE4, 0xFD, 0x7F, 0xE1, 0x12, 0x03, 0x45, 0x7D, 0x70, 0x7F, 0x27, 0x12, 0x03, 0x45, 0xD0, 0x07, 
0xD0, 0x06, 0xD0, 0x05, 0xD0, 0x03, 0xD0, 0x02, 0xD0, 0x01, 0xD0, 0x00, 0xD0, 0xD0, 0xD0, 0x82, 
0xD0, 0x83, 0xD0, 0xE0, 0x32, 0xBB, 0x01, 0x06, 0x89, 0x82, 0x8A, 0x83, 0xE0, 0x22, 0x50, 0x02, 
0xE7, 0x22, 0xBB, 0xFE, 0x02, 0xE3, 0x22, 0x89, 0x82, 0x8A, 0x83, 0xE4, 0x93, 0x22, 0xBB, 0x01, 
0x0C, 0xE5, 0x82, 0x29, 0xF5, 0x82, 0xE5, 0x83, 0x3A, 0xF5, 0x83, 0xE0, 0x22, 0x50, 0x06, 0xE9, 
0x25, 0x82, 0xF8, 0xE6, 0x22, 0xBB, 0xFE, 0x06, 0xE9, 0x25, 0x82, 0xF8, 0xE2, 0x22, 0xE5, 0x82, 
0x29, 0xF5, 0x82, 0xE5, 0x83, 0x3A, 0xF5, 0x83, 0xE4, 0x93, 0x22, 0xF8, 0xBB, 0x01, 0x0D, 0xE5, 
0x82, 0x29, 0xF5, 0x82, 0xE5, 0x83, 0x3A, 0xF5, 0x83, 0xE8, 0xF0, 0x22, 0x50, 0x06, 0xE9, 0x25, 
0x82, 0xC8, 0xF6, 0x22, 0xBB, 0xFE, 0x05, 0xE9, 0x25, 0x82, 0xC8, 0xF2, 0x22, 0x8B, 0x12, 0x8A, 
0x13, 0x89, 0x14, 0xC2, 0xE9, 0x12, 0x03, 0x83, 0xAE, 0x07, 0xE4, 0xFD, 0xED, 0xC3, 0x95, 0x15, 
0x50, 0x18, 0xAB, 0x12, 0x05, 0x14, 0xE5, 0x14, 0xAA, 0x13, 0x70, 0x02, 0x05, 0x13, 0x14, 0xF9, 
0x12, 0x02, 0x05, 0xFF, 0x12, 0x03, 0x83, 0x0D, 0x80, 0xE2, 0xD2, 0xE9, 0xAF, 0x06, 0x22, 0x8B, 
0x17, 0x8A, 0x18, 0x89, 0x19, 0xC2, 0xE9, 0x12, 0x03, 0x83, 0xAE, 0x07, 0xE4, 0xFD, 0xED, 0xC3, 
0x95, 0x1A, 0x50, 0x17, 0xE4, 0xFF, 0x12, 0x03, 0x83, 0xAB, 0x17, 0xAA, 0x18, 0xA9, 0x19, 0x8D, 
0x82, 0x75, 0x83, 0x00, 0xEF, 0x12, 0x02, 0x4B, 0x0D, 0x80, 0xE3, 0xD2, 0xE9, 0xAF, 0x06, 0x22, 
0x8B, 0x1B, 0x8A, 0x1C, 0x89, 0x1D, 0x12, 0x03, 0x30, 0xE4, 0xFE, 0xEE, 0xC3, 0x9F, 0x50, 0x18, 
0xAB, 0x1B, 0xAA, 0x1C, 0xA9, 0x1D, 0x8E, 0x82, 0x75, 0x83, 0x00, 0x12, 0x02, 0x1E, 0xFD, 0x74, 
0x42, 0x2E, 0xF8, 0xA6, 0x05, 0x0E, 0x80, 0xE3, 0x7E, 0x00, 0x12, 0x00, 0x03, 0x02, 0x03, 0x58, 
0x7F, 0xFA, 0x7E, 0x00, 0x12, 0x03, 0x0B, 0x7F, 0xFA, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0xEF, 0x1F, 0x70, 0x01, 0x1E, 0x14, 0x4E, 0x70, 0xF2, 0x22, 0x30, 0x82, 0x12, 0xD2, 0x83, 0x7F, 
0xD0, 0x7E, 0x07, 0x12, 0x00, 0x37, 0xC2, 0x83, 0x7F, 0xD0, 0x7E, 0x07, 0x12, 0x00, 0x37, 0x22, 
0xE4, 0xFF, 0xFE, 0x12, 0x02, 0x05, 0x60, 0x0C, 0x0F, 0xEF, 0x70, 0x01, 0x0E, 0x09, 0xE9, 0x70, 
0xF2, 0x0A, 0x80, 0xEF, 0x22, 0xAE, 0x05, 0xC2, 0xE9, 0x12, 0x03, 0x83, 0xAD, 0x07, 0xAF, 0x06, 
0x12, 0x03, 0x83, 0xD2, 0xE9, 0xAF, 0x05, 0x22, 0xE4, 0xF5, 0x20, 0xC2, 0x08, 0xC2, 0xE8, 0x7D, 
0x0F, 0x7F, 0x20, 0x12, 0x03, 0x45, 0xD2, 0xE8, 0x22, 0xC2, 0xE9, 0x12, 0x03, 0x83, 0xE4, 0xFF, 
0x12, 0x03, 0x83, 0xD2, 0xE9, 0x22, 0x75, 0x93, 0xFF, 0x75, 0x94, 0xFF, 0x53, 0x93, 0xF7, 0x43, 
0x93, 0x04, 0x22, 0x8F, 0xE7, 0xE5, 0xE6, 0x30, 0xE1, 0xFB, 0xAF, 0xE7, 0x22, 0x12, 0x03, 0x76, 
0x12, 0x03, 0x1A, 0x80, 0xFB, 0xC1, 0x08, 0x01, 0x62, 0x00, 0x00, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

uint16_t code_len = sizeof(hex_code1);

void setup() {
  pinMode(FCSN_pin, OUTPUT);  digitalWrite(FCSN_pin, LOW);
  pinMode(PROG_pin, OUTPUT);  digitalWrite(PROG_pin, LOW);
  pinMode(RESET_pin, OUTPUT);  digitalWrite(RESET_pin, LOW);
  SPI.begin();
  Serial.begin(9600);
  INIT_SERIAL_MSG(56);
  Serial.print("\nEscrever codigo no nrf24le1.\nPara iniciar envie um 'S'\n");
}
uint8_t some_value = 0xBB;
void loop() {
  if (Serial.available() && Serial.read() == 'S') {
    INIT_SERIAL_MSG(10);
    Serial.print("\nRecebido\n");
    if (!wait_serial_bytes(1)) { //se recebeu 1 byte
      switch (Serial.read()) {
        case '0':
          serial_receive_prog_data();
          //send_error_to_gui();
          //nrf_write_flash(0x01,0x00, 1, &some_value);
          break;
        case '1':
          if (nrf_enter_program_mode()) {
            send_error_to_gui();
          }
          break;
        case '2':
          if (nrf_connect_prog()) {
            send_error_to_gui();
          }
          break;
        case '3':
          Serial.print("\n");
          mostrar512(infopageData,0x00);
          send_code_to_gui(infopageData, 0x00);
          break;
        case '4':
          if (nrf_erase_old_code()) {
            send_error_to_gui();
          }
          break;
        case '5':
          if (nrf_write_new_code(code_len, hex_code1)) {
            send_error_to_gui();
          }
          break;
        case '6':
          if (nrf_verify_code(code_len, hex_code1)) {
            send_error_to_gui();
          }
          break;
        case '7':
          nrf_disconnect_prog();
          break;
        case '8':
          rotina_gravacao_auto();
          break;
        case '9':
          readFlashCode();
          break;
        default:
          send_error_to_gui();
          INIT_SERIAL_MSG(26);
          DEBUG_PRINT("\nErro ao receber comando.\n");
      }
    }
  }
}

uint8_t wait_serial_bytes(uint16_t quantos_bytes) {
  long tempo_inicio = millis();
  while (Serial.available() < quantos_bytes) {
    if (millis() - tempo_inicio > 1000) { //timeout
      return 1;
    }
  }
  return 0;
}

void send_code_to_gui(uint8_t *code_to_send, uint8_t addr_msb) {
  Serial.write('S');
  Serial.write(SERIAL_PAGE_CODE);
  Serial.write(addr_msb);
  Serial.write(code_to_send, 512);
}

void send_error_to_gui() {
  Serial.write('S');
  Serial.write(SERIAL_ERROR_CODE);
}

uint8_t serial_receive_prog_data() {
  //  inicializar_vetores();
  if (wait_serial_bytes(2)) {
    return 1;
  }
  uint8_t code_len_h = Serial.read();
  uint8_t code_len_l = Serial.read();
  code_len = code_len_h << 8 | code_len_l;

  while (true) {
    if (receive_hex_code()) {
      break;
    }
  }
  return 0;
}

uint8_t receive_hex_code() {
  //packet: start qnt addr_h addr_l type data data data ... check
  if (wait_serial_bytes(1) && Serial.read() != ':') { //Recebe start
    return 1;
  }
  if (wait_serial_bytes(4)) {
    return 1;
  }
  uint8_t qnt = Serial.read();
  uint8_t addr_h = Serial.read();
  uint8_t addr_l = Serial.read();
  uint8_t data_type = Serial.read();
  for (int i = 0; i < qnt; i++) {
    if (wait_serial_bytes(1)) {
      return 1;
    }
    hex_code[addr_h << 8 | addr_l] = Serial.read();
  }
  if (wait_serial_bytes(1)) {
    return 1;
  }
  Serial.read();//uint8_t check_byte =

  if (data_type == 0x01) {
    return 2;
  } else if (data_type != 0x00) {
    Serial.print(SERIAL_ERROR_CODE);
    return 3;
  }

  return 0;
}

void rotina_gravacao_auto() {
  if (!nrf_enter_program_mode()) {
    if (!nrf_connect_prog()) {
      //Serial.print("\nDados na InfoPage:\n");
      send_code_to_gui(infopageData, 0x00);
      if (!nrf_erase_old_code()) {
        nrf_write_new_code(sizeof(hex_code1), hex_code1);
      }
      nrf_verify_code(sizeof(hex_code1), hex_code1);
    }
    nrf_disconnect_prog();
  }
  INIT_SERIAL_MSG(1 + 3 + 1 + 2 + 1 + 3 + 5 + 1 + 1);
  Serial.print("\nFim da execucao,\n");
}

void readFlashCode() {
  nrf_read_flash(0x00, 512, temporary_page_vector);
  Serial.print("\n");
  mostrar512(temporary_page_vector, 0x00);
  //send_code_to_gui(temporary_page_vector,0x00);
}

void mostrar512(uint8_t *data2show, uint16_t addr_offset) {
  for (int i = 0; i < 16; i++) {
    Serial.print("0x");
    Serial.print(i + addr_offset, HEX); Serial.print("0:\t");
    for (int j = 0; j < 16; j++) {
      Serial.print(data2show[i * 16 + j], HEX); Serial.print("\t");
    }
    Serial.println();
  }
  for (int i = 0; i < 16; i++) {
    Serial.print("0x");
    Serial.print(0x0100 + i + addr_offset, HEX); Serial.print("0:\t");
    for (int j = 0; j < 16; j++) {
      Serial.print(data2show[i * 16 + j + 256], HEX); Serial.print("\t");
    }
    Serial.println();
  }
}

/**************STEP 1***************/
/*Entrar no modo de programação.*/

/**
   Envia comando WREN para perimitir escrita/apagar bytes
*/
void nrf_enable_wen() {
  digitalWrite(FCSN_pin, LOW);
  SPI.beginTransaction(settings_nRF_SPI);

  SPI.transfer(CF_WREN);//Enviando comando WREN

  digitalWrite(FCSN_pin, HIGH);
  SPI.endTransaction();
}

/**
   Envia pulsos para os pinos PROG e RESET de modo a iniciar o modo de programação
   Detalhes de acordo com o manual do nrf.
*/
void nrf_RESET_and_PROG_pulses() {
  //Iniciando modo de programação do sensor
  digitalWrite(RESET_pin, HIGH);
  digitalWrite(PROG_pin, HIGH);
  delay(2);// you must wait at least 1.5 ms before you input the first flash command.
  digitalWrite(RESET_pin, LOW); //pulse of minimun 0.2us
  delayMicroseconds(1);
  digitalWrite(RESET_pin, HIGH);
  delay(2);// you must wait at least 1.5 ms before you input the first flash command.

  //////////////////
  //Debug Mesages //
  //////////////////
  INIT_SERIAL_MSG(27);
  DEBUG_PRINT("\n*************************\n");
  INIT_SERIAL_MSG(28);
  DEBUG_PRINT("Pulsos em Reset e Prog pins.");
  INIT_SERIAL_MSG(27);
  DEBUG_PRINT("\n*************************\n");
}

/**
   Envia o comando spi para ler o register FSR
   composto por [ENDEBUG] [STP] [WEN] [RDYN] [INFEN] [RDISMB] [NULL] [NULL]
   @return valor no register fsr
*/
uint8_t nrf_read_FSR() {

  digitalWrite(FCSN_pin, LOW);
  SPI.beginTransaction(settings_nRF_SPI);

  SPI.transfer(CF_RDSR);
  uint8_t fsr_value = SPI.transfer(NULL_SPI);

  digitalWrite(FCSN_pin, HIGH);
  SPI.endTransaction();
  INIT_SERIAL_MSG(27);
  DEBUG_PRINT("\n*************************\n");
  INIT_SERIAL_MSG(34);
  DEBUG_PRINT("Read Flash status register (FSR): ");
  INIT_SERIAL_MSG(1);
  DEBUG_PRINT(" -BIN- ");
  DEBUG_PRINTBIN(fsr_value);
  DEBUG_PRINT(" -- ");
  INIT_SERIAL_MSG(3);
  DEBUG_PRINT(" - ");
  INIT_SERIAL_MSG(1);
  DEBUG_PRINT(" -HEX- ");
  DEBUG_PRINTHEX(fsr_value);
  DEBUG_PRINT(" -- ");
  INIT_SERIAL_MSG(27);
  DEBUG_PRINT("\n*************************\n");
  return fsr_value;
}

/**
   Realiza as seguintes configuraçoes:
    - Alterar nível nos pinos RESET e PROG.
    - Ler FSR pela primeira vez.
    Com isso a interface de programação SPI se torna acessivel
    @return flag de erro: 0-ok | 1-Interface não liberada devido a falha.
*/
uint8_t nrf_enter_program_mode() {
  nrf_RESET_and_PROG_pulses();
  //if RDISM bit of FSR is set to 0, the SPI master has full access to the flash
  if (nrf_read_FSR() & 0x04) { //Se 1 em bit RDISMB - o acesso foi negado
    INIT_SERIAL_MSG(69);
    DEBUG_PRINT("\nNao foi possivel entrar noo modo de programacao - algum erro ocorreu");
    return 1;
  } else {
    INIT_SERIAL_MSG(27);
    DEBUG_PRINT("\n*************************\n");
    INIT_SERIAL_MSG(47);
    DEBUG_PRINT("nrf_enter_program_mode() executada com sucesso.");
    INIT_SERIAL_MSG(27);
    DEBUG_PRINT("\n*************************\n");
    return 0;
  }
}

/**************STEP 2***************/
/*Conectar ao dispositivo.*/

/**
   Envia o comando spi para escrever no register FSR
   composto por [ENDEBUG] [STP] [WEN] [RDYN] [INFEN] [RDISMB] [NULL] [NULL]
   Permissão:   [R/W*]    [R]   [R/W] [R]    [R/W]  [R/W*]    [R]     [R]
   Obs *: Permitida escrita com restrições, consultar data-sheet
   @return flag de erro: 0-ok | 1-Erro
*/
uint8_t nrf_write_FSR(uint8_t fsr_byte) {

  digitalWrite(FCSN_pin, LOW);
  SPI.beginTransaction(settings_nRF_SPI);

  SPI.transfer(CF_WRSR);
  SPI.transfer(fsr_byte);

  digitalWrite(FCSN_pin, HIGH);
  SPI.endTransaction();

  if (nrf_read_FSR() == fsr_byte) {
    INIT_SERIAL_MSG(27);
    DEBUG_PRINT("\n*************************\n");
    INIT_SERIAL_MSG(38);
    DEBUG_PRINT("nrf_write_FSR() executada com sucesso.");
    INIT_SERIAL_MSG(27);
    DEBUG_PRINT("\n*************************\n");
    return 0;
  } else {
    INIT_SERIAL_MSG(50);
    DEBUG_PRINT("\nFalha ao escrever em Flash status register (FSR).");
    return 1; //Verifique possiveis erros de escrita em bits inacessiveis
  }
}

/**
   Altera bit INFEN de FSR.
   Will re-direct general SPI read/write/erase
   commands from the flash MB to the IP.
   Except SPI command ERASE ALL, which will erase both MB and IP
   @return flag de erro: 0-ok | 1-Erro
*/
uint8_t nrf_connect_to_infopage() {
  uint8_t fsr_byte = nrf_read_FSR();
  fsr_byte |= 0x08; //Alterando bit INFEN para 1
  return nrf_write_FSR(fsr_byte);
}

/**
   Altera bit INFEN de FSR.
   Will not re-direct general SPI read/write/erase
   commands from the flash MB to the IP.
   @return flag de erro: 0-ok | 1-Erro
*/
uint8_t nrf_disconnect_from_infopage() {
  uint8_t fsr_byte = nrf_read_FSR();
  fsr_byte &= 0xF7; //Alterando bit INFEN para 0
  return nrf_write_FSR(fsr_byte);
}

/**
   Le um bloco endereçado de memoria.
   @param addr      Endereço de 16 bits do inicio da leitura
   @param data_size Quantidade de bytes para serem lidos, max = 1k
   @param data_read Vetor onde os dados lidos serão armazenados
*/
void nrf_read_flash(uint16_t addr, uint16_t data_size, uint8_t *data_read) {

  digitalWrite(FCSN_pin, LOW);
  SPI.beginTransaction(settings_nRF_SPI);
  SPI.transfer(CF_READ);

  SPI.transfer(addr >> 8); //MSB
  SPI.transfer((uint8_t) (addr && 0x0011)); //LSB

  for (int i = 0; i < data_size; i++) {
    data_read[i] = SPI.transfer(NULL_SPI);
  }

  digitalWrite(FCSN_pin, HIGH);
  SPI.endTransaction();
}

/**
     [x] Conectar-se a InfoPage.
     [x] Ler InfoPage.
     [] Verificar o identifier (0x00-0x0F).
     [] NumberOfUnprotectedPages (0x20): 0xFF.
     [] Verificar se ReadDisableMainblock (0x23): 0xFF.
     [] Verificar se EnableHWDebug (0x24): 0xFF.
     [x] Desconectar-se da InfoPage.
   @return  flag de erro: 0-ok | 1-Erro conectar na InfoPage | 2-Erro desconectar da InfoPage
*/
uint8_t nrf_connect_prog() {
  INIT_SERIAL_MSG(45);
  DEBUG_PRINT("\n************nrf_connect_prog()*************\n");
  if (nrf_connect_to_infopage()) {
    INIT_SERIAL_MSG(31);
    DEBUG_PRINT("Erro ao se conectar a InfoPage.");
    return 1; //Erro ao se conectar a InfoPage
  }
  INIT_SERIAL_MSG(42);
  DEBUG_PRINT("\n************lendo info page*************\n");
  nrf_read_flash(0x0000, 512, infopageData);
  if (nrf_disconnect_from_infopage()) {
    INIT_SERIAL_MSG(34);
    DEBUG_PRINT("Erro ao se desconectar a InfoPage.");
    return 2; //Erro ao se desconectar a InfoPage
  }
  INIT_SERIAL_MSG(27);
  DEBUG_PRINT("\n*************************\n");
  INIT_SERIAL_MSG(34);
  DEBUG_PRINT("Conectado a interface com sucesso.");
  INIT_SERIAL_MSG(27);
  DEBUG_PRINT("\n*************************\n");
  return 0;//Nenhum erro
}

/**************STEP 3***************/
/*Apagar código antigo.*/

/*
  Verifica se não esta na InfoPage e se possui backup dos dados de DSYS da InfoPage.
  Envia comando WEN.
  Envia comando SPI ERASE ALL.
  Usa comando SPI RDSR para saber se foi completada. (Como exatamente?).
*/
uint8_t nrf_erase_old_code() {

  if (nrf_read_FSR() && 0x08) { //Se estiver na InfoPage
    if (nrf_disconnect_from_infopage()) { //Tenta sair da InfoPage
      //Se nao saiu retorna erro
      INIT_SERIAL_MSG(55);
      DEBUG_PRINT("Erro ao desconectar-se da info page antes de erase all.");
      return 1;
    }
  }

  nrf_enable_wen();

  digitalWrite(FCSN_pin, LOW);
  SPI.beginTransaction(settings_nRF_SPI);

  SPI.transfer(CF_ERASE_ALL);//Enviando comando WREN

  digitalWrite(FCSN_pin, HIGH);
  SPI.endTransaction();

  uint8_t espera_maxima = 20; //2 segundos
  while (nrf_read_FSR() & 0x10 && espera_maxima) {
    delay(100);
    espera_maxima--;
  }
  if (nrf_read_FSR() & 0x10) {
    INIT_SERIAL_MSG(47);
    DEBUG_PRINT("Erro em nrf_erase_old_code(), timeout alcancado");
    return 2;
  } else {
    INIT_SERIAL_MSG(27);
    DEBUG_PRINT("\n*************************\n");
    INIT_SERIAL_MSG(44);
    DEBUG_PRINT("nrf_erase_old_code() executada com sucesso. ");
    INIT_SERIAL_MSG(27);
    DEBUG_PRINT("\n*************************\n");
    return 0;
  }
}

/**************STEP 4***************/
/*Gravar código novo.*/

/**
   Escreve um bloco endereçado de memoria.
   @param addr      Endereço de 16 bits do inicio da escrita
   @param data_size Quantidade de bytes para serem escritos, max = 1k
   @param data_write Vetor de dados a ser escrito
*/
void nrf_write_flash(uint8_t addr_h, uint8_t addr_l, uint16_t data_size, uint8_t *data_write) {

  nrf_enable_wen();

  digitalWrite(FCSN_pin, LOW);
  SPI.beginTransaction(settings_nRF_SPI);

  SPI.transfer(CF_PROGRAM);

  SPI.transfer(addr_h);//MSB
  SPI.transfer(addr_l);//LSB

  for (int i = 0; i < data_size; i++) {
    SPI.transfer(data_write[i]);
  }

  digitalWrite(FCSN_pin, HIGH);
  SPI.endTransaction();
}

/**
   Verifica se os proximos 512 bytes a partir de um endereço sao
   compostos por 0xFF, ou seja, se esta apagada.
   @param  page_addr endereco de 16 bits do primeiro byte da pagina
   @return           0 - tudo apagada. 1 - possui bytes nao nulos
*/
uint8_t nrf_verificar_se_esta_apagada(uint16_t page_addr) {
  nrf_read_flash(0x0000, 512, temporary_page_vector);
  uint8_t flag_non_null_byte = 0;
  for (int i = 0; i < 512; i++) {
    if ( temporary_page_vector[i] != 0xFF) {
      INIT_SERIAL_MSG(28);
      DEBUG_PRINT("\nExistem bytes nao apagados\n");
      flag_non_null_byte = 1;
    }
  }
  return flag_non_null_byte;
}
#define WRITE_PACKET_SIZE 256
/**
  [x] Verifica se foi apagado (realizar leitura).
  [x] Envia comando WEN.
  [x] Envia comando Program.
  [x] Realizar comando para cada 512 bytes de programa. (É possivel até 1k por vez.)
   @return  flag de erro: 0-ok | 1-Program memory nao esta apagada
*/
uint8_t nrf_write_new_code(uint16_t data_size, uint8_t *new_code) {
  if (nrf_verificar_se_esta_apagada(0x0000)) {
    return 1;
  }

  //Escrevendo em blocos de 512 bytes.
  uint16_t index_atual = 0x0000;
  while (index_atual < data_size) { //Enquanto tiver algum para escrever
    INIT_SERIAL_MSG(36);
    DEBUG_PRINT("\n******** Escrevendo codigo: ADDR = ");
    INIT_SERIAL_MSG(1);
    DEBUG_PRINTHEX(index_atual);
    if ((data_size - index_atual) <= WRITE_PACKET_SIZE) { //Se exitem menos de WRITE_PACKET_SIZE bytes para serem escritos
      INIT_SERIAL_MSG(10);
      DEBUG_PRINT(" - SIZE = ");
      INIT_SERIAL_MSG(1);
      DEBUG_PRINT(data_size - index_atual);
      nrf_write_flash((uint8_t) (index_atual >> 8), (uint8_t) index_atual, (data_size - index_atual), &new_code[index_atual]);
      index_atual = data_size;
    } else { //Se existem mais de WRITE_PACKET_SIZE bytes para serem escritos, escreve um pacote
      INIT_SERIAL_MSG(13);
      DEBUG_PRINT(" - SIZE = 128");
      nrf_write_flash((uint8_t) (index_atual >> 8), (uint8_t) index_atual, WRITE_PACKET_SIZE, &new_code[index_atual]);
      index_atual += WRITE_PACKET_SIZE;
    }
  }
  INIT_SERIAL_MSG(27);
  DEBUG_PRINT("\n*************************\n");
  INIT_SERIAL_MSG(44);
  DEBUG_PRINT("nrf_write_new_code() executada com sucesso. ");
  INIT_SERIAL_MSG(27);
  DEBUG_PRINT("\n*************************\n");
  return 0;
}

/**************STEP 5***************/
/*Verificar Escrita*/
uint8_t nrf_verify_bloco_flash(uint16_t addr, uint16_t data_size, uint8_t *code_to_verify) {

  nrf_read_flash(addr, data_size, temporary_page_vector);
#ifdef DEBUG_MODE
  Serial.print("\n");
  mostrar512(temporary_page_vector, addr);
#endif
  uint8_t flag_non_null_byte = 0;
  for (int i = 0; i < data_size; i++) {
    if ( temporary_page_vector[i] != code_to_verify[i]) {
      flag_non_null_byte = 1;
    }
  }

  return flag_non_null_byte;

}

//TODO: implement
/**
     [] Verificar InfoPage.
     [] Sair da InfoPage.
     [] Realizar leitura do código.
     [] Verificar se ele está igual ao que se desejava.
     [] Verificar se as paginas vazias realmente estão vazias.
   @param  code [description]
   @return      [description]
*/
uint8_t nrf_verify_code(uint16_t data_size, uint8_t *code_to_verify) {

  //Verificando em blocos de 512 bytes.
  uint16_t index_atual = 0x0000;
  while (index_atual < data_size) { //Enquanto tiver algum para escrever
    INIT_SERIAL_MSG(37);
    DEBUG_PRINT("\n******** Verificando codigo: ADDR = ");
    INIT_SERIAL_MSG(1);
    DEBUG_PRINTHEX(index_atual);

    if ((data_size - index_atual) <= 512) { //Se exitem menos de 512 bytes para serem escritos
      INIT_SERIAL_MSG(10);
      DEBUG_PRINT(" - SIZE = ");
      INIT_SERIAL_MSG(1);
      DEBUG_PRINT(data_size - index_atual);

      if (nrf_verify_bloco_flash(index_atual, (data_size - index_atual), &code_to_verify[index_atual])) {
        INIT_SERIAL_MSG(57);
        DEBUG_PRINT("\nFalha encontrada ao verificar, Existem bytes diferentes\n");
        return 1;
      }

      index_atual = data_size;
    } else { //Se existem mais de 512 bytes para serem escritos, escreve um pacote
      INIT_SERIAL_MSG(13);
      DEBUG_PRINT(" - SIZE = 512");

      if (nrf_verify_bloco_flash(index_atual, 512, &code_to_verify[index_atual])) {
        INIT_SERIAL_MSG(57);
        DEBUG_PRINT("\nFalha encontrada ao verificar, Existem bytes diferentes\n");
        return 1;
      }

      index_atual += 512;
    }
  }
  INIT_SERIAL_MSG(27);
  DEBUG_PRINT("\n*************************\n");
  INIT_SERIAL_MSG(41);
  DEBUG_PRINT("nrf_verify_code() executada com sucesso. ");
  INIT_SERIAL_MSG(27);
  DEBUG_PRINT("\n*************************\n");
  return 0;
}

/**************STEP 6***************/
/*Sair do modo e programação*/

/**
     [x] Enviar comando WEN.
     [x] Enviar comando RDISMB - Enable Read DISable of MainBlock).
     [x] Retorn níveis lógicos de prog e reset.
*/
void nrf_disconnect_prog() {
  nrf_enable_wen();

  digitalWrite(FCSN_pin, LOW);
  SPI.beginTransaction(settings_nRF_SPI);

  SPI.transfer(CF_RDISMB);//Enviando comando CF_RDISMB

  digitalWrite(FCSN_pin, HIGH);
  SPI.endTransaction();

  digitalWrite(PROG_pin, LOW);
  digitalWrite(RESET_pin, LOW); //pulse of minimun 0.2us
  delay(1);
  digitalWrite(RESET_pin, HIGH);
  delay(2);// you must wait at least 1.5 ms before you input the first flash command.
  INIT_SERIAL_MSG(27);
  DEBUG_PRINT("\n*************************\n");
  INIT_SERIAL_MSG(41);
  DEBUG_PRINT("Desconectado da interface de programacao.");
  INIT_SERIAL_MSG(27);
  DEBUG_PRINT("\n*************************\n");
}
