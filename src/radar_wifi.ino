#include <SoftwareSerial.h>

SoftwareSerial ESP01(2, 3);

/// Declaraciones de la media móvil
unsigned long EscaneoFinal = 0; ///Valor inical del tiempo en el que va recibir los datos el Arduino del ESP01.
const unsigned long IntervaloTiempo = 3000; ///Tiempo de espera para el muestreo local (bajado de 15s a 3s: ya no hay escaneo bloqueante que respetar aquí)
unsigned long UltimoEnvio = 0; ///Marca de tiempo del último envío exitoso a ThingSpeak
const unsigned long IntervaloEnvio = 15000; ///Tiempo de espera de 15s para cumplir con el límite de ThingSpeak
const int memoria = 10; ///Tamaño de la memoria de valores para la media
int LecturasRSSI[memoria]; ///Se declara la matriz o arreglo
int indice = 0; ///Indice de la media movil para el arreglo

///Inicialización de pines para los LEDs y buzzer. 
const int pinbuzzer = 7; 
const int pinLedVerde = 4;
const int pinLedAmarillo = 5;
const int pinLedRojo = 6;

///Inicialización de variables para el cálculo de distancia 
const float A = -76.0; /// "A" es la potencia de señal cuando está a 1 metro de distancia. **ES UN VALOR EXPERIMENTAL QUE CAMBIARÁ AL HACER LA PRUEBA REAL**
const float n = 2.0; /// "n" es la medida de qué tan rápido se degrada la fuerza de la señal de radiofrecuencia a medida que se aleja del emisor debido al entorno. 
///**ES UN VALOR EXPERIMENTAL QUE CAMBIARÁ AL HACER LA PRUEBA REAL**

/// Credenciales de ThingSpeak
const String apiKey = "BOIF1YURT7Q4685N";

void setup() {
  ESP01.begin(9600); ///Puerto ESP01
  Serial.begin(9600); ///Puerto de serie

  ///Los LEDs reciben voltaje.
  pinMode(pinbuzzer, OUTPUT);
  pinMode(pinLedVerde, OUTPUT);
  pinMode(pinLedAmarillo, OUTPUT);
  pinMode(pinLedRojo, OUTPUT);

  for (int i = 0; i < memoria; i++) { /// Inicializamos el arreglo para que quede en 0 y lo recorra el i.
    LecturasRSSI[i] = 0;
  }

  /// Secuencia visual de prueba al encender
  digitalWrite(pinLedVerde, HIGH);
  digitalWrite(pinLedAmarillo, HIGH);
  digitalWrite(pinLedRojo, HIGH);
  tone(pinbuzzer, 2000, 200);
  delay(1000); ///Espera un segundo
  digitalWrite(pinLedVerde, LOW);
  digitalWrite(pinLedAmarillo, LOW);
  digitalWrite(pinLedRojo, LOW);

  Serial.println("--- Inicializando ESP-01 ---");

  /// Esperar a que el módulo responda OK antes de continuar
  delay(2000);
  ESP01.println("AT");
  delay(1000);

  ESP01.println("AT+CWMODE=1"); ///Le pedimos al ESP01 que se configure para recibir los valores de la red
  delay(1000);

  /// Autenticación y conexión formal a la red locall
  bool conectado = false;
  while (!conectado) {
    Serial.println("Conectando a Wi-Fi...");
    ESP01.println("AT+CWJAP=\"Templario\",\"juliojose02\"");
    
    unsigned long inicioConexion = millis();
    while (millis() - inicioConexion < 8000) {
      if (ESP01.find("WIFI CONNECTED") || ESP01.find("OK")) {
        conectado = true;
        break;
      }
    }
    
    if (!conectado) {
      /// Destello rojo si falla el intento
      digitalWrite(pinLedRojo, HIGH);
      delay(500);
      digitalWrite(pinLedRojo, LOW);
      delay(1000);
    }
  }

  /// Confirmación visual de conexión exitosa
  digitalWrite(pinLedVerde, HIGH);
  tone(pinbuzzer, 1000, 100);
  delay(300);
  tone(pinbuzzer, 1500, 100);
  delay(500);
  digitalWrite(pinLedVerde, LOW);

  Serial.println("--- Radar Wi-Fi Inicializado y Conectado ---");
}

void loop() {
  /// Disparo periódico de la consulta de RSSI del AP ya conectado 
  if (millis() - EscaneoFinal >= IntervaloTiempo) {
    EscaneoFinal = millis();
    ESP01.println("AT+CWJAP?");
  }

  if (ESP01.available()) { 
    String incomingText = ESP01.readStringUntil('\n'); 
    
    if (incomingText.startsWith("+CWJAP:")) { 
      int ultimaComa = incomingText.lastIndexOf(",");
      String TxtRSSI = incomingText.substring(ultimaComa + 1); 
      TxtRSSI.trim();
      int RSSI = TxtRSSI.toInt(); 

      if (RSSI < 0) { 
        LecturasRSSI[indice] = RSSI; 
        indice++;

        if (indice >= memoria) { 
          indice = 0;
        }
        
        int sum = 0;
        for (int i = 0; i < memoria; i++) { 
          sum += LecturasRSSI[i]; 
        }
        
        int resultadoPotencia = sum / memoria; ///Cálculo del promedio

        float exponent = ((A - resultadoPotencia) / (10 * n)); ///FORMULA MATEMÁTICA
        float distancia = pow(10.0, exponent);

        Serial.print("RSSI Instantaneo: ");
        Serial.print(RSSI);
        Serial.print(" dBm | Promedio: ");
        Serial.print(resultadoPotencia);
        Serial.print(" dBm | Distancia aprox: ");
        Serial.print(distancia, 2); 
        Serial.println(" m");                       

        // Control de LEDs y Buzzer
        if (resultadoPotencia > -70) { 
          digitalWrite(pinLedVerde, HIGH);
          digitalWrite(pinLedAmarillo, LOW);
          digitalWrite(pinLedRojo, LOW);
          tone(pinbuzzer, 1500, 150);

        } else if (resultadoPotencia <= -70 && resultadoPotencia >= -82) {
          digitalWrite(pinLedVerde, LOW);
          digitalWrite(pinLedAmarillo, HIGH);
          digitalWrite(pinLedRojo, LOW);
          tone(pinbuzzer, 1000, 100);

        } else {
          digitalWrite(pinLedVerde, LOW);
          digitalWrite(pinLedAmarillo, LOW);
          digitalWrite(pinLedRojo, HIGH);
          noTone(pinbuzzer);
        } 

        /// Transmisión de datos a ThingSpeak (respetando el intervalo de 15s exigido por la API, separado del muestreo local)
        if (millis() - UltimoEnvio >= IntervaloEnvio) {
          UltimoEnvio = millis();
          enviarThingSpeak(RSSI, resultadoPotencia, distancia);
        }
      }
    } 
  }
}

/// Función auxiliar para estructurar y enviar la petición HTTP a ThingSpeak
void enviarThingSpeak(int rssi, int promedio, float dist) {
  ESP01.println("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80");
  delay(1000);

  String url = "GET /update?api_key=" + apiKey + "&field1=" + String(rssi) + "&field2=" + String(promedio) + "&field3=" + String(dist, 2) + "\r\n\r\n";
  String cipSend = "AT+CIPSEND=" + String(url.length());
  
  ESP01.println(cipSend);
  delay(500);
  ESP01.print(url);
  delay(1000);
  ESP01.println("AT+CIPCLOSE");
}
