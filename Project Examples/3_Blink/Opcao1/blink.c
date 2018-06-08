#include<Nordic\reg24le1.h> // Registradores do nrf24le1
#include<hal_delay.h> // HAL para as fun��es de delay

/**
 * Fun��o de inicializa��o das fun��es que ser�o utilizadas
 */
void setup(){
    // Inicializa os pinos do Port 0 e Port 1 todos como entradas.
    P0DIR = 0xFF; P1DIR = 0xFF;
    // Configura o pino 3 do Port 0 como sa�da
    P0DIR &= ~(1 << 3);
}

/**
 * Est� fun��o � chamada repetidivamente.
 */
void loop(){
    // Inverte o Pino 0 do Port 0
    P03 = !P03;
    // Aguarda 1s
    delay_ms(1000);
}

/**
 * Fun��o Main, indicando como funcionar� o sitema.
 * Ele est� simplificado para funcionar semelhante ao arduino.
 */
void main(){
    setup();
    while(1){
        loop();
    }
}