#include <Arduino.h>
#include <ESP32Servo.h>

// --- Definição dos Pinos dos Servos ---
const int servoPinHoriz = 5; // Servo da base (Esquerda/Direita)
const int servoPinVert = 18; // Servo de elevação (Cima/Baixo)

// --- Definição dos Pinos dos LDRs ---
const int ldrTL = 32; // Top-Left (Cima Esquerda)
const int ldrTR = 33; // Top-Right (Cima Direita)
const int ldrBL = 34; // Bottom-Left (Baixo Esquerda)
const int ldrBR = 35; // Bottom-Right (Baixo Direita)

// --- Criação dos Objetos dos Servos ---
Servo Servohorizontal;
Servo Servovertikal;

// --- Variáveis de Posição Inicial ---
int posHoriz = 90; // Começa centralizado
int posVert = 90;  // Começa apontando para cima/meio

// Limites de segurança para não forçar os motores
const int limitHorizMin = 10;
const int limitHorizMax = 170;
const int limitVertMin = 30;
const int limitVertMax = 150;

// Tolerância para evitar que o servo fique tremendo
const int tolerancia = 50;

void setup() {
  Serial.begin(115200);
  Serial.println("\nIniciando Rastreador Solar Ativo (4 LDRs)...");

  // Configura a resolução de leitura analógica para 12 bits (0 a 4095)
  analogReadResolution(12);

  // Inicializa os Servos
  Servohorizontal.attach(servoPinHoriz);
  Servovertikal.attach(servoPinVert);

  // Move os servos para a posição inicial
  Servohorizontal.write(posHoriz);
  Servovertikal.write(posVert);
  delay(1000);
}

void loop() {
  // 1. Faz a leitura dos 4 sensores LDR
  int valTL = analogRead(ldrTL);
  int valTR = analogRead(ldrTR);
  int valBL = analogRead(ldrBL);
  int valBR = analogRead(ldrBR);

  // 2. Calcula as médias das metades (Cima, Baixo, Esquerda, Direita)
  int mediaCima = (valTL + valTR) / 2;
  int mediaBaixo = (valBL + valBR) / 2;
  int mediaEsquerda = (valTL + valBL) / 2;
  int mediaDireita = (valTR + valBR) / 2;

  // 3. Calcula a diferença de luminosidade entre os eixos
  int difVert = mediaCima - mediaBaixo;
  int difHoriz = mediaEsquerda - mediaDireita;

  // 4. Lógica de movimentação Vertical (Cima/Baixo)
  if (abs(difVert) > tolerancia) {
    if (mediaCima > mediaBaixo) {
      posVert--; // Luz mais forte em cima, levanta
    } else {
      posVert++; // Luz mais forte embaixo, abaixa
    }
  }

  // 5. Lógica de movimentação Horizontal (Esquerda/Direita)
  if (abs(difHoriz) > tolerancia) {
    if (mediaEsquerda > mediaDireita) {
      posHoriz++; // Luz mais forte na esquerda, vira para a esquerda
    } else {
      posHoriz--; // Luz mais forte na direita, vira para a direita
    }
  }

  // 6. Garante que os ângulos não ultrapassem os limites físicos
  if (posHoriz > limitHorizMax)
    posHoriz = limitHorizMax;
  if (posHoriz < limitHorizMin)
    posHoriz = limitHorizMin;
  if (posVert > limitVertMax)
    posVert = limitVertMax;
  if (posVert < limitVertMin)
    posVert = limitVertMin;

  // 7. Envia o novo comando para os motores
  Servohorizontal.write(posHoriz);
  Servovertikal.write(posVert);

  // Mostra no monitor serial para depuração
  Serial.print("Cima: ");
  Serial.print(mediaCima);
  Serial.print(" | Baixo: ");
  Serial.print(mediaBaixo);
  Serial.print(" | Esq: ");
  Serial.print(mediaEsquerda);
  Serial.print(" | Dir: ");
  Serial.println(mediaDireita);

  delay(50);
}