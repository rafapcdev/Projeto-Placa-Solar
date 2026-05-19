# Rastreador Solar Ativo com ESP32 (4 LDRs)

Este projeto implementa um sistema inteligente de rastreamento solar utilizando um microcontrolador ESP32, dois servomotores (para controle dos eixos horizontal e vertical) e quatro sensores LDR dispostos em quadrantes. 

Diferente dos sistemas de varredura simples, este rastreador opera de forma ativa em malha fechada, calculando continuamente a diferença de luminosidade entre as metades (superior/inferior e esquerda/direita) para inclinar os motores em direção ao ponto de maior incidência de luz.

## 🛠️ Hardware e Mapeamento de Pinos

| Componente | Pino no ESP32 | Descrição |
| :--- | :---: | :--- |
| **Servo Horizontal (Base)** | Pino 5 | Controla o movimento Esquerda / Direita |
| **Servo Vertical (Topo)** | Pino 18 | Controla o movimento Cima / Baixo |
| **LDR Superior Esquerdo (TL)** | Pino 32 | Sensor do quadrante superior esquerdo |
| **LDR Superior Direito (TR)** | Pino 33 | Sensor do quadrante superior direito |
| **LDR Inferior Esquerdo (BL)** | Pino 34 | Sensor do quadrante inferior esquerdo |
| **LDR Inferior Direito (BR)** | Pino 35 | Sensor do quadrante inferior direito |

## 💻 Dependências de Software

O projeto foi desenvolvido utilizando a **Arduino IDE** e requer as seguintes bibliotecas:
- `Arduino.h` (Nativa do ecossistema)
- `ESP32Servo.h` (Gerenciador de servos para a arquitetura do ESP32)

*Nota: A biblioteca `ESP32Servo` pode ser instalada diretamente pelo Gerenciador de Bibliotecas da Arduino IDE (Gerenciar Bibliotecas > Buscar por "ESP32Servo").*

## 🚀 Como Executar o Projeto

1. Certifique-se de ter o suporte às placas ESP32 instalado na sua Arduino IDE.
2. Instale a biblioteca `ESP32Servo`.
3. Abra o arquivo `Rastreador_Solar_ESP32.ino` localizado dentro da pasta de código.
4. Conecte seu ESP32 e selecione a porta COM e o modelo de placa correto (ex: *ESP32 Dev Module* ou *Lolin32 Lite*).
5. Ajuste a variável `tolerancia` no código se os motores estiverem oscilando muito devido à luz ambiente.
6. Faça o upload do código.