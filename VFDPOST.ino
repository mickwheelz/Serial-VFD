#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

const char* ssid = "ssid";
const char* password = "pass";

ESP8266WebServer server(80);

int handleBrightness(int brightness) {

    Serial.write(0x04); //Code to set brightness
    if(brightness == 20) {
      Serial.write(0x20);
    }
    else if(brightness == 40) {
      Serial.write(0x40);
    }
    else if(brightness == 60) {
      Serial.write(0x60);
    }
    else if(brightness == 100) {
      Serial.write(0xFF);
    }
    else {
      Serial.write(0x1F); //code to reset display
      return 1;
    }
    return 0;
}

void handleScrolling(boolean scrolling) {
  scrolling ? Serial.write(0x12) : Serial.write(0x11);
}

void handleCursor(boolean csr) {
    csr ? Serial.write(0x13) : Serial.write(0x14);
}

void handleRoot() {
  server.send(200, "text/plain", "POST some text to /message to display it. The display is 2x20 chars. It must be in JSON format. eg. {\"message\":\"goes here\"}");
}

void handleConfig() {

  /*
   *  example config:
   *  {
   *    "brightness":20, //20, 40, 60, 100
   *    "scroll": true, //false
   *    "cursor": true, //false
   *  }
   */
  if(server.method() == HTTP_POST) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    
    if (error) {
      server.send(400, "text/plain", "deserializeJson() failed: " + (String)error.c_str());
      Serial.println(error.c_str());
      return;
    }
  
    int brightnessError = handleBrightness(doc["brightness"].as<int>());
    if(brightnessError == 1) {
      server.send(400, "text/plain", "Error setting brightness, valid brightness settings are 20, 40, 60, 100");
    }
    handleScrolling(doc["scroll"].as<boolean>());
    handleCursor(doc["cursor"].as<boolean>());
    server.send(200, "text/plain", "success");
  }
  else {
    server.send(400, "text/plain", "POST the config /config to set it. It must be in JSON format. eg. {\"brightness\":20,\"scroll\":true,\"cursor\":false}");
  }
  
}

void handleMessage() {

  /*
   *  example msg:
   *  {
   *    "message": "a string here" //max 40chr
   *  }
   */

  if(server.method() == HTTP_POST) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    
    if (error) {
      server.send(400, "text/plain", "deserializeJson() failed: " + (String)error.c_str());
      Serial.println(error.c_str());
      return;
    }
    
    //do 2x line feed and set cursor to first char (there isn't a clear function that doesn't reset the display)
    Serial.write(0x0A); 
    Serial.write(0x0A); 
    Serial.write(0x10); 
    Serial.write(0x00); 

    Serial.print(doc["message"].as<String>());
    server.send(200, "text/plain", "success");

  }
  else {
    server.send(400, "text/plain", "POST some text to /message to display it. The display is 2x20 chars. It must be in JSON format. eg. {\"message\":\"goes here\"}");
  }
  
}

void handleNotFound() {
  
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void) {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  server.on("/", handleRoot);
  server.on("/message", handleMessage);
  server.on("/config", handleConfig);

  server.onNotFound(handleNotFound);

  server.begin();
  
  Serial.println("Running at:");
  Serial.print(WiFi.localIP());

}

void loop(void) {
  server.handleClient();
}
