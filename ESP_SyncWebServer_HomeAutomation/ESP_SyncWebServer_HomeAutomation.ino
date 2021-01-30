#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <EEPROM.h>

#define LOW_LEVEL_TRIGGERED_RELAYS true

const char* ssid     = "******";  //Your wifi ssid
const char* password = "*******";  // Your wifi password

WebServer server(80);

//Set up your relay pins here 
struct MY_RELAY_PINS
{
  String relayPinName;
  int relayPinNumber;
  int relayStatus;  
  String buttonDescription;
};
std::vector<MY_RELAY_PINS> myRelayPins = 
{
  {"Relay1", 4, 0, "Bulb"},
  {"Relay2", 16, 0, "Lamp"},
  {"Relay3", 17, 0, "Christmas Light"},
  {"Relay4", 18, 0, "LED"},    
};

const char* myInitialPage = R"HTMLHOMEPAGE(
<html>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <head>
    <style>
      body 
      {
        max-width: max-content;
        margin: auto;
        background-color: powderblue;
      }
      th
      {
        width:300px;font-size:30;height:70px;font-family:arial;
      }
      input[type=button]
      {
        background-color:red;color:white;border-radius:50%;width:100px;height:40px;font-size:20;
      }
    </style>
    <h2 style="color: teal;text-align:center;">Welcome To Home Automation</h2>
    <h3  style="color: teal;text-align:center;">Using Local Sync Webserver</h3>
  </head>

  <body>

    %ALL_BUTTONS_PLACEHOLDER%

    <script>
      function setRelayPinStatus(button) 
      {
        var url;
        if (button.value == "OFF")
        {
          button.value="ON";
          button.style.backgroundColor = "green";
          url = "/" + button.id + "?value=1" ;
        }
        else
        {
          button.value="OFF";
          button.style.backgroundColor = "red";
          url = "/" + button.id + "?value=0" ;
        }
        var xhttp = new XMLHttpRequest();  
        sendCommand(url, xhttp);    
      }

      function getRelayPinStatus() 
      {
        var inputs = document.getElementsByTagName('input');
        for(var i = 0; i < inputs.length; i++) 
        {
          var button = inputs[i];
          var url = "/status/" + button.id ;
          var xhttp = new XMLHttpRequest();            
          sendCommand(url, xhttp);
          if (xhttp.readyState == 4 && xhttp.status == 200) 
          {
            button.value = xhttp.responseText;
            if (button.value == "ON")
            {
              button.style.backgroundColor = "green";
            }
            else
            {
              button.style.backgroundColor = "red";
            }
          }          
        }
      }
      function sendCommand(url, xhttp)
      {
        xhttp.open("GET", url, false);  //Here false means send request synchronously
        // via Cache-Control header:
        xhttp.setRequestHeader("Cache-Control", "no-cache, no-store, max-age=0");
        xhttp.send();   
      }
    
      window.onload = getRelayPinStatus;
    </script>
  </body>
</html>       
)HTMLHOMEPAGE";


//This function replaces %ALL_BUTTONS_PLACEHOLDER% and creates buttons dynamically from relay pins declared above
String replaceButtonsPlaceholderAndCreateMainHTML()
{
  String htmlMainHomePage = myInitialPage;
  String htmlHomePageButtons;
  htmlHomePageButtons  += "<table>";
  
  for (int i = 0; i < myRelayPins.size(); i++)
  {
    htmlHomePageButtons  += "<tr>";
    htmlHomePageButtons  += "<th>" + myRelayPins[i].buttonDescription + "</th>  <td><input type='button' id='" + myRelayPins[i].relayPinName  + "' value='OFF' onclick='setRelayPinStatus(this)'></td>";
    htmlHomePageButtons  += "</tr>";
  }
  
  htmlHomePageButtons  += "</table>";
  htmlMainHomePage.replace("%ALL_BUTTONS_PLACEHOLDER%", htmlHomePageButtons);
  return htmlMainHomePage;
}

void handleRoot() 
{
  server.send(200, "text/html", replaceButtonsPlaceholderAndCreateMainHTML());
}

void handleNotFound() 
{
  server.send(404, "text/plain", "File Not Found");
}

void setUpPinModes()
{
  EEPROM.begin(myRelayPins.size());
  for (int i = 0; i < myRelayPins.size(); i++)
  {
    myRelayPins[i].relayStatus = EEPROM.read(i) == 1 ? 1 : 0;
    pinMode(myRelayPins[i].relayPinNumber, OUTPUT); 
    digitalWrite(myRelayPins[i].relayPinNumber, LOW_LEVEL_TRIGGERED_RELAYS ? !(myRelayPins[i].relayStatus) :  myRelayPins[i].relayStatus);    
  }
}

void setup(void) 
{
  setUpPinModes();
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  for (int i = 0; i < myRelayPins.size(); i++)
  {
    //Create callbacks for each pin change
    //Example: We will create url string as /Relay1 
    String pinStringName = "/" + myRelayPins[i].relayPinName;
    server.on(pinStringName, [i]() 
    {
      myRelayPins[i].relayStatus = server.arg("value").toInt();
      EEPROM.write(i, byte(myRelayPins[i].relayStatus));
      EEPROM.commit();
      digitalWrite(myRelayPins[i].relayPinNumber, LOW_LEVEL_TRIGGERED_RELAYS ? !(myRelayPins[i].relayStatus) :  myRelayPins[i].relayStatus);   
      server.send(200, "text/plain", "ok");
    });   

    //This is to get the status of relays . Example: We will create url string as /status/Relay1 
    pinStringName = "/status/" + myRelayPins[i].relayPinName;
    server.on(pinStringName, [i]() 
    {
      String relayStatus  = myRelayPins[i].relayStatus ? "ON" : "OFF" ; 
      server.send(200, "text/plain", relayStatus);
    });      
  }
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) 
{
  server.handleClient();
}
