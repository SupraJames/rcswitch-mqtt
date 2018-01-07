#include <RCSwitch.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network
const char* ssid = "xxx";
const char* password = "xxx";
const char* mqtt_server = "smartie";
const char* controlTopic = "dunley/rfswitch/";
const char* messagesTopic = "messages";

// PIN where RF transmitter is connected
int RC_PIN = 5;

char* TS_ON_CODE[]={"F0F0FFFF0101", "F0F0FFFF1001", "F0F0FFF10001", "F0F0FF1F0001", "F0F0F1FF0001"};
char* TS_OFF_CODE[]={"F0F0FFFF0110", "F0F0FFFF1010", "F0F0FFF10010", "F0F0FF1F0010", "F0F0F1FF0010"};

RCSwitch mySwitch = RCSwitch();
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
byte switchStatus = 0; // a bit for each switch 1-8

void setup() {
  mySwitch.enableTransmit(RC_PIN);
  mySwitch.setPulseLength(182);
  pinMode(BUILTIN_LED, OUTPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
 
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  int topicLen = strlen(topic);
  int address = (topic[topicLen - 1] - '0');
  char statusTopic[50];
  char* statusMsg = "0";

  sprintf(statusTopic, "%s%d/status", controlTopic, address);

  switch (payload[0]) {
    case 's':
      Serial.print("Status query for address ");
      Serial.println(address);

      Serial.print("Will publish to ");
      Serial.println(statusTopic);
      
      if (bitRead(switchStatus, address)) {
        statusMsg = "1";
      } else {
        statusMsg = "0";
      }
      break;
    case 't':
      Serial.print("Toggle for address ");
      Serial.println(address);
   
      if (bitRead(switchStatus, address)) {
        if (address < 4) {
          mySwitch.switchOff(1, address);
        } else {
          mySwitch.sendTriState(TS_OFF_CODE[address - 4]);
        }
        bitClear(switchStatus, address);
      } else {
        if (address < 4) {
          mySwitch.switchOn(1, address);
        } else {
          mySwitch.sendTriState(TS_ON_CODE[address - 4]);
        }
      bitSet(switchStatus, address);
      }
      break;
    case '1':
      if (address < 4) {
        mySwitch.switchOn(1, address);
      } else {
        mySwitch.sendTriState(TS_ON_CODE[address - 4]);
      }
      bitSet(switchStatus, address);
      statusMsg = "1";
      break;
    case '0':
      if (address < 4) {
        mySwitch.switchOff(1, address);
      } else {
        mySwitch.sendTriState(TS_OFF_CODE[address - 4]);
      }
      bitClear(switchStatus, address);
      statusMsg = "0";
      break;
    }
    client.publish(statusTopic, statusMsg);
  }

void reconnect() {

  // Allocate a new buffer one char bigger than controlTopic
  char subTopic[strlen(controlTopic) + 2];
  // Copy the contents of controlTopic to it
  strncpy(subTopic,controlTopic,strlen(controlTopic));
  // Add a '+' as the final character
  subTopic[strlen(controlTopic)] = '+';
  subTopic[strlen(controlTopic) + 1] = '\0';
  
  //Serial.print("controlTopic: ");
  //Serial.println(controlTopic);
  Serial.print("subTopic: ");
  Serial.println(subTopic);
  //Serial.print("strlen controlTopic: ");
  //Serial.println(strlen(controlTopic));
  //Serial.print("strlen subTopic: ");
  //Serial.println(strlen(subTopic));
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266-RCSwitch")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(messagesTopic, "ESP8266-RCSwitch is ONLINE");
      // ... and resubscribe
      client.subscribe(subTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 3600000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 75, "ESP8266-RCSwitch is still ONLINE #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(messagesTopic, msg);
  }
}
