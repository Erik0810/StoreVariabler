/* inkluderer aktuelle bibliotek for å kunne gjøre at nodene kan kommunisere med hverandre over wifi */
#include <esp_now.h>
#include <WiFi.h>

// Oppbevarer informasjon om den andre noden denne noden skal kople seg med
esp_now_peer_info_t nodePult;

// Internett kanalen nodene operer på
#define CHANNEL 6

//oppretter variabler som setter lysene
const int rodLys1 = 2;
const int rodLys2 = 18;
const int gronnLys1 = 4;
const int gronnLys2 = 19;

// Oppretter variabler til ringeklokke-knappen
const int ringeklokke = 5;
int ringeklokkeVerdi = 0;
bool ringeklokkeTrykket = false;
bool erParet = false;
bool nodeFunnet = false;

// Initaliserer ESPNow-protokollen om initaliseringen feiler forsøker den å resta mikrokontrollen og starte programmet på nytt.
void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow initialisering suksess");
  }
  else {
    Serial.println("ESPNow initialisering feilet");
    ESP.restart();
  }
}

// setter opp ESP-NOW koplinge mellom den faste oppgitte enheten og lagrer nodePult som en peer med oppgitt. Endrer deretter erParet til true.
void setupNode(){
  nodePult.channel = CHANNEL;
  nodePult.encrypt = 0;
  uint8_t nodePult_mac[] = {0xC8, 0x2E, 0x18, 0x26, 0x73, 0xA0};
  memcpy(nodePult.peer_addr, nodePult_mac, sizeof(nodePult_mac));
  if(esp_now_add_peer(&nodePult) == ESP_OK){
    Serial.println("Node lagt til");
    erParet = true;
  }else{
  Serial.println("Feil under tilkopling av Node");    
    }
  }

// leter etter noden ved å skanne tilgjenglige wifi-nettverk, identifisere noden med spesifikke SSID og deretter bruke den enhetens MAC-adresse for å etablere ESP-NOW kommunikasjonen.
// Denne metoden er for å sikre at man kan finne riktig node om man har feil MAC-adresse på noden
void sjekkNode(){
  if(!nodeFunnet){
    Serial.println("Søker etter node...");
    int16_t scanResultat = WiFi.scanNetworks(false, false, false, 300, CHANNEL);
    if(scanResultat > 0){
      for(int i = 0; i< scanResultat; i++){
        String SSID = WiFi.SSID(i);
        if(SSID.indexOf("Mariell sin Iphone") == 0){
          Serial.println("Node funnet");
          nodeFunnet = true;
          String BSSIDstr = WiFi.BSSIDstr(i);
          int mac[6];
          if(6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5])){
            for(int ii = 0; ii<6; ++ii){
              nodePult.peer_addr[ii] = (uint8_t) mac[ii];
            }
            setupNode();
          }
          break;
        }
      }
    }
  }
}

// Denne metoden gjør at denne noden kopler seg til et eksisterende Wifi-nettverk
void configDeviceSTA() {
  const char *SSID = "Mariell sin Iphone";
  const char *password = "testeps1";
  WiFi.begin(SSID,password);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi tilkoplet");
}

// Sender data, altså knappens verdi til nodePult. 
void sendData(int verdi){
  if(erParet){
    uint8_t data = verdi;
    const uint8_t *nodePult_addr = nodePult.peer_addr;
    Serial.print("Sender: "); Serial.println(data);
    esp_err_t result = esp_now_send(nodePult_addr, &data, sizeof(data));
    Serial.print("Send status: ");
    if(result == ESP_OK){
      Serial.println("suksess");
    }else{
      Serial.println("Feil ved sending av data");
    }
  }
}

// En callback funksjon som skrives ut når dataoverføringen er ferdig sendt. 
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Sist pakke sendt til: "); Serial.println(macStr);
  Serial.print("Siste pakke sin sendestatus: "); Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

/* En callback funksjon som skrives ut om den mottatte dataen. Metoden skriver ut hvem man mottok dataen fra og hva dataen var. 
Om mottatt data er 2, skal rødt lys 1 og 2 skrus på og grønt lys 1 og 2 skal slås av.
*/ 
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Sist pakke mottatt fra: "); Serial.println(macStr);
  Serial.print("Siste pakke mottatt data: "); Serial.println(*data);
  Serial.println("");

  if(*data == 2){
    digitalWrite(rodLys1, HIGH);
    digitalWrite(rodLys2, HIGH);
    digitalWrite(gronnLys1, LOW);
    digitalWrite(gronnLys2, LOW);
  }else if (*data == 1){
    digitalWrite(rodLys1, LOW);
    digitalWrite(rodLys2, LOW);
    digitalWrite(gronnLys1, HIGH);
    digitalWrite(gronnLys2, HIGH);
  }
}
/* Oppsett av mikrokontrollen ESP32 og oppkopling mot eksisterende oppgitt nettverk. Her definerer vi også input
og output verdier som er satt til de ulike pins. Og vi initaliserer ESP-Now protokollen.*/
void setup() {
  Serial.begin(115200);
  Serial.println("ProsjektIN1060");
  WiFi.mode(WIFI_STA); //setter mikrokontrollen til å være StationMode- slik at den kan kople seg på eksisterende nettverk
  configDeviceSTA();
  Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());
  Serial.print("STA CHANNEL "); Serial.println(WiFi.channel());
  InitESPNow();
  setupNode();
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  pinMode(ringeklokke, INPUT_PULLUP);
  pinMode(rodLys1, OUTPUT);
  pinMode(rodLys2, OUTPUT);
  pinMode(gronnLys1, OUTPUT);
  pinMode(gronnLys2, OUTPUT);
}


/*selve hovedprogrammet der man først sjekker kontakten med nodePult, at de er på samme kanal og at de er paret. Deretter 
sjekker metoden om ringeklokke er trykket og hvis den er det sender den ringeklokkeverdien til nodePult. */
void loop() {
  sjekkNode();
  if(nodePult.channel == CHANNEL){
    if(erParet){
      bool ringeklokkeNyttTrykk = (digitalRead(ringeklokke) == LOW);
      
      if(ringeklokkeNyttTrykk && !ringeklokkeTrykket){
        ringeklokkeTrykket = true;
        ringeklokkeVerdi = (ringeklokkeVerdi == 0) ? 1 : 0;
        sendData(ringeklokkeVerdi);
      }else if(ringeklokkeNyttTrykk && ringeklokkeTrykket){
        ringeklokkeTrykket = false;
      }
  }else{
    Serial.println("Venter på at noden skal bli funnet");
  }
  delay(100);
}
}

