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

bool inControllerMode = false;

void setupController() {
  DmxSimple.usePin(3);
  DmxSimple.maxChannel(128);
  Ethernet.begin(mac, ip);
  server.begin();
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

  // Handle /update?ch=X&val=Y
  if (strstr(getStart, "/update?")) {
    char *query = strchr(getStart, '?');
    if (query) {
      query++;
      int ch = parseParam(query, "ch");
      int val = parseParam(query, "val");
      if (ch >= 1 && ch <= universeSize && val != -1) {
        if (ch <= 20){
          channelValues[ch - 1] = val;
          DmxSimple.write(dmxChannels[ch - 1], val);
        } else {
          DmxSimple.write(ch, val);
        }
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
    client.print(F("<input type='range' class='slider' min='0' max='255' value='"));
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
    client.print(F("<input type='range' class='slider' min='0' max='255' value='"));
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
  client.println(F("<button style='background-color:#FF0000' onclick='setPreset(1)'></button>"));
  client.println(F("<button style='background-color:#FF8C00' onclick='setPreset(2)'></button>"));
  client.println(F("<button style='background-color:#FFFF00' onclick='setPreset(3)'></button>"));
  client.println(F("<button style='background-color:#00FF00' onclick='setPreset(4)'></button>"));
  client.println(F("<button style='background-color:#00FFFF' onclick='setPreset(5)'></button>"));
  client.println(F("<button style='background-color:#0000FF' onclick='setPreset(6)'></button>"));
  client.println(F("<button style='background-color:#FF0080' onclick='setPreset(7)'></button>"));
  client.println(F("<button style='background-color:#8000FF' onclick='setPreset(8)'></button>"));
  client.println(F("<button style='background-color:#FFFFFF' onclick='setPreset(9)'></button>"));
  client.println(F("</div>")); // end button grid

  // JavaScript
  client.println(F("<script>"));
  
  client.println(F("function sendUpdate(channel, value) {"));
  client.println(F("  var xhr = new XMLHttpRequest();"));
  client.println(F("  xhr.open('GET', '/update?ch=' + channel + '&val=' + value, true);"));
  client.println(F("  xhr.send();"));
  client.println(F("}"));

  client.println(F("function setPreset(presetNum) {"));
  client.println(F("  const presets = {"));
  client.println(F("    1: [255, 0, 0, 0],     // Red"));
  client.println(F("    2: [255, 140, 0, 0],     // Orange"));
  client.println(F("    3: [255, 255, 0, 0],     // Yellow"));
  client.println(F("    4: [0, 255, 0, 0],   // Green"));
  client.println(F("    5: [0, 255, 255, 0],   // Light Blueish Cyan"));
  client.println(F("    6: [0, 0, 255, 0],    // Dark Blue"));
  client.println(F("    7: [255, 0, 128, 0],    // Pink"));
  client.println(F("    8: [128, 0, 255, 0],    // Purple"));
  client.println(F("    9: [255, 255, 255, 255]    // White"));
  client.println(F("  };"));

  client.println(F("  const rgbw = presets[presetNum];"));
  client.println(F("  if (!rgbw) return;"));

  client.println(F("  // Determine selected groups"));
  client.println(F("  const selectedGroups = [];"));
  client.println(F("  for (let g = 1; g <= 4; g++) {"));
  client.println(F("    let cb = document.getElementById('groupCheckbox' + g);"));
  client.println(F("    if (cb && cb.checked) selectedGroups.push(g);"));
  client.println(F("  }"));

  client.println(F("  // For each selected group, set its 5 channels"));
  client.println(F("  selectedGroups.forEach(groupNum => {"));
  client.println(F("    let baseChannel = (groupNum - 1) * 5 + 2;"));
  client.println(F("    for (let i = 0; i < 4; i++) {")); // RGBW on last 4 channels of the group
  client.println(F("      let ch = baseChannel + i;"));
  client.println(F("      sendUpdate(ch, rgbw[i]);"));
  client.println(F("      const slider = document.querySelector(`input[type=range][oninput*='${ch}']`);"));
  client.println(F("      if (slider) {"));
  client.println(F("        slider.oninput = null;"));
  client.println(F("        slider.value = rgbw[i];"));
  client.println(F("        setTimeout(() => {"));
  client.println(F("          slider.oninput = () => sendUpdate(ch, slider.value);"));
  client.println(F("        }, 300);"));
  client.println(F("      }"));
  client.println(F("    }"));
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
