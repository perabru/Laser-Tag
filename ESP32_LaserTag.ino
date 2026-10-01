/*
  Veiculo RC Lasertag - ESP32 + Bluetooth Classic + Infravermelho
  Projeto baseado no TCC/INIC enviado pelo usuario.

  Pressupostos desta implementacao:
  - ESP32 classico (ESP32-WROOM/DevKit), com Bluetooth Classic.
  - Driver L298N com jumpers ENA e ENB instalados.
  - Dois canais de motor: lado esquerdo e lado direito.
  - Receptor IR TSOP/VS1838B em um unico GPIO.
  - LED IR emissor no GPIO indicado (para maior alcance, use transistor driver).
  - Buzzer ativo.

  Biblioteca adicional:
  - Arduino-IRremote (instalar pelo Library Manager da Arduino IDE)

  Protocolo Bluetooth:
    F = frente
    B = re
    L = esquerda
    R = direita
    S = parar
    T = atirar
    Q = consultar status
    Z = reiniciar vidas
*/

#include <Arduino.h>
#include "BluetoothSerial.h"
#include <IRremote.hpp>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error "Este firmware exige um ESP32 com Bluetooth Classic habilitado."
#endif

BluetoothSerial SerialBT;

// ---------- IDENTIDADE DO ROBO ----------
const uint16_t ROBOT_ID = 1;       // Mude para 2 no segundo robo, 3 no terceiro...
const uint8_t SHOT_COMMAND = 0xA5;

// ---------- PINOS L298N ----------
// Canal A = motor/lado esquerdo
const uint8_t IN1 = 26;
const uint8_t IN2 = 27;
// Canal B = motor/lado direito
const uint8_t IN3 = 32;
const uint8_t IN4 = 33;

// ---------- LASERTAG ----------
const uint8_t IR_SEND_PIN = 4;
const uint8_t IR_RECEIVE_PIN = 34;

// ---------- FEEDBACK ----------
const uint8_t STATUS_LED_PIN = 2;
const uint8_t BUZZER_PIN = 13;

// ---------- REGRAS DO JOGO ----------
const int INITIAL_LIVES = 3;
const unsigned long SHOT_COOLDOWN_MS = 700;
const unsigned long INVULNERABILITY_MS = 1000;

int lives = INITIAL_LIVES;
bool gameOver = false;
unsigned long lastShotAt = 0;
unsigned long lastHitAt = 0;

enum MotionState {
  STOPPED,
  FORWARD,
  BACKWARD,
  LEFT,
  RIGHT
};

MotionState motion = STOPPED;

// --------------------------------------------------------
// MOTORES
// --------------------------------------------------------
void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  motion = STOPPED;
}

void moveForward() {
  if (gameOver) return;

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  motion = FORWARD;
}

void moveBackward() {
  if (gameOver) return;

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  motion = BACKWARD;
}

void turnLeft() {
  if (gameOver) return;

  // Giro sobre o proprio eixo
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);

  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);

  motion = LEFT;
}

void turnRight() {
  if (gameOver) return;

  // Giro sobre o proprio eixo
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);

  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);

  motion = RIGHT;
}

// --------------------------------------------------------
// FEEDBACK
// --------------------------------------------------------
void beep(unsigned int durationMs) {
  // Para buzzer ativo.
  digitalWrite(BUZZER_PIN, HIGH);
  delay(durationMs);
  digitalWrite(BUZZER_PIN, LOW);
}

String motionName() {
  switch (motion) {
    case FORWARD:  return "FRENTE";
    case BACKWARD: return "RE";
    case LEFT:     return "ESQUERDA";
    case RIGHT:    return "DIREITA";
    default:       return "PARADO";
  }
}

void sendStatus() {
  SerialBT.print("STATUS;ID=");
  SerialBT.print(ROBOT_ID);
  SerialBT.print(";VIDAS=");
  SerialBT.print(lives);
  SerialBT.print(";MOV=");
  SerialBT.print(motionName());
  SerialBT.print(";ESTADO=");
  SerialBT.println(gameOver ? "FIM" : "JOGANDO");
}

void sendMessage(const String &msg) {
  Serial.println(msg);
  SerialBT.println(msg);
}

// --------------------------------------------------------
// DISPARO
// --------------------------------------------------------
void fireShot() {
  if (gameOver) {
    sendMessage("ERRO;JOGO_FINALIZADO");
    return;
  }

  unsigned long now = millis();

  if (now - lastShotAt < SHOT_COOLDOWN_MS) {
    sendMessage("ERRO;AGUARDE_RECARGA");
    return;
  }

  lastShotAt = now;

  // Endereco = ID de quem atirou
  // Comando = codigo padrao de tiro
  IrSender.sendNEC(ROBOT_ID, SHOT_COMMAND, 0);

  digitalWrite(STATUS_LED_PIN, HIGH);
  beep(35);
  digitalWrite(STATUS_LED_PIN, LOW);

  sendMessage("TIRO;OK");
}

// --------------------------------------------------------
// ACERTO
// --------------------------------------------------------
void registerHit(uint16_t shooterId) {
  unsigned long now = millis();

  if (gameOver) return;

  if (now - lastHitAt < INVULNERABILITY_MS) {
    return;
  }

  lastHitAt = now;
  lives--;

  // Feedback de acerto
  for (int i = 0; i < 2; i++) {
    digitalWrite(STATUS_LED_PIN, HIGH);
    beep(90);
    digitalWrite(STATUS_LED_PIN, LOW);
    delay(70);
  }

  SerialBT.print("ACERTO;ATIRADOR=");
  SerialBT.print(shooterId);
  SerialBT.print(";VIDAS=");
  SerialBT.println(lives);

  if (lives <= 0) {
    lives = 0;
    gameOver = true;
    stopMotors();

    digitalWrite(STATUS_LED_PIN, HIGH);
    beep(500);
    digitalWrite(STATUS_LED_PIN, LOW);

    sendMessage("FIM_DE_JOGO;VIDAS=0");
  }
}

void handleIR() {
  if (!IrReceiver.decode()) return;

  auto &data = IrReceiver.decodedIRData;

  if (data.protocol == NEC) {
    uint16_t shooterId = data.address;
    uint8_t command = data.command;

    if (command == SHOT_COMMAND && shooterId != ROBOT_ID) {
      registerHit(shooterId);
    }
  }

  IrReceiver.resume();
}

// --------------------------------------------------------
// BLUETOOTH
// --------------------------------------------------------
void processCommand(char command) {
  command = toupper(command);

  switch (command) {
    case 'F':
      moveForward();
      sendMessage("MOV;FRENTE");
      break;

    case 'B':
      moveBackward();
      sendMessage("MOV;RE");
      break;

    case 'L':
      turnLeft();
      sendMessage("MOV;ESQUERDA");
      break;

    case 'R':
      turnRight();
      sendMessage("MOV;DIREITA");
      break;

    case 'S':
      stopMotors();
      sendMessage("MOV;PARADO");
      break;

    case 'T':
      fireShot();
      break;

    case 'Q':
      sendStatus();
      break;

    case 'Z':
      lives = INITIAL_LIVES;
      gameOver = false;
      lastHitAt = 0;
      lastShotAt = 0;
      stopMotors();
      sendMessage("RESET;OK");
      sendStatus();
      break;

    default:
      SerialBT.print("ERRO;COMANDO_DESCONHECIDO=");
      SerialBT.println(command);
      break;
  }
}

void handleBluetooth() {
  while (SerialBT.available()) {
    char c = (char)SerialBT.read();

    // Ignora terminadores de linha e espacos
    if (c == '\n' || c == '\r' || c == ' ' || c == '\t') {
      continue;
    }

    processCommand(c);
  }
}

// --------------------------------------------------------
// SETUP / LOOP
// --------------------------------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  stopMotors();
  digitalWrite(STATUS_LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  IrSender.begin(IR_SEND_PIN);
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);

  String bluetoothName = "RC-LASERTAG-" + String(ROBOT_ID);
  SerialBT.begin(bluetoothName);

  Serial.println();
  Serial.println("======================================");
  Serial.println("  RC LASERTAG - ESP32");
  Serial.print("  Bluetooth: ");
  Serial.println(bluetoothName);
  Serial.println("======================================");

  // Sinaliza inicializacao
  digitalWrite(STATUS_LED_PIN, HIGH);
  beep(100);
  digitalWrite(STATUS_LED_PIN, LOW);

  sendStatus();
}

void loop() {
  handleBluetooth();
  handleIR();

  // Por seguranca, quando perde todas as vidas o carro fica parado.
  if (gameOver && motion != STOPPED) {
    stopMotors();
  }

  delay(2);
}
