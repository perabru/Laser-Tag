ESP32 LaserTag RC
Firmware para um veículo RC com sistema LaserTag por infravermelho, controlado por Bluetooth Classic através de um aplicativo Android.
O projeto utiliza:
ESP32
Driver de motores L298N
2 motores DC ou 2 lados de tração
LED infravermelho emissor
Receptor infravermelho TSOP/VS1838B
Buzzer ativo
LED de status
Comunicação Bluetooth com Android
---
1. Mapa de pinos
ESP32 → L298N
Função	ESP32	L298N
Motor esquerdo - IN1	GPIO 26	IN1
Motor esquerdo - IN2	GPIO 27	IN2
Motor direito - IN3	GPIO 32	IN3
Motor direito - IN4	GPIO 33	IN4
Terra comum	GND	GND
> Nesta versão, os jumpers **ENA** e **ENB** do L298N devem permanecer instalados.  
> Assim, os motores trabalham em velocidade máxima quando acionados.
---
ESP32 → Sistema LaserTag
Componente	Pino ESP32	Observação
LED infravermelho emissor	GPIO 4	Saída de disparo
Receptor IR TSOP/VS1838B	GPIO 34	Entrada de detecção
Buzzer ativo	GPIO 13	Feedback sonoro
LED de status	GPIO 2	Feedback visual
---
2. Diagrama simplificado de ligação
```text
                         ESP32
                    ┌───────────────┐
                    │               │
GPIO 26 ────────────┤ IN1           │
GPIO 27 ────────────┤ IN2           │
GPIO 32 ────────────┤ IN3           │
GPIO 33 ────────────┤ IN4           │
                    │               │
GPIO 4  ────────────┤ LED IR        │
GPIO 34 ────────────┤ RECEPTOR IR   │
GPIO 13 ────────────┤ BUZZER        │
GPIO 2  ────────────┤ LED STATUS    │
                    │               │
GND ─────────────────┤ GND           │
                    └───────────────┘
                           │
                           │
                           ▼
                      GND COMUM
                           │
           ┌───────────────┴───────────────┐
           │                               │
           ▼                               ▼
        L298N                        Sensores / IR
```
---
3. Ligação do L298N
O L298N é responsável por fornecer a corrente necessária para os motores.
Canal A - lado esquerdo
```text
ESP32 GPIO 26 → L298N IN1
ESP32 GPIO 27 → L298N IN2

L298N OUT1 → Motor esquerdo
L298N OUT2 → Motor esquerdo
```
Canal B - lado direito
```text
ESP32 GPIO 32 → L298N IN3
ESP32 GPIO 33 → L298N IN4

L298N OUT3 → Motor direito
L298N OUT4 → Motor direito
```
Alimentação
```text
Bateria +  → L298N +12V / VIN
Bateria -  → L298N GND

ESP32 GND  → L298N GND
```
MUITO IMPORTANTE
O GND da ESP32 e o GND do L298N precisam estar ligados juntos.
Sem o terra comum, os sinais enviados pela ESP32 ao driver podem não funcionar corretamente.
---
4. Ligação do receptor infravermelho
Para um módulo receptor VS1838B / TSOP:
```text
Receptor IR

VCC  → 3.3 V
GND  → GND
OUT  → GPIO 34
```
No firmware:
```cpp
const uint8_t IR_RECEIVE_PIN = 34;
```
O receptor identifica os disparos enviados pelos outros veículos.
---
5. Ligação do emissor infravermelho
O emissor é o responsável pelo "tiro" do LaserTag.
No firmware:
```cpp
const uint8_t IR_SEND_PIN = 4;
```
Ligação básica para testes:
```text
GPIO 4
  │
  ├── resistor
  │
  ▼
LED IR
  │
  ▼
 GND
```
Para obter maior alcance, recomenda-se utilizar um transistor para acionar o LED infravermelho.
Exemplo:
```text
                     +5 V
                      │
                     LED IR
                      │
                   Resistor
                      │
                      C
GPIO 4 ─ Resistor ── B   Transistor NPN
                      E
                      │
                     GND
```
O GPIO da ESP32 não deve alimentar um LED IR de alta potência diretamente.
---
6. Ligação do buzzer
No firmware:
```cpp
const uint8_t BUZZER_PIN = 13;
```
Para buzzer ativo:
```text
GPIO 13 → positivo do buzzer
GND     → negativo do buzzer
```
O buzzer é utilizado para indicar:
disparo;
acerto;
perda de vida;
fim da partida.
---
7. LED de status
O firmware utiliza:
```cpp
const uint8_t STATUS_LED_PIN = 2;
```
Em muitas placas ESP32 DevKit o GPIO 2 já está conectado ao LED integrado da placa.
Caso seja usado um LED externo:
```text
GPIO 2
  │
 resistor 220 Ω
  │
 LED
  │
 GND
```
---
8. Identificação de cada robô
Cada veículo deve possuir um identificador diferente.
No código:
```cpp
const uint16_t ROBOT_ID = 1;
```
Exemplo:
Robô 1
```cpp
const uint16_t ROBOT_ID = 1;
```
Bluetooth:
```text
RC-LASERTAG-1
```
Robô 2
```cpp
const uint16_t ROBOT_ID = 2;
```
Bluetooth:
```text
RC-LASERTAG-2
```
Robô 3
```cpp
const uint16_t ROBOT_ID = 3;
```
Bluetooth:
```text
RC-LASERTAG-3
```
Nunca utilize o mesmo `ROBOT_ID` em dois robôs que irão competir entre si.
---
9. Comandos Bluetooth
O aplicativo Android envia caracteres simples para a ESP32.
Comando	Função
`F`	Frente
`B`	Ré
`L`	Esquerda
`R`	Direita
`S`	Parar
`T`	Atirar
`Q`	Consultar status
`Z`	Reiniciar partida
Exemplo:
```text
F
```
A ESP32 faz o veículo andar para frente.
```text
S
```
A ESP32 para os motores.
```text
T
```
A ESP32 executa um disparo infravermelho.
---
10. Sistema de vidas
O firmware inicia cada partida com:
```cpp
const int INITIAL_LIVES = 3;
```
Portanto:
```text
VIDAS = 3

Acerto 1 → VIDAS = 2
Acerto 2 → VIDAS = 1
Acerto 3 → VIDAS = 0
```
Quando as vidas chegam a zero:
```text
FIM_DE_JOGO
```
Os motores são automaticamente desligados.
---
11. Proteção contra disparos repetidos
O firmware possui dois tempos importantes.
Tempo mínimo entre tiros
```cpp
const unsigned long SHOT_COOLDOWN_MS = 700;
```
O jogador precisa esperar aproximadamente:
```text
700 ms
```
entre um disparo e outro.
Invulnerabilidade após um acerto
```cpp
const unsigned long INVULNERABILITY_MS = 1000;
```
Depois de receber um tiro, o robô fica aproximadamente:
```text
1 segundo
```
sem poder perder outra vida.
Isso evita que o mesmo sinal infravermelho seja contado várias vezes.
---
12. Biblioteca necessária
Na Arduino IDE:
```text
Sketch
→ Include Library
→ Manage Libraries
```
Procure por:
```text
IRremote
```
Instale:
```text
Arduino-IRremote
```
O firmware utiliza:
```cpp
#include <IRremote.hpp>
```
Para o Bluetooth Classic:
```cpp
#include "BluetoothSerial.h"
```
A biblioteca Bluetooth já faz parte do suporte ESP32 da Arduino IDE.
---
13. Configuração da Arduino IDE
Selecione uma placa ESP32 compatível.
Exemplo:
```text
Tools
→ Board
→ ESP32 Arduino
→ ESP32 Dev Module
```
Configuração típica:
```text
Board: ESP32 Dev Module
Upload Speed: 921600 ou 115200
CPU Frequency: 240 MHz
Flash Frequency: 80 MHz
```
Escolha também a porta USB correta.
---
14. Pareamento com Android
Após enviar o firmware:
Ligue o veículo.
Ative o Bluetooth do celular.
Abra as configurações Bluetooth.
Procure por:
```text
RC-LASERTAG-1
```
Faça o pareamento.
Abra o aplicativo RC LaserTag.
Selecione o robô.
Pressione Conectar.
---
15. Fluxo de funcionamento
```text
          LIGAR ROBÔ
              │
              ▼
      ESP32 INICIALIZA
              │
              ▼
     BLUETOOTH É CRIADO
              │
              ▼
       ANDROID CONECTA
              │
              ▼
      ┌────────────────┐
      │ CONTROLE DO RC │
      └────────────────┘
              │
       ┌──────┴──────┐
       │             │
       ▼             ▼
 MOVIMENTAÇÃO      ATIRAR
       │             │
       │             ▼
       │         EMISSOR IR
       │             │
       │             ▼
       │         OUTRO ROBÔ
       │             │
       │             ▼
       │        RECEPTOR IR
       │             │
       │             ▼
       │        PERDE 1 VIDA
       │             │
       │             ▼
       │        LED + BUZZER
       │             │
       │             ▼
       │       VIDAS = ZERO?
       │          │      │
       │         NÃO    SIM
       │          │      │
       └──────────┘      ▼
                     FIM DO JOGO
```
---
16. Resumo completo dos GPIOs
```text
ESP32
│
├── GPIO 26 ── L298N IN1
├── GPIO 27 ── L298N IN2
├── GPIO 32 ── L298N IN3
├── GPIO 33 ── L298N IN4
│
├── GPIO 4  ── Emissor infravermelho
├── GPIO 34 ── Receptor infravermelho
│
├── GPIO 13 ── Buzzer
├── GPIO 2  ── LED de status
│
├── 3V3 ────── VCC receptor IR
└── GND ────── GND comum
```
---
17. Mapa visual dos pinos
```text
                    ESP32 DEVKIT
              ┌─────────────────────┐
              │                     │
      3V3  ───┤                     │─── VIN
              │                     │
GPIO 34 IR RX ├── 34                │
              │                     │
GPIO 32 IN3   ├── 32                │
GPIO 33 IN4   ├── 33                │
              │                     │
GPIO 26 IN1   ├── 26                │
GPIO 27 IN2   ├── 27                │
              │                     │
GPIO 13 BUZZ  ├── 13                │
              │                     │
GPIO 4 IR TX  ├── 4                 │
GPIO 2 LED    ├── 2                 │
              │                     │
      GND  ───┤ GND                 │
              │                     │
              └─────────────────────┘
```
---
18. Observações importantes
Os GPIOs deste README correspondem à implementação atual do firmware.
Caso a montagem física utilize outros pinos, altere as constantes no código.
Nunca ligue motores diretamente à ESP32.
Use o L298N ou outro driver de potência.
ESP32 e L298N devem compartilhar o mesmo GND.
Para maior alcance do LaserTag, utilize transistor no LED IR.
O GPIO 34 da ESP32 é somente entrada, sendo adequado para o receptor IR.
O código atual foi pensado para ESP32 clássico com Bluetooth Classic.
Para ESP32-C3 ou ESP32-S3, o sistema Bluetooth precisa ser adaptado para BLE.
---
19. Estrutura básica do projeto
```text
RC_LaserTag/
│
├── ESP32_LaserTag/
│   └── ESP32_LaserTag.ino
│
├── Android_LaserTag/
│   └── aplicativo Android
│
└── README.md
```
---
Projeto
Veículo RC com Sistema LaserTag por Infravermelho
Projeto educacional de robótica e sistemas embarcados utilizando ESP32, controle Bluetooth e comunicação infravermelha.
