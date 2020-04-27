/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com/esp8266-nodemcu-access-point-ap-web-server/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/

// Import required libraries
#include <FS.h>
#include <Wire.h>
#include <Arduino.h>
// #include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <max6675.h>
#include <EEPROM.h>

int addr = 0;
 
int8_t thermoDO = D6;
int8_t thermoCSA = D7;
int8_t thermoCSB = D5;
int8_t thermoCLK = D8;

MAX6675 thermocoupleA;
MAX6675 thermocoupleB;

const char* ssid     = "ESP8266-Access-Point";
const char* password = "12345678";


// current temperature & humidity, updated in loop()
double ta = 0.0;
double ta_offset = 0; 
double tb = 0.0;
double tb_offset = 1.0; 

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;    // will store last time DHT was updated

unsigned long previousMillisMemory = 0; 

// Updates readings every 10 seconds
const long interval = 10000;  
const long intervalMemory = 1800000;  

const String file_name = "/file.txt";
// current timestamp 1586895132 
// 20years to seconds 631138519

int timestamp =  955756613;

File file; 
const char graph_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head></head> 
<body>
  <p>
    <span>Temperature</span> 
    <span>%TEMPERATURE%</span> 
  </p>
</body>  

</html>)rawliteral";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <p>
    <span class="dht-labels">TemperatureA</span> 
    <span id="temperature">%TEMPERATUREA%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <span class="dht-labels">TemperatureB</span>
    <span id="humidity">%TEMPERATUREB%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <span class="data">%DATA%</span>
  
</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/get_temperature", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;
</script>
</html>)rawliteral";

String readFile(){ 
  String output = ""; 
  String line = ""; 

  bool success = SPIFFS.begin(); 
  
  if (success) {
    Serial.println("File system mounted with success");
  } else {
    Serial.println("Error mounting the file system");
    return String();
  }
  File local_file = SPIFFS.open(file_name, "r");
  if (!local_file) {
    Serial.println("Error opening file for writing");
    return String(); 
  }
  Serial.println("Starting Reading File"); 
  while(local_file.available()){
    Serial.println("File available"); 
    line = local_file.readString();
    Serial.println("Line:"); 
    Serial.println(line);
    output += line; 
  }
  return output; 
}

String readDataFile(){ 
  String output = ""; 
  String line = ""; 

  bool success = SPIFFS.begin(); 
  
  if (success) {
    Serial.println("File system mounted with success");
  } else {
    Serial.println("Error mounting the file system");
    return String();
  }
  File local_file = SPIFFS.open(file_name, "r");
  if (!local_file) {
    Serial.println("Error opening file for writing");
    return String(); 
  }
  Serial.println("Starting Reading File"); 
  while(local_file.available()){
    Serial.println("File available"); 
    line = local_file.readString();
    Serial.println("Line:"); 
    Serial.println(line);
    output += line; 
  }
  return output; 
}

size_t writeToFile(String text){ 
  bool success = SPIFFS.begin(); 
  
  if (success) {
    Serial.println("File system mounted with success");
  } else {
    Serial.println("Error mounting the file system");
    return 0;
  }
  File local_file = SPIFFS.open(file_name, "a");
  if (!local_file) {
    Serial.println("Error opening file for writing");
    return 0; 
  }

  Serial.println("Starting Writing File"); 

  int bytesWritten = local_file.println(text);
  
  if (bytesWritten > 0) {
    Serial.println("File was written");
  } else {
    Serial.println("File write failed");
    Serial.println(bytesWritten);
  }
  return bytesWritten; 
}

// Replaces placeholder with DHT values
String processor(const String& var){
  Serial.println("Processor");
  Serial.println(var);
  if(var == "TEMPERATUREA"){
    return String(ta);
  }
  else if(var == "TEMPERATUREB"){
    return String(tb);
  }else if( var == "DATA"){
  String data = readDataFile(); 
  data.replace("\n", "<br>"); 
  return data; 
  }
  return String();
}

String graph_processor(const String& var){ 
  Serial.println("Processor");
  Serial.println(var);
  String output = ""; 
  String line = ""; 

  bool success = SPIFFS.begin(); 
  
  if (success) {
    Serial.println("File system mounted with success");
  } else {
    Serial.println("Error mounting the file system");
    return String();
  }
  File local_file = SPIFFS.open(file_name, "r");
  if (!local_file) {
    Serial.println("Error opening file for writing");
    return String(); 
  }
  if(var == "TEMPERATURE"){
    Serial.println("Temperature start"); 
    while(local_file.available()){
      Serial.println("File available"); 
      line = local_file.readString();
      Serial.println("Line:"); 
      Serial.println(line);
      output += line; 
    }

    Serial.println("Output:"); 
    Serial.println(output); 
    local_file.close(); 
    return output;
  }
  return String();
}

String get_temperatureA(){
  return String(ta);
}


void setup(){
  // Serial port for debugging purposes
  Serial.begin(9600);
/*
  bool success = SPIFFS.begin(); 
  
  if (success) {
    Serial.println("File system mounted with success");
  } else {
    Serial.println("Error mounting the file system");
    return;
  }
 
  file = SPIFFS.open("/file.txt", "w+");
 
  if (!file) {
    Serial.println("Error opening file for writing");
    return;
  }
 */
  Serial.print("Setting thermocouple...");
  thermocoupleA.begin(thermoCLK, thermoCSA, thermoDO);
  thermocoupleB.begin(thermoCLK, thermoCSB, thermoDO);

  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", graph_html, graph_processor);
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(tb).c_str());
  });

  server.on("/get_temperatureA", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(ta).c_str());
  });
  // Start server
  server.begin();
}

void loop(){  
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you updated the DHT values
    previousMillis = currentMillis;
    // Read temperature as Celsius (the default)

    // Read temperature as Fahrenheit (isFahrenheit = true)
    //float newT = dht.readTemperature(true);
    // if temperature read failed, don't change t value
    double newTA = thermocoupleA.readCelsius(); // dht.readHumidity(); 
    
    // if humidity read failed, don't change h value 
    if (isnan(newTA)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      ta = newTA + ta_offset;
    }
    
    double newTB = thermocoupleB.readCelsius(); // dht.readHumidity(); 
    
    // if humidity read failed, don't change h value 
    if (isnan(newTB)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      tb = newTB+tb_offset;
    }

    // Read Humidity
    Serial.print("Temp A:"); 
    Serial.print(ta);
    Serial.println(" C ");

    Serial.print("Temp B:"); 
    Serial.print(tb);
    Serial.println(" C ");

    Serial.println(); 
  }

 if (currentMillis - previousMillisMemory >= intervalMemory) {
    previousMillisMemory = currentMillis;

    writeToFile(String(ta)+ " "+String(tb)+"; "); 
  }
}