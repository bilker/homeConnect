#include "DHTesp.h" //DHTESP LIBRARY
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
LiquidCrystal_I2C lcd (0x27,20,4);  // Set the LCD I2C address
DHTesp dht;

// Set your Static IP address
IPAddress local_IP(192, 168, 1, 107);

// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

//constants
const char* ssid = "  "; //Enter your WiFi SSID
const char* password = "  "; //Enter your WiFi Password
const long utcOffsetInSeconds = 10800;
char daysOfTheWeek[7][12] = {"Pazar", "Pazartesi", "Sali", "Carsamba", "Perşembe", "Cuma", "Cumartesi"};

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

//wifiserver
WiFiServer server(80);

//creating custom LCD char for combi waiting
byte customChar[8] = {0x04,0x04,0x04,0x04,0x04,0x1F,0x0E,0x04};
byte customChar2[] = {
  0x04,
  0x0E,
  0x1F,
  0x04,
  0x04,
  0x04,
  0x04,
  0x04
};


void setup() {

  // Configures static IP address
    if (!WiFi.config(local_IP, gateway, subnet)) {
      Serial.println("STA Failed to configure");
    }
       
    Serial.begin(115200);
    dht.setup(D0, DHTesp::DHT11);
    lcd.init();
    lcd.backlight();
    WiFi.mode(WIFI_STA);
    lcd.createChar(0, customChar); //creating custom lcd char down arrow 
    lcd.createChar(1, customChar2); //creating custom lcd char down arrow 
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Set the hostname
  WiFi.hostname("web-blink");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(10);
    Serial.print(".");
   // WiFi.hostname("web-blink"); //yeni 
  //WiFi.begin(ssid, password);  //yeni
  }

  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  //time client start
    timeClient.begin();

   
}
 
void loop() {

  
    //check if wifi is connected or not
  while (WiFi.status() != WL_CONNECTED) {
    delay(10);
    Serial.print(".");
    WiFi.hostname("web-blink"); //yeni 
    WiFi.begin(ssid, password);  //yeni
  }
  
  if (WiFi.status() == WL_CONNECTED){ 
    Serial.println("");
    Serial.println("WiFi connected");
     // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  }

  //update time 
  timeClient.update();
   Serial.print(daysOfTheWeek[timeClient.getDay()]);
  Serial.print(", ");
  Serial.print(timeClient.getHours());
  int hours = timeClient.getHours();
  Serial.print(":");
  Serial.print(timeClient.getMinutes());
  int mins = timeClient.getMinutes();
  Serial.print(":");
  Serial.println(timeClient.getSeconds());
  //Serial.println(timeClient.getFormattedTime());
  delay(10);

  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    //return;
  }

  // Wait until the client sends some data
  Serial.println("new client");

  while(client.connected() && !client.available())
  {
    delay(1);
  }

  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  
    int durum = 1;
    int kombiDurum = 0; //0 kapalı, 1 kapalı, 2 çalışıyor
    float h = dht.getHumidity();
    float t = dht.getTemperature();
    float setTemp = 23;
    float hysterisis = 2; 
    float upperHys = setTemp + hysterisis; //üst sınır buraya kadar ısıtacak, setTemp in altında tekrar devreye girecek
 
    Serial.print("{\"humidity\": ");
    Serial.print(h);
    Serial.print(", \"temp\": ");
    Serial.print(t);
    Serial.print("}\n");
    
    lcd.setCursor (0,0);  // go home
    if (h>60 || t<22){
      lcd.print(" :( "); 
      durum = 0;
    } else {
      lcd.print(" :) ");
      durum = 1;
    }

    //WIFI STATUS ON SCREEN
    if (WiFi.status() == WL_CONNECTED){ 
      lcd.print("            (())");
    } else {
      lcd.print("              x ");
    }

    //NEM MEASUREMENT LCD DISPLAY
    lcd.setCursor ( 0, 1 );        // go to the next line
    lcd.print ("NEM:"); 
    lcd.print (h);

    //TEMP MEASUREMENT LCD DISPLAY
    lcd.setCursor (0,2);
    lcd.print ("SICAKLIK:");
    lcd.print (t); 
    lcd.print (" ");
    //check setTemp vs t
    if(t >= setTemp && t <= upperHys && kombiDurum == 2){
          digitalWrite(D2, HIGH);
          lcd.write(1); // kombinin çalıştığını lcd ekrana göster
      } else if (t >= setTemp && t <= upperHys && kombiDurum == 1){
          digitalWrite(D2,LOW);
          lcd.write(0); //writing the special custom character for kombi kapalı
         } else if (t < setTemp){
            digitalWrite(D2,HIGH);
            lcd.write(1); // kombinin çalıştığını lcd ekrana göster
            kombiDurum = 2;
          } else {
              digitalWrite(D2,LOW);
              lcd.write(0); //writing the special custom character for kombi kapalı
              kombiDurum = 1;
            }
    

    //WEB ADDRESS LCD DISPLAY
    lcd.setCursor (0,3);
    lcd.print ("ADRES:");
    lcd.print (WiFi.localIP());

 
    delay(2000);

    // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); // do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<head><!--<meta http-equiv=\"refresh\" content=\"60\">--><link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\"><meta charset=\"utf-8\"><style>a {text-decoration:none;} </style></head>");
    
    //CLOCK
  client.println("<div class=\"panel panel-default\"><div class=\"panel-body\"><p>");
client.print("<span class=\"glyphicon glyphicon-time\" aria-hidden=\"true\"></span>");
  client.print(" ");
client.print(timeClient.getHours());
  client.print(":");
  client.print(timeClient.getMinutes());
  client.print(":");
  client.println(timeClient.getSeconds());
  client.println(" ");
  client.println(daysOfTheWeek[timeClient.getDay()]);
  delay(10);
     client.println("<br>");

//using weatherwidget. Use your own location with weatherwidget here
  client.print("<a class=\"weatherwidget-io\" href=\"https://forecast7.com/tr/40d6729d89/havuzlu-bahce/\" data-label_1=\"HAVUZLU BAHÇE\" data-label_2=\"DIŞ SICAKLIK\" data-theme=\"original\" >HAVUZLU BAHÇE DIŞ SICAKLIK</a><script>!function(d,s,id){var js,fjs=d.getElementsByTagName(s)[0];if(!d.getElementById(id)){js=d.createElement(s);js.id=id;js.src='https://weatherwidget.io/js/widget.min.js';fjs.parentNode.insertBefore(js,fjs);}}(document,'script','weatherwidget-io-js');</script>");

  client.print("<br>");
  client.print("<b>OTURMA ODASI </b>");
  if (durum == 0){
      client.print("&#128546;");
    } else {client.print("&#9786;");}
  client.print("<br>");

    //HUMIDITY MEASUREMENT WEB
         client.print("<span class=\"glyphicon glyphicon-tint\" aria-hidden=\"true\"></span>");
    client.print(" NEM: ");
      client.print(h);
      delay(10);
    client.println("<br>");

    //TEMP MEASUREMENT WEB
    client.print("&#127777;");
    client.print(" SICAKLIK: ");
    client.print(t); 
    client.print("&#8451;");
    delay(10);
    client.println("<br>");

    //COMBI STATUS WEB
    client.print("<span class=\"glyphicon glyphicon-dashboard\" aria-hidden=\"true\"></span>");
    client.print(" KOMBİ: ");
    if (kombiDurum == 2){
        client.print(" YANIYOR <span class=\"glyphicon glyphicon-fire\" aria-hidden=\"true\"></span>");
      } else {
          client.print(" BEKLİYOR <span class=\"glyphicon glyphicon-sort-by-attributes-alt\" aria-hidden=\"true\"></span>");
        }

    client.println("</html>");

    delay(1);
    Serial.println("Client disonnected");
    Serial.println("");

        //putting 5 secs delay for every loop for the wifi security
    delay(5000);
    
}
