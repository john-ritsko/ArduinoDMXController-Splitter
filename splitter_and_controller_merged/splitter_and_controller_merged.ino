#include <DmxSimple.h>
#include <SPI.h>
#include <Ethernet.h>

// MAC and IP for Ethernet Shield
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 2);

EthernetServer server(80);

// DMX channel numbers and values
const int numChannels = 20;
const int dmxChannels[numChannels] = {
  1, 2, 3, 4, 5, 
  6, 7, 8, 9, 10,
  11,12,13,14,15,
  16,17,18,19,20
};
uint8_t channelValues[numChannels] = {0};

const int universeSize = 128;

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

bool getParam(const char *request, const char *key, char *valBuf, int valBufSize) {
  const char *pos = strstr(request, key);
  if (!pos) return false;
  pos += strlen(key);
  if (*pos != '=') return false;
  pos++; // skip '='

  int i = 0;
  while (*pos && *pos != '&' && *pos != ' ' && i < valBufSize - 1) {
    valBuf[i++] = *pos++;
  }
  valBuf[i] = '\0';
  return i > 0;
}

void handleButton(const char *name, int cb1, int cb2, int cb3, int cb4) {
  int presetVals[4];
  int groupsSelected[] = {cb1, cb2, cb3, cb4};
  if (strcmp(name, "btn1") == 0){ //red
    presetVals[0] = 255;
    presetVals[1] = 0;
    presetVals[2] = 0;
    presetVals[3] = 0;
  } else if (strcmp(name, "btn2") == 0) { //orange
    presetVals[0] = 255;
    presetVals[1] = 140;
    presetVals[2] = 0;
    presetVals[3] = 0;
  } else if (strcmp(name, "btn3") == 0) { //yellow
    presetVals[0] = 255;
    presetVals[1] = 255;
    presetVals[2] = 0;
    presetVals[3] = 0;
  } else if (strcmp(name, "btn4") == 0) { //green
    presetVals[0] = 0;
    presetVals[1] = 255;
    presetVals[2] = 0;
    presetVals[3] = 0;
  } else if (strcmp(name, "btn5") == 0) { //light bluish cyan
    presetVals[0] = 0;
    presetVals[1] = 255;
    presetVals[2] = 255;
    presetVals[3] = 0;
  } else if (strcmp(name, "btn6") == 0) { //dark blue
    presetVals[0] = 0;
    presetVals[1] = 0;
    presetVals[2] = 255;
    presetVals[3] = 0;
  } else if (strcmp(name, "btn7") == 0) { //pink
    presetVals[0] = 255;
    presetVals[1] = 0;
    presetVals[2] = 128;
    presetVals[3] = 0;
  } else if (strcmp(name, "btn8") == 0) { //purple
    presetVals[0] = 128;
    presetVals[1] = 0;
    presetVals[2] = 255;
    presetVals[3] = 0;
  } else if (strcmp(name, "btn9") == 0) { //white
    presetVals[0] = 255;
    presetVals[1] = 255;
    presetVals[2] = 255;
    presetVals[3] = 255;
  }

  // send preset to selected groups
  for (int i = 0; i < 4; i++){
    if (groupsSelected[i] == 1) {
      int baseChannel = i * 5 + 2;
      channelValues[baseChannel-1] = presetVals[0];
      channelValues[baseChannel] = presetVals[1];
      channelValues[baseChannel+1] = presetVals[2];
      channelValues[baseChannel+2] = presetVals[3];

      DmxSimple.write(baseChannel, presetVals[0]);
      DmxSimple.write(baseChannel+1, presetVals[1]);
      DmxSimple.write(baseChannel+2, presetVals[2]);
      DmxSimple.write(baseChannel+3, presetVals[3]);
    }
  }
}

void handleButton(int r, int g, int b, int w, int cb1, int cb2, int cb3, int cb4) {
  int presetVals[] = {r, g, b, w};
  Serial.println(presetVals[0]);
  int groupsSelected[] = {cb1, cb2, cb3, cb4};

  // send preset to selected groups
  for (int i = 0; i < 4; i++){
    if (groupsSelected[i] == 1) {
      int baseChannel = i * 5 + 2;
      channelValues[baseChannel-1] = presetVals[0];
      channelValues[baseChannel] = presetVals[1];
      channelValues[baseChannel+1] = presetVals[2];
      channelValues[baseChannel+2] = presetVals[3];

      DmxSimple.write(baseChannel, presetVals[0]);
      DmxSimple.write(baseChannel+1, presetVals[1]);
      DmxSimple.write(baseChannel+2, presetVals[2]);
      DmxSimple.write(baseChannel+3, presetVals[3]);
    }
  }
}

bool inControllerMode = false;

void setupController() {
  DmxSimple.usePin(3);
  DmxSimple.maxChannel(128);
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.begin(9600);
}

void setup() {
  pinMode(2, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(4, OUTPUT);
  setupController();
}

void splitterLoop() {
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(4, LOW);
}

void controllerLoop() {
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(4, HIGH);
    
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

  if (strstr(getStart, "/sliderValues")) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();

  for (int i = 0; i < 20; i++) {
    client.print(channelValues[i]);
    if (i < 19) client.print(",");
  }

  client.println();
  client.stop();
  return;
}

  if (strstr(getStart, "/button?")) {
    char *query = strchr(getStart, '?');
    if (query) {
      query++;

      if (strstr(query, "name=")) {
      char name[32];
      char cb1Str[4], cb2Str[4], cb3Str[4], cb4Str[4];

      if (!getParam(requestBuffer, "name", name, sizeof(name))) {
        // optional: send 400 error if missing
      }
      if (!getParam(requestBuffer, "cb1", cb1Str, sizeof(cb1Str))) cb1Str[0] = '0';
      if (!getParam(requestBuffer, "cb2", cb2Str, sizeof(cb2Str))) cb2Str[0] = '0';
      if (!getParam(requestBuffer, "cb3", cb3Str, sizeof(cb3Str))) cb3Str[0] = '0';
      if (!getParam(requestBuffer, "cb4", cb4Str, sizeof(cb4Str))) cb4Str[0] = '0';

      int cb1 = atoi(cb1Str);
      int cb2 = atoi(cb2Str);
      int cb3 = atoi(cb3Str);
      int cb4 = atoi(cb4Str);

      handleButton(name, cb1, cb2, cb3, cb4);
    }

    }
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("OK");
    client.stop();
    return;
  }

  if (strstr(getStart, "/customColor?")) {
  char *query = strchr(getStart, '?');
  if (query) {
    query++;

    char rStr[4], gStr[4], bStr[4], wStr[4], cb1Str[4], cb2Str[4], cb3Str[4], cb4Str[4];

    if (!getParam(query, "r", rStr, sizeof(rStr))) rStr[0] = '0';
    if (!getParam(query, "g", gStr, sizeof(gStr))) gStr[0] = '0';
    if (!getParam(query, "b", bStr, sizeof(bStr))) bStr[0] = '0';
    if (!getParam(query, "w", wStr, sizeof(wStr))) wStr[0] = '0';

    if (!getParam(requestBuffer, "cb1", cb1Str, sizeof(cb1Str))) cb1Str[0] = '0';
    if (!getParam(requestBuffer, "cb2", cb2Str, sizeof(cb2Str))) cb2Str[0] = '0';
    if (!getParam(requestBuffer, "cb3", cb3Str, sizeof(cb3Str))) cb3Str[0] = '0';
    if (!getParam(requestBuffer, "cb4", cb4Str, sizeof(cb4Str))) cb4Str[0] = '0';

    int cb1 = atoi(cb1Str);
    int cb2 = atoi(cb2Str);
    int cb3 = atoi(cb3Str);
    int cb4 = atoi(cb4Str);

    int r = atoi(rStr);
    int g = atoi(gStr);
    int b = atoi(bStr);
    int w = atoi(wStr);

    handleButton(r, g, b, w, cb1, cb2, cb3, cb4);
  }

    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");
    client.println();
    client.println("OK");
    client.stop();
    return;
  }

  if (strstr(getStart, "/update?")) {
  char *query = strchr(getStart, '?');
  if (query) {
    query++;

    // Detect slider update: look for `ch=`
    if (strstr(query, "ch=")) {
      int ch = parseParam(query, "ch");
      int val = parseParam(query, "val");
      if (ch >= 1 && ch <= universeSize && val != -1) {
        if (ch <= 20) {
          channelValues[ch - 1] = val;
          DmxSimple.write(dmxChannels[ch - 1], val);
        } else {
          DmxSimple.write(ch, val);
        }
      }
    }
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("OK");
  client.stop();
  return;
}



  // Serve UI
  client.println(F("HTTP/1.1 200 OK"));
  client.println(F("Content-Type: text/html"));
  client.println(F("Connection: close"));
  client.println();

  client.println(F("<!DOCTYPE html><html><head><style>"));
  client.println(F("body { background-color: black; color: white; font-family: sans-serif; text-align: left; padding-top: 40px; margin-top: 0px; }"));
  
  client.println(F(".parent-container {"));
  client.println(F("  display: flex;"));
  client.println(F("  justify-content: flex-start;"));
  client.println(F("  gap: 60px;"));
  client.println(F("  padding-left: 20px;"));
  client.println(F("}"));
  
  client.println(F(".slider-section {"));
  client.println(F("  display: flex;"));
  client.println(F("  flex-direction: column;"));
  client.println(F("  gap: 30px;"));
  client.println(F("}"));

  client.println(F(".slider-container {"));
  client.println(F("  display: flex;"));
  client.println(F("  justify-content: flex-start;"));
  client.println(F("  gap: 0px;"));
  client.println(F("  margin: 0;"));
  client.println(F("  flex-wrap: nowrap;"));
  client.println(F("  max-width: 850px;"));
  client.println(F("}"));

  client.println(F(".slider-wrapper {"));
  client.println(F("  display: flex;"));
  client.println(F("  flex-direction: column;"));
  client.println(F("  align-items: center;"));
  client.println(F("  color: white;"));
  client.println(F("  margin-left: -60px;"));
  client.println(F("  margin-right: -60px;"));
  client.println(F("  margin-top: 60px;"));
  client.println(F("  margin-bottom: 60px;"));
  client.println(F("}"));

  client.println(F(".slider {"));
  client.println(F("  -webkit-appearance: none;"));
  client.println(F("  appearance: none;"));
  client.println(F("  width: 200px;"));
  client.println(F("  height: 8px;"));
  client.println(F("  background: #888;"));
  client.println(F("  border-radius: 4px;"));
  client.println(F("  outline: none;"));
  client.println(F("  transform: rotate(270deg);"));
  client.println(F("  margin: 20px 0;"));
  client.println(F("}"));

  client.println(F(".slider::-webkit-slider-thumb {"));
  client.println(F("  -webkit-appearance: none;"));
  client.println(F("  appearance: none;"));
  client.println(F("  width: 16px;"));
  client.println(F("  height: 16px;"));
  client.println(F("  background: white;"));
  client.println(F("  border-radius: 50%;"));
  client.println(F("  cursor: pointer;"));
  client.println(F("}"));

  client.println(F(".channel-label {"));
  client.println(F("  margin-top: 85px;"));
  client.println(F("  font-size: 14px;"));
  client.println(F("}"));

  client.println(F(".button-grid {"));
  client.println(F("  display: grid;"));
  client.println(F("  grid-template-columns: repeat(3, 80px);"));
  client.println(F("  gap: 15px;"));
  client.println(F("  margin-top: 40px;"));
  client.println(F("}"));

  client.println(F(".button-grid button {"));
  client.println(F("  width: 80px;"));
  client.println(F("  height: 40px;"));
  client.println(F("  border: none;"));
  client.println(F("  border-radius: 6px;"));
  client.println(F("  cursor: pointer;"));
  client.println(F("  color: white;"));
  client.println(F("  font-weight: bold;"));
  client.println(F("  font-size: 14px;"));
  client.println(F("}"));

  client.println(F(".group-selection {"));
  client.println(F("  margin-bottom: 30px;"));
  client.println(F("}"));

  client.println(F(".group-selection label {"));
  client.println(F("  margin-right: 30px;"));
  client.println(F("  font-size: 16px;"));
  client.println(F("  cursor: pointer;"));
  client.println(F("}"));

  client.println(F("#manual-control {"));
  client.println(F("  position: fixed;"));
  client.println(F("  bottom: 70px;"));
  client.println(F("  right: 20px;"));
  client.println(F("  background-color: rgba(30, 30, 30, 0.8);"));
  client.println(F("  padding: 10px 15px;"));
  client.println(F("  border-radius: 8px;"));
  client.println(F("  color: white;"));
  client.println(F("  font-size: 14px;"));
  client.println(F("  box-shadow: 0 0 10px rgba(0,0,0,0.7);"));
  client.println(F("  z-index: 1000;"));
  client.println(F("}"));
  client.println(F("#manual-control input {"));
  client.println(F("  width: 60px;"));
  client.println(F("}"));
  client.println(F("#manual-control button {"));
  client.println(F("  margin-left: 10px;"));
  client.println(F("  cursor: pointer;"));
  client.println(F("  font-weight: bold;"));
  client.println(F("  padding: 4px 10px;"));
  client.println(F("  border-radius: 5px;"));
  client.println(F("  border: none;"));
  client.println(F("  background-color: #3498db;"));
  client.println(F("  color: white;"));
  client.println(F("}"));

  client.println(F("#custom-color {"));
  client.println(F("  position: fixed;"));
  client.println(F("  bottom: 20px;"));
  client.println(F("  right: 20px;"));
  client.println(F("  background-color: rgba(30, 30, 30, 0.8);"));
  client.println(F("  padding: 10px 15px;"));
  client.println(F("  border-radius: 8px;"));
  client.println(F("  color: white;"));
  client.println(F("  font-size: 14px;"));
  client.println(F("  box-shadow: 0 0 10px rgba(0,0,0,0.7);"));
  client.println(F("  z-index: 1000;"));
  client.println(F("}"));
  client.println(F("#custom-color input {"));
  client.println(F("  width: 60px;"));
  client.println(F("}"));
  client.println(F("#custom-color button {"));
  client.println(F("  margin-left: 10px;"));
  client.println(F("  cursor: pointer;"));
  client.println(F("  font-weight: bold;"));
  client.println(F("  padding: 4px 10px;"));
  client.println(F("  border-radius: 5px;"));
  client.println(F("  border: none;"));
  client.println(F("  background-color: #3498db;"));
  client.println(F("  color: white;"));
  client.println(F("}"));


  client.println(F("</style></head><body>"));

  client.println(F("<div id='manual-control'>"));
  client.print(F("  <label for='channelInput'>Channel (1-"));
  client.print(universeSize);
  client.println(F("): </label>"));
  client.print(F("  <input type='number' id='channelInput' min='1' max='"));
  client.print(universeSize);
  client.println(F("'>"));
  client.println(F("  <label for='valueInput' style='margin-left: 10px;'>Value (0-255): </label>"));
  client.println(F("  <input type='number' id='valueInput' min='0' max='255'>"));
  client.println(F("  <button onclick='sendManualUpdate()'>Set</button>"));
  client.println(F("</div>"));

  client.println(F("<div id='custom-color'>"));
  client.println(F("  <label for='customRed' style='margin-left: 10px;'>Custom Color  -  Red: </label>"));
  client.println(F("  <input type='number' id='customRed' min='0' max='255'>"));
  client.println(F("  <label for='customGreen' style='margin-left: 10px;'>Green: </label>"));
  client.println(F("  <input type='number' id='customGreen' min='0' max='255'>"));
  client.println(F("  <label for='customBlue' style='margin-left: 10px;'>Blue: </label>"));
  client.println(F("  <input type='number' id='customBlue' min='0' max='255'>"));
  client.println(F("  <label for='customWhite' style='margin-left: 10px;'>White: </label>"));
  client.println(F("  <input type='number' id='customWhite' min='0' max='255'>"));
  client.println(F("  <button onclick='sendCustomColor()'>Set</button>"));
  client.println(F("</div>"));


  // Group selection checkboxes
  client.println(F("<div class='group-selection'>"));
  for (int g = 1; g <= 4; g++) {
    client.print(F("<label><input type='checkbox' id='groupCheckbox"));
    client.print(g);
    client.print(F("' checked> Group "));
    client.print(g);
    client.println(F("</label>"));
  }
  client.println(F("</div>"));

  // Sliders section: two rows of 10
  client.println(F("<div class='slider-section'>"));

  // First row of 10 sliders
  client.println(F("<div class='slider-container'>"));
  for (int i = 0; i < 10; i++) {
    client.print(F("<div class='slider-wrapper'>"));
    client.print(F("<input type='range' id='slider"));
    client.print(i+1);
    client.print(F("' class='slider' min='0' max='255' value='"));
    client.print(channelValues[i]);
    client.print(F("' oninput='sendUpdate("));
    client.print(i + 1);
    client.println(F(", this.value)'>"));
    client.print(F("<div class='channel-label'>Channel "));
    client.print(i + 1);
    client.println(F("</div></div>"));
  }
  client.println(F("</div>")); // end first slider row

  // Second row of 10 sliders
  client.println(F("<div class='slider-container'>"));
  for (int i = 10; i < 20; i++) {
    client.print(F("<div class='slider-wrapper'>"));
    client.print(F("<input type='range' id='slider"));
    client.print(i+1);
    client.print(F("' class='slider' min='0' max='255' value='"));
    client.print(channelValues[i]);
    client.print(F("' oninput='sendUpdate("));
    client.print(i + 1);
    client.println(F(", this.value)'>"));
    client.print(F("<div class='channel-label'>Channel "));
    client.print(i + 1);
    client.println(F("</div></div>"));
  }
  client.println(F("</div>")); // end second slider row

  client.println(F("</div>")); // end slider-section

  // Buttons grid
  client.println(F("<div class='button-grid'>"));
  client.println(F("<button style='background-color:#FF0000' onclick='sendButton(\"btn1\")'></button>"));
  client.println(F("<button style='background-color:#FF8C00' onclick='sendButton(\"btn2\")'></button>"));
  client.println(F("<button style='background-color:#FFFF00' onclick='sendButton(\"btn3\")'></button>"));
  client.println(F("<button style='background-color:#00FF00' onclick='sendButton(\"btn4\")'></button>"));
  client.println(F("<button style='background-color:#00FFFF' onclick='sendButton(\"btn5\")'></button>"));
  client.println(F("<button style='background-color:#0000FF' onclick='sendButton(\"btn6\")'></button>"));
  client.println(F("<button style='background-color:#FF0080' onclick='sendButton(\"btn7\")'></button>"));
  client.println(F("<button style='background-color:#8000FF' onclick='sendButton(\"btn8\")'></button>"));
  client.println(F("<button style='background-color:#FFFFFF' onclick='sendButton(\"btn9\")'></button>"));
  client.println(F("</div>")); // end button grid

  // JavaScript
  client.println(F("<script>"));
  
  client.println(F("function sendUpdate(channel, value) {"));
  client.println(F("  var xhr = new XMLHttpRequest();"));
  client.println(F("  xhr.open('GET', '/update?ch=' + channel + '&val=' + value, true);"));
  client.println(F("  xhr.send();"));
  client.println(F("}"));

  client.println(F("function getCheckbox(id) {"));
  client.println(F("  return document.getElementById(id).checked ? 1 : 0;"));
  client.println(F("  }"));

  client.println(F("function sendButton(buttonName) {"));
  client.println(F("  const cb1 = getCheckbox('groupCheckbox1');"));
  client.println(F("  const cb2 = getCheckbox('groupCheckbox2');"));
  client.println(F("  const cb3 = getCheckbox('groupCheckbox3');"));
  client.println(F("  const cb4 = getCheckbox('groupCheckbox4');"));
  client.println(F("  "));
  client.println(F("  const url = `/button?name=${buttonName}&cb1=${cb1}&cb2=${cb2}&cb3=${cb3}&cb4=${cb4}`;"));
  client.println(F("  fetch(url).then(res => {"));
  client.println(F("    console.log(`Sent: ${url}`);"));
  client.println(F("    setTimeout(() => {"));
  client.println(F("      updateSliders();"));
  client.println(F("    }, 100);"));
  client.println(F("  });"));
  client.println(F("}"));


  client.println(F("function sendManualUpdate() {"));
  client.println(F("  const ch = parseInt(document.getElementById('channelInput').value);"));
  client.println(F("  const val = parseInt(document.getElementById('valueInput').value);"));
  client.print(F("  if (isNaN(ch) || ch < 1 || ch > "));
  client.print(universeSize);
  client.print(F(") { alert('Channel must be between 1 and "));
  client.print(universeSize);
  client.println(F("'); return; }"));
  client.println(F("  if (isNaN(val) || val < 0 || val > 255) { alert('Value must be between 0 and 255'); return; }"));
  client.println(F("  sendUpdate(ch, val);"));
  client.println(F("  const slider = document.querySelector(`input[type=range][oninput*='${ch}']`);"));
  client.println(F("  if (slider) slider.value = val;"));
  client.println(F("}"));

  client.println(F("function sendCustomColor() {"));
  client.println(F("  const cb1 = getCheckbox('groupCheckbox1');"));
  client.println(F("  const cb2 = getCheckbox('groupCheckbox2');"));
  client.println(F("  const cb3 = getCheckbox('groupCheckbox3');"));
  client.println(F("  const cb4 = getCheckbox('groupCheckbox4');"));
  client.println(F("  const r = parseInt(document.getElementById('customRed').value);"));
  client.println(F("  const g = parseInt(document.getElementById('customGreen').value);"));
  client.println(F("  const b = parseInt(document.getElementById('customBlue').value);"));
  client.println(F("  const w = parseInt(document.getElementById('customWhite').value);"));
  client.println(F("  "));
  client.println(F("  const url = `/customColor?r=${r}&g=${g}&b=${b}&w=${w}&cb1=${cb1}&cb2=${cb2}&cb3=${cb3}&cb4=${cb4}`;"));
  client.println(F("  fetch(url).then(res => {"));
  client.println(F("    console.log(`Sent: ${url}`);"));
  client.println(F("    setTimeout(() => {"));
  client.println(F("      updateSliders();"));
  client.println(F("    }, 100);"));
  client.println(F("  });"));
  client.println(F("}"));

  client.println(F("async function updateSliders() {"));
  client.println(F("  try {"));
  client.println(F("    const response = await fetch('/sliderValues');"));
  client.println(F("    if (!response.ok) throw new Error('Network response was not OK');"));
  client.println(F("    const text = await response.text();"));
  client.println(F("    const values = text.trim().split(',');"));
  client.println(F(""));
  client.println(F("    for (let i = 0; i < 20 && i < values.length; i++) {"));
  client.println(F("      const slider = document.getElementById(`slider${i+1}`);"));
  client.println(F("      if (slider) {"));
  client.println(F("        slider.value = values[i];"));
  client.println(F("      }"));
  client.println(F("      console.log('I think i updated');"));
  client.println(F("    }"));
  client.println(F("  } catch (error) {"));
  client.println(F("    console.error('Failed to update sliders:', error);"));
  client.println(F("  }"));
  client.println(F("}"));


  client.println(F("document.addEventListener(\"DOMContentLoaded\", () => {"));

  // Manual control listeners
  client.println(F("  [document.getElementById(\"channelInput\"), document.getElementById(\"valueInput\")].forEach(input => {"));
  client.println(F("    input.addEventListener(\"keydown\", (event) => {"));
  client.println(F("      if (event.key === \"Enter\") sendManualUpdate();"));
  client.println(F("    });"));
  client.println(F("  });"));

  // Custom color listeners
  client.println(F("  [\"customRed\", \"customGreen\", \"customBlue\", \"customWhite\"].forEach(id => {"));
  client.println(F("    const el = document.getElementById(id);"));
  client.println(F("    if (el) {"));
  client.println(F("      el.addEventListener(\"keydown\", (event) => {"));
  client.println(F("        if (event.key === \"Enter\") sendCustomColor();"));
  client.println(F("      });"));
  client.println(F("    }"));
  client.println(F("  });"));

  client.println(F("});"));


  client.println(F("</script>"));

  client.println(F("</body></html>"));

  client.stop();
}

void loop() {
  bool currentState = digitalRead(2);

  if (currentState) {
    controllerLoop();
  } else {
    splitterLoop();
  }
}
