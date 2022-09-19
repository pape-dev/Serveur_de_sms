//Librairies & bibliothèques
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>// https://github.com/me-no-dev/AsyncTCP
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>//https://github.com/me-no-dev/ESPAsyncWebServer
#include <FS.h>
#include <SD.h>
#include <SPI.h>

// Le code PIN de la carte sim
const char simPIN[] = "";

// Votre numéro de téléphone pour envoyer des SMS : + (signe +) et l'indicatif du pays, 

//#define SMS_TARGET "+33698739661"

// Configuration TinyGSM library
#define TINY_GSM_MODEM_SIM800 // Le modem est SIM800
#define TINY_GSM_RX_BUFFER 1024 // Définir le tampon RX sur 1Ko

#include <Wire.h>
#include <TinyGsmClient.h>

//Définitions des broches TTGO T-Call
#define MODEM_RST 5
#define MODEM_PWKEY 4
#define MODEM_POWER_ON 23
#define MODEM_TX 27
#define MODEM_RX 26
#define I2C_SDA 21
#define I2C_SCL 22

// Définir la série pour la console de débogage (sur le moniteur Série, vitesse par défaut 115200)
#define SerialMon Serial

// Définir la série pour les commandes AT (vers le module SIM800)
#define SerialAT Serial1

// Définir la console série pour les impressions de débogage, si nécessaire
//#define DUMP_AT_COMMANDS

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

#define IP5306_ADDR 0x75
#define IP5306_REG_SYS_CTL0 0x00

//Initialiser un AsyncWebServerobjet sur le port 80 pour configurer le serveur Web.
AsyncWebServer server(80);

//Insérez les identifiants du réseau Wifi
const char* ssid = "ESTIAM Wifi";  //nom du réseau 
const char* password = "r8d8c3p0r2d2";  // mot de passe

//Demande sur une URL invalide, nous appelons le "page 404 non trouvé"()fonction, définie au début de l'esquisse.

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

//Remplace l'espace réservé dans le fichier html
String editHTML(const String& var) {
  return "test";
}

// Démarrage du serveur pour gérer les clients
void setup() {
  Serial.begin(115200);
  
  // Set GSM module baud rate and UART pins
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);

  //---------------Connexion au réseau local -----------------
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("WiFi Failed!\n");
    return;
  }
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  

// Redémarrez le module SIM800, cela prend un certain temps Pour l'ignorer, appelez init() au lieu de restart()
  Serial.println("Initializing modem...");
  modem.restart();

  
  // Initialiser la librairie SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

// Lorsque vous accédez à l'URL de la route, vous envoyez la page HTML au client. Dans ce cas, le texte HTML est enregistré sur leindex_htmlvariable.
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false, editHTML);
    
    String phoneParam = "phone"; //  <IP>/get?led=<state>
    String phone;
    String msgParam = "msg"; //  <IP>/get?servo=<pos>
    String msg;

    if (request->hasParam(phoneParam)) {
      phone = request->getParam(phoneParam)->value(); //Get the value
      msg = request->getParam(msgParam)->value(); //Get the value
      Serial.println("Numero : " + phone);
      Serial.println("Message : " + msg);
      message(phone, msg);
      request->send(SPIFFS, "/index.html", String(), false, editHTML);
    }  else {
      request->send(200, "text/plain", "ERROR");
    }
  });
  server.serveStatic("/", SPIFFS, "/");
  server.onNotFound(notFound);
  server.begin();  //démarrer le serveur



// Conserver l'alimentation lors de l'exécution à partir de la batterie
  Wire.begin(I2C_SDA, I2C_SCL);
  bool isOk = setPowerBoostKeepOn(1);
  Serial.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FAIL"));

  // Set modem reset, enable, power pins
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);
  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);
}

void loop() {
}

void message(String number, String message) {

 // Déverrouillez la carte SIM avec un code PIN si nécessaire
  while (strlen(simPIN) && modem.getSimStatus() != 1 ) {
    Serial.print("PIN failed... ");
    Serial.print(modem.getSimStatus());
    modem.init();
    delay(2000);
    modem.simUnlock(simPIN);
    delay(2000);
    Serial.println("Reboot");
  }

  if (modem.sendSMS(number, message)) {
    Serial.println(message);
  } else {
    Serial.println("SMS failed to send");
  }
  //contenu principal 
 
}

bool setPowerBoostKeepOn(int en) {
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(IP5306_REG_SYS_CTL0);
  if (en) {
    Wire.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
  } else {
    Wire.write(0x35); // 0x37 is default reg value
  }
  return Wire.endTransmission() == 0;
}
