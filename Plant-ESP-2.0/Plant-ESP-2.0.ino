#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

const char* host = "esp32";
const char* ssid = "Yeet-2.4G";
const char* password = "widebird712";
const String OtaUser = "admin";
const String OtaPass = "esp32";

WebServer server(80);

//BEGIN ADDED VARS
const int output1 = 2;//Blue onboard LED

const int sensorRelay = 6;//20
const int pumpRelay = 7;//21

const int sensor1 = 36;//3 "touch" enabled pins may not work, so skip them for now
const int sensor2 = 39;//4
const int sensor3 = 34;//5
const int sensor4 = 35;//6
const int sensor5 = 25;//9
const int sensor6 = 26;//10

/*Login page*/
const String loginIndex =
  "<body bgcolor='111111'>"
  "<form name='loginForm'>"
  "<table width='20%' bgcolor='A09F9F' align='center'>"
  "<tr>"
  "<td colspan=2>"
  "<center><font size=4><b>ESP32 OTA Update Login Page</b></font></center>"
  "<br>"
  "</td>"
  "<br>"
  "<br>"
  "</tr>"
  "<td>Username:</td>"
  "<td><input type='text' size=25 name='userid'><br></td>"
  "</tr>"
  "<br>"
  "<br>"
  "<tr>"
  "<td>Password:</td>"
  "<td><input type='Password' size=25 name='pwd'><br></td>"
  "<br>"
  "<br>"
  "</tr>"
  "<tr>"
  "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
  "</tr>"
  "</table>"
  "</form>"
  "</body>"
  
  "<script>"
  "function check(form)"
  "{"
  "if(form.userid.value=='"+OtaUser+"' && form.pwd.value=='"+OtaPass+"')"
  "{"
  "window.open('/serverIndex')"
  "}"
  "else"
  "{"
  " alert('Error Password or Username')/*displays error message*/"
  "}"
  "}"
  "</script>";


/*Server Index Page*/
const char* serverIndex =
  "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
  "<body bgcolor='A09F9F'>"
  "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
  "<input type='file' name='update'>"
  "<input type='submit' value='Update'>"
  "</form>"
  "<div id='prg'>progress: 0%</div>"
  "</body>"
  "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')"
  "},"
  "error: function (a, b, c) {"
  "}"
  "});"
  "});"
  "</script>";

/*
   PlantInfo Index Page (home)
*/

const char* plantInfo = //not being used rn
  "<table width='20%' bgcolor='A09F9F' align='center'>"
  "<tr>"
  "<td colspan=2>"
  "<center><font size=4><b>Soil Readings</b></font></center>"
  "<br>"
  "</table>";

const char* readingsPage = "<h1>test</h1>";

/*
   setup function
*/
void setup(void) {
  Serial.begin(115200);

  // Connect to WiFi network
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

  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  //return index page which is stored in serverIndex
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });
  //index page
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  //not being used rn, probably trash this..
  server.on("/plantinfo", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", plantInfo);
  });

  //handling uploading firmware file
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });

  server.begin();
  //END OTA
  
  //BEGIN CUSTOM SETUP
pinMode(output1, OUTPUT);
pinMode(sensor1, INPUT);

//serve the readings
  server.on("/readings", HTTP_GET, []() {
    digitalWrite(output1, HIGH);
    
    int reading1 = analogRead(sensor1);
    int reading2 = analogRead(sensor2);
    int reading3 = analogRead(sensor3);
    int reading4 = analogRead(sensor4);
    int reading5 = analogRead(sensor5);
    int reading6 = analogRead(sensor6);  
    
    server.sendHeader("Connection", "close");
    //server.send(200, "text/html", readingsPage);
    server.send(200, "text/html", "Sensor1: "+String(reading1)+"<br> Sensor2:"+String(reading2)
    +"<br> Sensor3:"+String(reading3)
    +"<br> Sensor4:"+String(reading4)
    +"<br> Sensor5:"+String(reading5)
    +"<br> Sensor6:"+String(reading6)+
    "<script>"
    "location.reload();"
    "</script>");

      digitalWrite(output1, LOW);
  });
  
}


void loop(void) {
  server.handleClient();
  delay(1);

  //BEGIN CUSTOM LOOP CODE
  digitalWrite(output1, HIGH);
  sleep(5);
  digitalWrite(output1, LOW);
  sleep(2000);
}

//Allows the program to pause without interupting the OTAWebUpdater
void sleep(int duration) {
  for (int i = 0; i <= duration; i++) {
    server.handleClient();
    delay(1);
  }
}
