#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
const char* ssid = "a";
const char* password = "00000000";
const int UNO_RX_PIN = 16, UNO_TX_PIN = 17;

WebServer server(80);
WebSocketsServer webSocket(81);

int sensorVal = 0;
String ledState = "off";

String index_html = R"rawliteral(
<!doctype html><html><head><meta charset="utf-8"><title>UNO Bridge</title>
<style>
 body { font-family: sans-serif; text-align: center; padding-top: 20px; }
 button { font-size: 1.1rem; padding: 8px 15px; margin: 5px; cursor:pointer; border-radius: 8px; border: 1px solid #ccc; }
 input { font-size: 1.1rem; padding: 6px; width:200px; margin:5px;}
 span { font-weight: bold; color: darkred; }
</style>
</head><body>
 <h3>Lab D — UNO + ESP Bridge</h3>
 <button id="on">LED ON</button>
 <button id="off">LED OFF</button>
 <p>LED (from UNO): <span id="led">?</span></p>
 <p>Sensor (from UNO): <span id="sensor">?</span></p>
 <hr>
 <h4>Send Text to LCD</h4>
 <input id="msg" placeholder="Type message"/>
 <button id="sendMsg">Send</button>
<script>
let ws;
function startWS(){
 ws = new WebSocket('ws://'+location.hostname+':81/');
 ws.onmessage = (e)=> {
   try{
     const j = JSON.parse(e.data);
     document.getElementById('led').innerText = j.led;
     document.getElementById('sensor').innerText = j.sensor;
   } catch(_){}
 };
 ws.onclose = ()=> { setTimeout(startWS,2000); };
}
startWS();

document.getElementById('on').onclick = ()=>
 ws.send(JSON.stringify({cmd:'led',state:'on'}));
document.getElementById('off').onclick = ()=>
 ws.send(JSON.stringify({cmd:'led',state:'off'}));

document.getElementById('sendMsg').onclick = ()=>{
 let txt = document.getElementById('msg').value;
 if(txt.trim().length>0){
   ws.send(JSON.stringify({cmd:'msg',text:txt}));
 }
};
</script>
</body></html>
)rawliteral";

void sendToUno(const String &cmd) { Serial1.println(cmd); }

void broadcastStatus() {
 String payload = String("{\"led\":\"") + ledState + "\",\"sensor\":" + String(sensorVal) + "}";
 webSocket.broadcastTXT(payload);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
 if (type == WStype_TEXT) {
   String msg = String((char*)payload);
   if (msg.indexOf("\"cmd\":\"led\"") >= 0) {
     if (msg.indexOf("\"state\":\"on\"") >= 0) sendToUno("LED ON");
     else if (msg.indexOf("\"state\":\"off\"") >= 0) sendToUno("LED OFF");
   } else if (msg.indexOf("\"cmd\":\"msg\"") >= 0) {
     int p = msg.indexOf("\"text\":\"");
     if (p>=0){
       String text = msg.substring(p+8);
       text.replace("\"}","");
       sendToUno("MSG:" + text);
     }
   }
 }
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, UNO_RX_PIN, UNO_TX_PIN);

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }

  Serial.print("\nIP: ");
  Serial.println(WiFi.localIP().toString());

  server.on("/", [](){ server.send(200, "text/html", index_html); });
  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Serial.println("Servers running");
  delay(1000);
  sendToUno("GET STATUS");
}

void loop() {
  server.handleClient();
  webSocket.loop();
  while (Serial1.available()) {
    String line = Serial1.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    bool updated = false;
    if (line.indexOf("LED:") >= 0) {
      ledState = (line.indexOf(":ON") > 0) ? "on" : "off";
      updated = true;
    }
    if (line.indexOf("SENSOR:") >= 0) {
      sensorVal = line.substring(line.lastIndexOf(':') + 1).toInt();
      updated = true;
    }
    if (updated) broadcastStatus();
  }
}
