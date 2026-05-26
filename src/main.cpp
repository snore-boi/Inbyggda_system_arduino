#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <LiquidCrystal.h>

IPAddress serverIP(192, 168, 4, 10);
int serverport = 8080;

IPAddress arduinoIP(192, 168, 4, 20);

void startaspel(long sekunder);
void uppdateraLCD();

byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02};// skriv in mac som sheilden har

EthernetClient client;

LiquidCrystal lcd {12,11, 48, 46, 44, 42, 40, 38, 36,34}; // skriv in pins 

const int vittSpelare = 22;
const int orangeSpelare = 24;

const int blue = 26;
const int green = 28;
const int red = 30; 

long vitTid = 0;
long orangeTid = 0;
int dragVit = 0;
int dragOrange = 0;
bool utanTid = false;

enum SpelStatus { VANTAR_PA_SERVER, VITT_TANKER, ORANGE_TANKER, SPEL_SLUT};
SpelStatus status = VANTAR_PA_SERVER;

unsigned long  forraMillisekund = 0;

void setup() {
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(blue, OUTPUT);
  
  // Starta med rött ljus under anslutningsfasen
  digitalWrite(red, HIGH);
  digitalWrite(green, LOW);
  digitalWrite(blue, LOW);

  Serial.begin(9600);
  delay(1000); 

  lcd.begin(16,2);
  lcd.setCursor(0,0);
  lcd.print("Ansluter LAN...");
  Serial.println("Startar nätverk...");

  pinMode(vittSpelare, INPUT_PULLUP);
  pinMode(orangeSpelare, INPUT_PULLUP);

  // Starta nätverket på 192.168.4.X-gatan
  Ethernet.begin(mac, arduinoIP, serverIP, serverIP);
  delay(2000); // Ge skölden stabiliseringstid

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Soker server...");
  Serial.println("Letar efter Java-servern...");

  bool ansluten = false;
  int forsock = 0;

  // Försök ansluta upp till 10 gånger så att USB-adaptern hinner vakna!
  while (!ansluten && forsock < 10) {
    forsock++;
    Serial.print("Anslutningsforsok ");
    Serial.print(forsock);
    Serial.println(" av 10...");
    
    lcd.setCursor(0,1);
    lcd.print("Forsok: ");
    lcd.print(forsock);

    if (client.connect(serverIP, serverport)) {
      ansluten = true;
    } else {
      delay(1000); // Vänta 1 sekund innan nästa försök
    }
  }

  // Utvärdera hur det gick
  if (ansluten) {
    digitalWrite(red, LOW);
    digitalWrite(green, HIGH); // GRÖNT LJUS!
    digitalWrite(blue, LOW);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Ansluten!");
    lcd.setCursor(0,1);
    lcd.print("Vantar pA svar");
    Serial.println("Ansluten till servern! Väntar på starttid...");
  } else {
    digitalWrite(red, LOW);
    digitalWrite(green, LOW);
    digitalWrite(blue, HIGH); // BLÅTT LJUS (Standardspel)
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Server ej funnen");
    Serial.println("Kunde inte ansluta efter 10 forsok. Startar lokalt standardspel...");
    startaspel(300);
  }
}

int redValue;
int greenValue;
int blueValue;


void loop() {
  if(status == VANTAR_PA_SERVER && client.available()){
    String serverData = client.readStringUntil('\n');
    long startTidSekunder = serverData.toInt();

    client.stop();
    Serial.println("Tid mottagen fran servern. Stanger anslutningen och startar.");
    startaspel(startTidSekunder);
  }

  if(status == VITT_TANKER || status == ORANGE_TANKER){
    if(!utanTid){
      unsigned long nuvarandeMillisekund = millis();
      if(nuvarandeMillisekund - forraMillisekund >= 1000){
        forraMillisekund = nuvarandeMillisekund;

        if(status == VITT_TANKER){
          vitTid -=1000;
          if(vitTid <= 0){
            status = SPEL_SLUT;
          } 
        } else if(status == ORANGE_TANKER){
          orangeTid -= 1000;
          if(orangeTid <= 0){
            status = SPEL_SLUT;
          }
        }
        uppdateraLCD();
      }
    }

    if(status == VITT_TANKER && digitalRead(vittSpelare) == LOW){
      status = ORANGE_TANKER;
      dragVit++;
      Serial.println("Vit spelare gjorde ett drag. Orange tanker...");
      uppdateraLCD();
      delay(250);
    }

    if(status == ORANGE_TANKER && digitalRead(orangeSpelare) == LOW){
      status = VITT_TANKER;      
      dragOrange++;
      Serial.println("Orange spelare gjorde ett drag. Vit tanker...");
      uppdateraLCD();
      delay(250);
    }
  }

  if(status == SPEL_SLUT){
    digitalWrite(red, HIGH);
    digitalWrite(green, LOW);
    digitalWrite(blue, LOW);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(" TIDEN SLUT! ");
    lcd.setCursor(0,1);
    if(vitTid <= 0){
      lcd.print(" ORANGE VINNARE!");
      Serial.println("Spelet slut! Orange vann pa tid.");
    } else {
      lcd.print(" VITT VINNARE");
      Serial.println("Spelet slut! Vit vann pa tid.");
    }
    while (true);
  }
}

void startaspel(long sekunder){
  lcd.clear();
  if(sekunder <= 0){
    utanTid = true;
    Serial.println("Spel startat UTAN tid.");
  } else {
    utanTid = false;
    vitTid = sekunder * 1000;
    orangeTid = sekunder * 1000;
    Serial.print("Spel startat MED tid: ");
    Serial.print(sekunder);
    Serial.println(" sekunder.");
  }

  status = VITT_TANKER;
  forraMillisekund = millis();
  uppdateraLCD();
}

void uppdateraLCD(){
  if(utanTid){
    lcd.setCursor(0,0);
    lcd.print("Vit:  "); 
    lcd.print(dragVit);
    lcd.print(" drag  ");
    if(status == VITT_TANKER){
      lcd.setCursor(15, 0);
      lcd.print("<");
    } else {
      lcd.setCursor(15,0); 
      lcd.print(" ");
    }

    lcd.setCursor(0,1);
    lcd.print("Orange:");
    lcd.print(dragOrange);
    lcd.print(" drag  ");
    if(status == ORANGE_TANKER){
      lcd.setCursor(15,1);
      lcd.print("<");
    } else{
      lcd.setCursor(15,1);
      lcd.print(" ");
    }
  }

  else{
    int minVitt = (vitTid / 1000) / 60;
    int sekVitt = (vitTid / 1000) % 60;
    int minOrange = (orangeTid / 1000) / 60;
    int sekOrange = (orangeTid / 1000) % 60;

    lcd.setCursor(0,0);
    lcd.print("V:"); 
    if(minVitt < 10){
      lcd.print("0");
    }
    lcd.print(minVitt);
    lcd.print(":");
    if(sekVitt < 10){
      lcd.print("0");
    }
    lcd.print(sekVitt);

    lcd.print("D:");
    lcd.print(dragVit);
    lcd.print("   ");

    if(status == VITT_TANKER){
      lcd.setCursor(15,0);
      lcd.print("<");
    } else{
      lcd.setCursor(15,0);
      lcd.print(" ");
    }



    lcd.setCursor(0,1);
    lcd.print("O:"); 
    if(minOrange < 10){
      lcd.print("0");
    }
    lcd.print(minOrange);
    lcd.print(":");
    if(sekOrange < 10){
      lcd.print("0");
    }
    lcd.print(sekOrange);

    lcd.print(" D:");
    lcd.print(dragOrange);
    lcd.print("   ");
    if(status == ORANGE_TANKER){
      lcd.setCursor(15,1);
      lcd.print("<");
    } else{
      lcd.setCursor(15,1);
      lcd.print(" ");
    }
  }
}

