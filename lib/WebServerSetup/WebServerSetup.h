#ifndef WEBSERVER_SETUP_H
#define WEBSERVER_SETUP_H

#include <Arduino.h>
#include <WebServer.h>

class WebServerSetup {
public:
  WebServerSetup();
  void begin();
  void handleClient();
  
  // Callbacks to get/set state from main
  void (*onMessageUpdate)(const String &message) = nullptr;
  String (*getScrollText)() = nullptr;
  String (*getAlarmStatus)() = nullptr;
  
private:
  WebServer server;
  void setupRoutes();
};

extern WebServerSetup webServer;

#endif
