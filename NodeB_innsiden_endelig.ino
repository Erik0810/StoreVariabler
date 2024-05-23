/* inkluderer aktuelle bibliotek for å kunne gjøre at nodene kan kommunisere med hverandre over wifi */
#include <esp_now.h>
#include <WiFi.h>

//Oppbevarer informasjonen om den andre noden denne noden skal kople seg med
esp_now_peer_info_t nodeOversikt;

// Kanalen nodene operer på
#define CHANNEL 6

//Oppretter variabler som setter lysene
const int blaaLys = 19;
const int rodLys = 18;
const int gronnLys = 5;
const int rodKnapp = 2;
const int gronnKnapp = 4;

/* oppretter variabler til knappenes tilstand: om de er trykket eller ikke trykket. Oppretter også et 
tomt String-objekt som vil oppbevare knappen status */
bool rodknappTrykket = false;
bool gronnknappTrykket = false;
String status ="";

//oppretter variabler som tar vare på om noden er koplet sammen og paret
bool nodeFunnet = false;
bool erParet = false;

// Initaliserer ESPNow-protokollen om initaliseringen feiler forsøker den å resta mikrokontrollen og starte programmet på nytt.
void InitESPNow(){
  WiFi.disconnect();
  if(esp_now_init() == ESP_OK){
    Serial.println("ESP initalisert vellykket");
  }
  else{
    Serial.println("ESP initalisering feilet");
    ESP.restart();
  }
}

// setter opp ESP-NOW koplinge mellom den faste oppgitte enheten og lagrer nodePult som en peer med oppgitt. Endrer deretter erParet til true.
void setupNode(){
  nodeOversikt.channel = CHANNEL;
  nodeOversikt.encrypt = 0;
  uint8_t nodeOversikt_mac[] = {0xC8, 0x2E, 0x18, 0x24, 0x84, 0x94};
  memcpy(nodeOversikt.peer_addr, nodeOversikt_mac, sizeof(nodeOversikt_mac));
  if(esp_now_add_peer(&nodeOversikt) == ESP_OK){
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
              nodeOversikt.peer_addr[ii] = (uint8_t) mac[ii];
            }
            setupNode();
          }
          break;
        }
      }
    }
  }
}

// Denne funksjonen gjør at denne noden kopler seg til et eksisterende Wifi-nettverk
void configDeviceSTA() {
  const char *SSID = "Mariell sin Iphone";
  const char *password = "testeps1";
  WiFi.begin(SSID,password);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi conntected");
}

// Sender data, altså knappens verdi til nodePult. 
void sendData(int verdi) {
  if(erParet) {
    uint8_t data = verdi;
    const uint8_t *nodeOversikt_addr = nodeOversikt.peer_addr;
    Serial.print("Sending: "); Serial.println(data);
    esp_err_t result = esp_now_send(nodeOversikt_addr, &data, sizeof(data));
    Serial.print("Send Status: ");
    if (result == ESP_OK) {
      Serial.println("Suksess");
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
Om mottatt data er 1 og status er "grønn" skal den kalle på settBlaa()- metoden.
*/ 
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Sist pakke mottatt fra:  "); Serial.println(macStr);
  Serial.print("Siste pakke mottatt data:"); Serial.println(*data);
  Serial.println("");
  
  if(*data == 1 && status == "gronn"){
    settBlaa();
  }
}

/* * Oppsett av mikrokontrollen ESP32 og oppkopling mot eksisterende oppgitt nettverk. Her definerer vi også input
og output verdier som er satt til de ulike pins. Og vi initaliserer ESP-Now protokollen. */
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); //setter mikrokontrollen til å være StationMode- slik at den kan kople seg på eksisterende nettverk
  Serial.println("ProsjektIN1060");
  configDeviceSTA();
  Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());
  Serial.print("STA CHANNEL "); Serial.println(WiFi.channel());
  InitESPNow();
  setupNode();
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  pinMode(rodKnapp, INPUT_PULLUP);
  pinMode(gronnKnapp,INPUT_PULLUP);
  pinMode(blaaLys, OUTPUT);
  pinMode(rodLys, OUTPUT);
  pinMode(gronnLys, OUTPUT);
}

//setter lyset til å lyse rødt og endrer status variabelen til å være rød
void settRod(){
  digitalWrite(rodLys,LOW);
  digitalWrite(gronnLys,HIGH);
  digitalWrite(blaaLys,HIGH);
  status = "rod";
}

//setter lyset til å lyse grønt og endrer statusvariabelen til å være grønn
void settGronn(){
  digitalWrite(rodLys,HIGH);
  digitalWrite(gronnLys,LOW);
  digitalWrite(blaaLys,HIGH);
  status = "gronn";
}

//Setter lyset til å lyse blått. Om man blir ringt på vil blått lys blinke i 5 sekunder, før det stilles tilbake til det opprinnelige lyset.
void settBlaa(){
  digitalWrite(rodLys,HIGH);
  digitalWrite(gronnLys,HIGH);
  digitalWrite(blaaLys,LOW);
  int naaTid = millis();
  while(millis()-naaTid<5000){
    digitalWrite(blaaLys,LOW);
    delay(500);
    digitalWrite(blaaLys,HIGH);
    delay(500);
  }
  if(status == "gronn"){
    settGronn();
  } else {
    settRod();
  }
}

/*selve hovedprogrammet der man først sjekker kontakten med nodePult, at de er på samme kanal og at de er paret. Deretter 
sjekker metoden om grønnknapp er trykket, i så fall sender den verdien 1 til nodeOversikt, og kaller på metoden sett grønn.
Om den røde knappen er trykket ned vil den sende verdien 2 til nodeOversikt og sette lyset til rødt.*/
void loop() {
 sjekkNode();
  if (nodeOversikt.channel == CHANNEL) { 
    if (erParet && nodeFunnet) {
      if(digitalRead(gronnKnapp)==HIGH){
        sendData(1);
        settGronn();
      }
      if(digitalRead(rodKnapp)==HIGH){
        sendData(2);
        settRod();
      }
  }else{
    Serial.println("Venter på at noden skal bli funnet");
  }
  delay(100);
}
}
