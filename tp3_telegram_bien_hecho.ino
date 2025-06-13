//KIM,AIZEN,RODRIGUEZ GRUPO 7 5A
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <U8g2lib.h>
#include <DHT.h>
#include <Wire.h>

// Pines y definiciones
#define SW1 35
#define SW2 34
#define LED 25
#define DHTPIN 23
#define DHTTYPE DHT11
#define BOTtoken "8144979266:AAF2a44-vr9p0QZr4ElCDBOh2q1-NJh-hwo"
#define CHAT_ID "7943275371"

#define P1 0
#define P2 1
#define ESPERA 2
#define SUMA 3
#define RESTA 4
#define SECUENCIA1 5
#define SECUENCIA2 6
#define SECUENCIA3 7
#define SECUENCIA4 8
#define SECUENCIA5 9

const char* ssid = "MECA-IoT";
const char* pass = "IoT$2025";

// Objetos
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);
DHT dht(DHTPIN, DHTTYPE);

// Variables globales
float temperatura = 0;
int umbral = 22;
bool alertaEnviada = false;


int estadoPantalla = P1;
unsigned long tiempoUltimoPaso = 0;
const unsigned long TIEMPO_LIMITE = 5000;

void TaskTelegram(void *pvParameters) {
  while (true) {
    int numMensajes = bot.getUpdates(bot.last_message_received + 1);
    if (numMensajes > 0) {
      for (int i = 0; i < numMensajes; i++) {
        String chat_id = bot.messages[i].chat_id;
        String mensaje = "Temperatura actual: " + String(temperatura, 1) + " °C\nUmbral: " + String(umbral) + " °C";
        bot.sendMessage(chat_id, mensaje, "");
      }
    }

    // Enviar alerta solo si supera el umbral y aún no se envió
    if (temperatura > umbral) &&!alertaEnviada) {
      String alerta = "ALERTA: Temperatura superó el umbral.\nTemp: " + String(temperatura, 1) + " °C";
      bot.sendMessage(CHAT_ID, alerta, "");
      digitalWrite(LED, HIGH);
      alertaEnviada = true;
    }

    // Si la temperatura baja, se desactiva la alerta
    if (temperatura <= umbral) {
      digitalWrite(LED, LOW);
      alertaEnviada = false;
    }
  }
}


// Tarea 2: Lógica principal y máquina de estado
void TaskPrincipal(void *pvParameters) {
  while (true) {
    unsigned long ahora = millis();
    float temp = dht.readTemperature();
    if (!isnan(temp)) {
      temperatura = temp;
    }

    if (temperatura > umbral) {
      digitalWrite(LED, HIGH);
    } else {
      digitalWrite(LED, LOW);
    }

    switch (estadoPantalla) {
      case P1:
        mostrarPantalla1();
        if (digitalRead(SW1) == LOW) {
          estadoPantalla = SECUENCIA1;
          tiempoUltimoPaso = ahora;
        }
        break;

      case SECUENCIA1:
        if (digitalRead(SW1) == HIGH) {
          estadoPantalla = SECUENCIA2;
          tiempoUltimoPaso = ahora;
        } else if (ahora - tiempoUltimoPaso > TIEMPO_LIMITE) {
          estadoPantalla = P1;
        }
        break;

      case SECUENCIA2:
        if (digitalRead(SW2) == LOW) {
          estadoPantalla = SECUENCIA3;
          tiempoUltimoPaso = ahora;
        } else if (ahora - tiempoUltimoPaso > TIEMPO_LIMITE) {
          estadoPantalla = P1;
        }
        break;

      case SECUENCIA3:
        if (digitalRead(SW2) == HIGH) {
          estadoPantalla = SECUENCIA4;
          tiempoUltimoPaso = ahora;
        } else if (ahora - tiempoUltimoPaso > TIEMPO_LIMITE) {
          estadoPantalla = P1;
        }
        break;

      case SECUENCIA4:
        if (digitalRead(SW1) == LOW) {
          estadoPantalla = SECUENCIA5;
          tiempoUltimoPaso = ahora;
        } else if (ahora - tiempoUltimoPaso > TIEMPO_LIMITE) {
          estadoPantalla = P1;
        }
        break;

      case SECUENCIA5:
        if (digitalRead(SW1) == HIGH) {
          estadoPantalla = P2;
        } else if (ahora - tiempoUltimoPaso > TIEMPO_LIMITE) {
          estadoPantalla = P1;
        }
        break;

      case P2:
        mostrarPantalla2();
        if(digitalRead(SW1) == LOW && digitalRead(SW2) == LOW){
          estadoPantalla = ESPERA;
        } else if(digitalRead(SW1) == LOW){
          estadoPantalla = SUMA;
        } else if(digitalRead(SW2) == LOW){
          estadoPantalla = RESTA;
        }
        break;

      case ESPERA:
        if(digitalRead(SW1) == HIGH && digitalRead(SW2) == HIGH){
          estadoPantalla = P1;
        }
        break;

      case SUMA:
        if(digitalRead(SW1) == LOW && digitalRead(SW2) == LOW){
          estadoPantalla = ESPERA;
        }
        if(digitalRead(SW1) == HIGH){
          estadoPantalla = P2;
          umbral++;
        }
        break;

      case RESTA:
        if(digitalRead(SW1) == LOW && digitalRead(SW2) == LOW){
          estadoPantalla = ESPERA;
        }
        if(digitalRead(SW2) == HIGH){
          estadoPantalla = P2;
          umbral--;
        }
        break;
    }
  }
}


// Pantalla 1
void mostrarPantalla1() {
  char tempStr[10], umbralStr[10];
  dtostrf(temperatura, 4, 1, tempStr);
  sprintf(umbralStr, "%d C", umbral);

  display.clearBuffer();
  display.setFont(u8g2_font_ncenB12_tr);
  display.drawStr(10, 20, "TEMP:");
  display.drawStr(70, 20, tempStr);
  display.drawStr(10, 40, "UMBRAL:");
  display.drawStr(100, 40, umbralStr);
  display.sendBuffer();
}

// Pantalla 2
void mostrarPantalla2() {
  char umbralStr[10];
  sprintf(umbralStr, "%d C", umbral);

  display.clearBuffer();
  display.setFont(u8g2_font_ncenB12_tr);
  display.drawStr(10, 30, "UMBRAL:");
  display.drawStr(90, 50, umbralStr);
  display.sendBuffer();
}


// Setup general
void setup() {
  Serial.begin(115200);
  dht.begin();
  display.begin();
  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);
  pinMode(LED, OUTPUT);

  // WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi conectado");
  client.setInsecure();  // Telegram usa SSL

  // Crear tareas
  xTaskCreatePinnedToCore(TaskTelegram, "Telegram", 10000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(TaskPrincipal, "Principal", 10000, NULL, 1, NULL, 1);
  
}
void loop() {
  
}
