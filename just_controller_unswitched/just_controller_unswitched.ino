#include <DmxSimple.h>
#include <SPI.h>
#include <Ethernet.h>

// MAC and IP for Ethernet Shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 2);

EthernetServer server(80);

// DMX channel numbers and values
const int numChannels = 20;
const int dmxChannels[numChannels] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                      11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
uint8_t channelValues[numChannels] = {0};

char requestBuffer[200];

int parseParam(const char *request, const char *key) {
  char *pos = strstr(request, key);
  if (pos) {
    pos += strlen(key);
    if (*pos == '=') {
      pos++;
      int val = 0;
      sscanf(pos, "%d", &val);
      if (val < 0) val = 0;
      else if (val > 255) val = 255;
      return val;
    }
  }
  return -1;
}

void setup() {
  DmxSimple.usePin(3);
  DmxSimple.maxChannel(128);
  Ethernet.begin(mac, ip);
  server.begin();
}

void loop() {
  EthernetClient client = server.available();
  if (!client) return;

  int idx = 0;
  while (client.connected() && idx < sizeof(requestBuffer) - 1) {
    if (client.available()) {
      char c = client.read();
      requestBuffer[idx++] = c;
      if (idx >= 4 &&
          requestBuffer[idx - 4] == '\r' &&
          requestBuffer[idx - 3] == '\n' &&
          requestBuffer[idx - 2] == '\r' &&
          requestBuffer[idx - 1] == '\n') {
        break;
      }
    }
  }
  requestBuffer[idx] = '\0';

  char *getStart = strstr(requestBuffer, "GET ");
  if (!getStart) {
    client.stop();
    return;
  }
  getStart += 4;

  char *getEnd = strstr(getStart, " HTTP/");
  if (!getEnd) {
    client.stop();
    return;
  }
  *getEnd = '\0';

  // Handle favicon.ico to avoid browser requests
  if (strstr(getStart, "/favicon.ico")) {
    client.println("HTTP/1.1 204 No Content");
    client.println("Connection: close");
    client.println();
    client.stop();
    return;
  }

  // Handle /update?ch=X&val=Y
  if (strstr(getStart, "/update?")) {
    char *query = strchr(getStart, '?');
    if (query) {
      query++;
      int ch = parseParam(query, "ch");
      int val = parseParam(query, "val");
      if (ch >= 1 && ch <= numChannels && val != -1) {
        channelValues[ch - 1] = val;
        DmxSimple.write(dmxChannels[ch - 1], val);
      }
    }
    client.println("HTTP/1.1 200 OK");
    client.println("Connection: close");
    client.println();
    client.stop();
    return;
  }

  // Serve UI
  client.println(F("HTTP/1.1 200 OK"));
  client.println(F("Content-Type: text/html"));
  client.println(F("Connection: close"));
  client.println();
  client.println(F("<!DOCTYPE html><html><head><style>"));
  client.println(F("body { background-color: black; color: white; font-family: sans-serif; text-align: center; padding-top: 80px; margin-top: 40px; }"));
  client.println(F(".slider-container { display: flex; justify-content: flex-start; gap: 0px; margin: 20px; }"));
  client.println(F(".slider-wrapper { display: flex; flex-direction: column; align-items: center; color: white; margin-left: -60px; margin-right: -60px; margin-top: 60px; margin-bottom: 60px; }"));
  client.println(F(".slider { -webkit-appearance: none; appearance: none; width: 200px; height: 8px; background: #888; border-radius: 4px; outline: none; transform: rotate(270deg); margin: 20px 0; }"));
  client.println(F(".slider::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 16px; height: 16px; background: white; border-radius: 50%; cursor: pointer; }"));
  client.println(F(".channel-label { margin-top: 85px; font-size: 14px; }"));
  client.println(F("</style></head><body>"));

  for (int row = 0; row < 2; row++) {
    client.println(F("<div class='slider-container'>"));
    for (int i = 0; i < 10; i++) {
      int chIndex = row * 10 + i;
      client.print(F("<div class='slider-wrapper'>"));
      client.print(F("<input type='range' class='slider' min='0' max='255' value='"));
      client.print(channelValues[chIndex]);
      client.print(F("' oninput='sendUpdate("));
      client.print(chIndex + 1);
      client.println(F(", this.value)'>"));
      client.print(F("<div class='channel-label'>Channel "));
      client.print(chIndex + 1);
      client.println(F("</div></div>"));
    }
    client.println(F("</div>")); // Close .slider-container
  }


  client.println(F("<script>"));
  client.println(F("function sendUpdate(channel, value) {"));
  client.println(F("  var xhr = new XMLHttpRequest();"));
  client.println(F("  xhr.open('GET', '/update?ch=' + channel + '&val=' + value, true);"));
  client.println(F("  xhr.send();"));
  client.println(F("}"));
  client.println(F("</script>"));
  client.println(F("</body></html>"));

  delay(1);
  client.stop();
}
