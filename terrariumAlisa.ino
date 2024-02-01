#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <DHT.h>
#include <PubSubClient.h>

bool ledstat[] = {0, 0, 0};

int led[] = {14, 12, 13};
const int count = 3; 
const int DHTPIN = 4;

const char* ssid = "MI";
const char* password = "1285012850";

const char *mqtt_server = "driver.cloudmqtt.com";
const int mqtt_port = 18930;
const char *mqtt_user = "euwiegvl";
const char *mqtt_pass = "yf291C7qJY6R";

int temperatureC = 0;
WiFiServer server(80);
DHT dht(DHTPIN, DHT11);

int Datchikh = 0;
int Datchikt = 0;


struct RelaySchedule {
  int id;
  int HourIn;
  int MinuteIn;
  int HourOut;
  int MinuteOut;
};
RelaySchedule *alarms = new RelaySchedule[count];

struct EveryHour {
  int id;
  int time;
  int timein;
  int timeout;
  bool status;
};
EveryHour everyhour;

WiFiClient espClient;
PubSubClient client(espClient);

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // Serial.print("Message received on topic: ");
  // Serial.print(topic);
  // Serial.print(", Message: ");
  // Serial.println(message);

  Serial.print("MQTT: ");
  // Process the message, you can add your logic here

  // Example: Toggle LED based on the received topic
  if (strcmp(topic, "led1") == 0) {
    setLedState(led[0], !ledstat[0]);
    ledstat[0] = !ledstat[0];
    Serial.println("led1");
  } else if (strcmp(topic, "led2") == 0) {
    setLedState(led[1], !ledstat[1]);
    ledstat[1] = !ledstat[1];
    Serial.println("led2");
  } else if (strcmp(topic, "led3") == 0) {
    setLedState(led[2], !ledstat[2]);
    ledstat[2] = !ledstat[2];
    Serial.println("led3");
  }
}


void setup() {

  dht.begin();
 
  Serial.begin(9600);
  pinMode(led[0], OUTPUT);
  pinMode(led[1], OUTPUT);
  pinMode(led[2], OUTPUT);

  delay(1000);
  digitalWrite(led[0], HIGH);
  digitalWrite(led[1], HIGH);
  digitalWrite(led[2], HIGH);
  delay(1000);
  digitalWrite(led[0], LOW);
  digitalWrite(led[1], LOW);
  digitalWrite(led[2], LOW);
  delay(10);


  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");

    // Set up MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Subscribe to MQTT topics
  client.subscribe("led1");
  client.subscribe("led2");
  client.subscribe("led3");

  delay(200);
  digitalWrite(led[0], HIGH);
  digitalWrite(led[1], HIGH);
  digitalWrite(led[2], HIGH);
  delay(200);
  digitalWrite(led[0], LOW);
  digitalWrite(led[1], LOW);
  digitalWrite(led[2], LOW);
  

  server.begin();
  Serial.println("Server started");


  Serial.println(WiFi.localIP());
}

void loop() {


  time_t now = time(nullptr);
    struct tm *timeinfo;
    timeinfo = localtime(&now);

    if (everyhour.status == 1 && timeinfo->tm_min == 0 && timeinfo->tm_sec == 0 && timeinfo->tm_hour > everyhour.timein && timeinfo->tm_hour < everyhour.timeout) {
      setLedState(led[everyhour.id], 0);
      ledstat[everyhour.id] = !ledstat[everyhour.id];
      Serial.println(ledstat[0]);
    }

    if (everyhour.status == 1 && ledstat[everyhour.id] == 1 && timeinfo->tm_min == everyhour.time && timeinfo->tm_sec <= 4) {
      setLedState(led[everyhour.id], 1);
      ledstat[everyhour.id] = !ledstat[everyhour.id];
      Serial.println("N");
    }

    for (int i = 0; i < count; i++){
          
          if (timeinfo->tm_hour == alarms[i].HourIn && timeinfo->tm_min == alarms[i].MinuteIn && timeinfo->tm_sec == 0){
            Serial.println("Y");
            setLedState(led[i], 0);
            ledstat[i] = !ledstat[i];
            
            
          } else if (timeinfo->tm_hour == alarms[i].HourOut && timeinfo->tm_min == alarms[i].MinuteOut && timeinfo->tm_sec == 0){
            Serial.println("N");
            setLedState(led[i], 1);
            ledstat[i] = !ledstat[i];
          
          
          }
      }

  if (!client.connected()) {
  reconnect();
  }
  client.loop();

  // sensors.requestTemperatures(); 
  WiFiClient client = server.available();
  if (!client) {
    return;
  }



  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }

  String req = client.readStringUntil('\r');
  Serial.println(req);

  controller(req, client);
  client.flush(); 


  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n";
  s += "Datchikt: " + String(Datchikt) + "\r\n";
  s += "Datchikh: " + String(Datchikh) + "\r\n";


  client.print(s);
  delay(1);
  Serial.println("Client disonnected");



  // if (isnan(Datchikh) || isnan(Datchikt)) {  // Проверка. Если не удается считать показания, выводится «Ошибка считывания», и программа завершает работу
  //   Serial.println("Ошибка считывания");
  // }

}


void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("arduinoClient2", mqtt_user, mqtt_pass)) {
      Serial.println("Connected to MQTT broker");
      client.subscribe("led1");
      client.subscribe("led2");
      client.subscribe("led3");
    } else {
      Serial.println("Failed, retrying in 5 seconds");
      delay(5000);
    }
  }
}
void controller(String req, WiFiClient client) {

    if (req.indexOf("/temperature") != -1) {
      Datchikh = dht.readHumidity(); //Измеряем влажность
      Datchikt = dht.readTemperature(); //Измеряем температуру
      Serial.println(Datchikt);
      Serial.println("Showing temperature");
    } else if (req.indexOf("/led1") != -1) {
      setLedState(led[0], ledstat[0]);
      ledstat[0] = !ledstat[0];
    } else if (req.indexOf("/led2") != -1) {
      setLedState(led[1], ledstat[1]);
      ledstat[1] = !ledstat[1];
    } else if (req.indexOf("/led3") != -1) {
      setLedState(led[2], ledstat[2]);
      ledstat[2] = !ledstat[2];


    } else if (req.indexOf("/everyhour") != -1) {
      int id = req.substring(req.indexOf("id=") + 3, req.indexOf("&")).toInt();
      bool status = req.substring(req.indexOf("status=") + 7, req.indexOf("&timein=")).toInt();
      int timein = req.substring(req.indexOf("timein=") + 7, req.indexOf("&timeout=")).toInt();
      int timeout = req.substring(req.indexOf("timeout=") + 8, req.indexOf("&time=")).toInt();
      int time = req.substring(req.indexOf("time=") + 5).toInt();

      everyhour.id = id;
      everyhour.status = status;
      everyhour.timein = timein;
      everyhour.timeout = timeout;
      everyhour.time = time;

      Serial.print("id: "); Serial.println(everyhour.id);
      Serial.print("status: "); Serial.println(everyhour.status);
      Serial.print("timein: "); Serial.println(everyhour.timein);
      Serial.print("timeout: "); Serial.println(everyhour.timeout);
      Serial.print("time: "); Serial.println(everyhour.time);

    } else if (req.indexOf("/relay") != -1) {
        int id = req.substring(req.indexOf("id=") + 3, req.indexOf("&")).toInt();
        int hourIn = req.substring(req.indexOf("hourin=") + 7, req.indexOf("&minin=")).toInt();
        int minIn = req.substring(req.indexOf("minin=") + 6, req.indexOf("&hourout=")).toInt();
        int hourOut = req.substring(req.indexOf("hourout=") + 8, req.indexOf("&minout=")).toInt();
        int minOut = req.substring(req.indexOf("minout=") + 7).toInt();

        alarms[id].id = id;
        alarms[id].HourIn = hourIn;
        alarms[id].MinuteIn = minIn;
        alarms[id].HourOut = hourOut;
        alarms[id].MinuteOut = minOut;

        if (id < count){
          alarms[id].id = id;
          alarms[id].HourIn = hourIn;
          alarms[id].MinuteIn = minIn;
          alarms[id].HourOut = hourOut;
          alarms[id].MinuteOut = minOut;
        }
        Serial.print("Relay ID: "); Serial.println(alarms[id].id);
          Serial.print("HourIn: "); Serial.println(alarms[id].HourIn);
          Serial.print("MinIn: "); Serial.println(alarms[id].MinuteIn);
          Serial.print("HourOut: "); Serial.println(alarms[id].HourOut);
          Serial.print("MinOut: "); Serial.println(alarms[id].MinuteOut);
    } else {
        Serial.println("invalid request");
        client.stop();
        return;
    }
}


void setLedState(int led, bool state){
  if(!state){
    digitalWrite(led, HIGH);
  } else {
    digitalWrite(led, LOW);}
  }
