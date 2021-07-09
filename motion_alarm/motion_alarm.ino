#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <string.h>

// Replace with your network credentials
const char* ssid = "xxxxx";
const char* password = "xxxxx";

// Initialize Telegram BOT
#define BOTtoken "xxxxxxxxxx:xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"  // your Bot Token (Get from Botfather)

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "xxxxxxxx"

X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
unsigned long bot_lasttime; // last time messages' scan has been done

const unsigned long BOT_MTBS = 1000; // mean time between scan messages

const String id = "alarm1";
const int motionSensor = 14; // PIR Motion Sensor
const int buzzer = 12; // Buzzer
bool systemStatus = false;
bool mute = false;
bool detectionStatus = false;
bool motionDetected = false;
float sen;
int frequency;
bool stopAlarm = false;

// Indicates when motion is detected
void ICACHE_RAM_ATTR detectsMovement() {
  //Serial.println("MOTION DETECTED!!!");
  if(systemStatus)
  {
    motionDetected = true; 
  }  
}

void handleNewMessages(int numNewMessages)
{
  Serial.print("handleNewMessages ");
  Serial.println(numNewMessages);

  for (int i = 0; i < numNewMessages; i++)
  {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    Serial.print("text: ");
    Serial.println(text); 

    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Convidado";

    if (text.indexOf("/ring") != -1 && text.indexOf(id) != -1)
    {
      playBuzzer(5, buzzer); // play buzzer n times
    }

    if (text.indexOf("/turnon") != -1 && text.indexOf(id) != -1)
    {
      systemStatus = true;
      bot.sendMessage(chat_id, "Alarme está LIGADO", "");
    }
    
    if (text.indexOf("/turnoff") != -1 && text.indexOf(id) != -1)
    {
      systemStatus = false;
      bot.sendMessage(chat_id, "Alarme está DESLIGADO", "");
    }

    if (text.indexOf("/stop") != -1 && text.indexOf(id) != -1)
    {
      stopAlarm = true;
      noTone(buzzer);
    }

    if (text.indexOf("/mute") != -1 && text.indexOf(id) != -1)
    {
      if(mute) 
      {
        mute = false;
        bot.sendMessage(chat_id, "Mudo está DESLIGADO", "");
      }
      else 
      {
        mute = true;
        bot.sendMessage(chat_id, "Mudo está LIGADO", "");
      }
    }

    if (text.indexOf("/status") != -1 && text.indexOf(id) != -1)
    {
      if (systemStatus)
      {
        bot.sendMessage(chat_id, "Alarme está LIGADO", "");
      }
      else
      {
        bot.sendMessage(chat_id, "Alarme está DESLIGADO", "");
      }
    }

    if (text == "/devices")
    {
      bot.sendMessage(chat_id, id, "Markdown");
    }
  }
}

void playBuzzer(int reps, int out) {
  //cada repetição dura 0.9s
  for(int rep = 1; rep <= reps; rep++) {
   for(int x=0;x<180;x++){
    //converte graus para radiando e depois obtém o valor do seno
    sen = (sin(x*3.1416/180));
    //gera uma frequência a partir do valor do seno
    frequency = 2000+(int(sen*1000));
    tone(out,frequency);
    delay(5);
   }
   if(rep % 10 == 0)
   {
     checkForMessages();
     if(stopAlarm)
     {
      stopAlarm = false;
      break;
     }
   }
  }
  noTone(out);
}

void checkForMessages() {
  if (millis() - bot_lasttime > BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages)
    {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    bot_lasttime = millis();
  }
}

void setup() {
  Serial.begin(115200);
  configTime(0, 0, "pool.ntp.org");      // get UTC time via NTP
  client.setTrustAnchors(&cert); // Add root certificate for api.telegram.org

  // PIR Motion Sensor mode INPUT_PULLUP
  pinMode(motionSensor, INPUT_PULLUP);
  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);

  pinMode(buzzer, OUTPUT);

  // Attempt to connect to Wifi network:
  Serial.print("Conectando WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi Conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  bot.sendMessage(CHAT_ID, "BOT iniciou", "");
}

void loop() {
  if(motionDetected){
    bot.sendMessage(CHAT_ID, "Movimento detectado!!!", "");
    if(!mute)
    {
      bot.sendMessage(CHAT_ID, "Buzzer irá disparar em 30 segundos", "");
      Serial.println("Movimento detectado!!! Buzzer irá tocar em 30 segundos");
      delay(35000);
      playBuzzer(200, buzzer);
      noTone(buzzer);
    }
    else
    {
      bot.sendMessage(CHAT_ID, "Mudo está ativo. Não irá tocar", "");  
    }
    delay(10000);
    motionDetected = false;
  }

  checkForMessages();
}
