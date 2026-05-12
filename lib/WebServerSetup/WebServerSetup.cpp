#include "WebServerSetup.h"

WebServerSetup webServer;

WebServerSetup::WebServerSetup() : server(80) {}

void WebServerSetup::begin() {
  setupRoutes();
  server.begin();
}

void WebServerSetup::handleClient() {
  server.handleClient();
}

void WebServerSetup::setupRoutes() {
  server.on("/", HTTP_GET, [this](){
    String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>BedBox Config</title></head><body>";
    html += "<h1>BedBox Display Config</h1>";
    
    if (getScrollText) {
      html += "<p>Scrolling text: " + getScrollText() + "</p>";
    }
    
    if (getAlarmStatus) {
      html += "<p>Alarm: " + getAlarmStatus() + "</p>";
    }
    
    html += "<form method=\"POST\" action=\"/set\">";
    html += "Message: <input name=\"message\" type=\"text\" size=\"22\"><br><br>";
    html += "<input type=\"submit\" value=\"Save message\">";
    html += "</form></body></html>";
    
    server.send(200, "text/html", html);
  });
  
  server.on("/set", HTTP_POST, [this](){
    if (server.hasArg("message")) {
      String msg = server.arg("message");
      if (msg.length() == 0) msg = " ";
      
      if (onMessageUpdate) {
        onMessageUpdate(msg);
      }
    }
    
    server.sendHeader("Location", "/", true);
    server.send(303, "text/plain", "");
  });
  
  server.onNotFound([this](){ 
    server.send(404, "text/plain", "Not Found"); 
  });
}
