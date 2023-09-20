#include <ESP8266WiFi.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <FirebaseESP8266.h>

#define FIREBASE_HOST "gps-track-740cb-default-rtdb.firebaseio.com"      
#define FIREBASE_AUTH "Tf4m2o8FqiPUDVE3MS1rBj4aOd7rNTUBrx4ZUnax"  
const String FIREBASE_HOSTSIM = "gps-track-740cb-default-rtdb.firebaseio.com";      
const String FIREBASE_AUTHSIM = "Tf4m2o8FqiPUDVE3MS1rBj4aOd7rNTUBrx4ZUnax";  
        
FirebaseData firebaseData;
FirebaseJson json;
const char* ssid = "gojann"; //Enter SSID
const char* password = "fatkulakbr_"; //Enter Password
static const int RX = D2, TX = D1;
static const int rxpin = 3, txpin = 1;
String latitude_data;
String longitude_data;
String loc();
void init_gsm();
void gprs_connect();
boolean gprs_disconnect();
boolean is_gprs_connected();
void post_to_firebase(String data);
boolean waitResponse(String expected_answer="OK", unsigned int timeout=2000);

const String APN  = "internet";//Check Provider
const String USER = "";
const String PASS = "";

#define USE_SSL true
#define DELAY_MS 500

SoftwareSerial ss(TX, RX);
SoftwareSerial SIM800(rxpin,txpin);
TinyGPSPlus gps;

void setup(void)
{ 
  Serial.begin(9600);
  ss.begin(9600);
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
     delay(500);
     Serial.print("*");
  }
  
  Serial.println("");
  Serial.println("WiFi connection Successful");
  Serial.print("The IP Address of ESP8266 Module is: ");
  Serial.print(WiFi.localIP());// Print the IP address
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}

void loop() 
{
  if (WiFi.status() == WL_CONNECTED){
    smartdelay_gps(1000);
    displaydata();
  }
  else{ 
  SIM800.begin(9600);
  init_gsm();
    String data = loc();
      if(!is_gprs_connected()){
        gprs_connect();
      }
    post_to_firebase(data);
    delay(1000);

  }

}

void init_gsm()
{
  //Testing AT Command
  SIM800.println("AT");
  waitResponse();
  delay(DELAY_MS);
  //Checks if the SIM is ready
  SIM800.println("AT+CPIN?");
  waitResponse("+CPIN: READY");
  delay(DELAY_MS);
  //Turning ON full functionality
  SIM800.println("AT+CFUN=1");
  waitResponse();
  delay(DELAY_MS);
  //Turn ON verbose error codes
  SIM800.println("AT+CMEE=2");
  waitResponse();
  delay(DELAY_MS);
  //Enable battery checks
  SIM800.println("AT+CBATCHK=1");
  waitResponse();
  delay(DELAY_MS);
  //Register Network (+CREG: 0,1 or +CREG: 0,5 for valid network)
  //+CREG: 0,1 or +CREG: 0,5 for valid network connection
  SIM800.println("AT+CREG?");
  waitResponse("+CREG: 0,");
  delay(DELAY_MS);
  //setting SMS text mode
  SIM800.print("AT+CMGF=1\r");
  waitResponse("OK");
  delay(DELAY_MS);
 }

boolean waitResponse(String expected_answer, unsigned int timeout) //uncomment if syntax error (esp8266)
{
  uint8_t x=0, answer=0;
  String response;
  unsigned long previous;
    
  //Clean the input buffer
  while( SIM800.available() > 0) SIM800.read();
  
  previous = millis();
  do{
    //if data in UART INPUT BUFFER, reads it
    if(SIM800.available() != 0){
        char c = SIM800.read();
        response.concat(c);
        x++;
        //checks if the (response == expected_answer)
        if(response.indexOf(expected_answer) > 0){
            answer = 1;
        }
    }
  }while((answer == 0) && ((millis() - previous) < timeout));
  
  Serial.println(response);
  return answer;
}

void gprs_connect()
{
  //DISABLE GPRS
  SIM800.println("AT+SAPBR=0,1");
  waitResponse("OK",6000);
  delay(DELAY_MS);
  //Connecting to GPRS: GPRS - bearer profile 1
  SIM800.println("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
  waitResponse();
  delay(DELAY_MS);
  //sets the APN settings for your sim card network provider.
  SIM800.println("AT+SAPBR=3,1,\"APN\","+APN);
  waitResponse();
  delay(DELAY_MS);
  //sets the user name settings for your sim card network provider.
  if(USER != ""){
    SIM800.println("AT+SAPBR=3,1,\"USER\","+USER);
    waitResponse();
    delay(DELAY_MS);
  }
  //sets the password settings for your sim card network provider.
  if(PASS != ""){
    SIM800.println("AT+SAPBR=3,1,\"PASS\","+PASS);
    waitResponse();
    delay(DELAY_MS);
  }
  //after executing the following command. the LED light of 
  //sim800l blinks very fast (twice a second) 
  //enable the GPRS: enable bearer 1
  SIM800.println("AT+SAPBR=1,1");
  waitResponse("OK", 3000);
  delay(DELAY_MS);
  //Get IP Address - Query the GPRS bearer context status
  SIM800.println("AT+SAPBR=2,1");
  waitResponse("OK");
  delay(DELAY_MS);
}

boolean gprs_disconnect()
{
  //Disconnect GPRS
  SIM800.println("AT+CGATT=0");
  waitResponse("OK",6000);
  //delay(DELAY_MS);
  //DISABLE GPRS
  //SIM800.println("AT+SAPBR=0,1");
  //waitResponse("OK",60000);
  //delay(DELAY_MS);

  return true;
}


boolean is_gprs_connected()
{
  SIM800.println("AT+CGATT?");
  if(waitResponse("+CGATT: 1",6000) == 1) { return false; }

  return true;
}

static void smartdelay_gps(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

void print_ok()
{
    Serial.println("------------------------------------");
    Serial.println("OK");
    Serial.println("PATH: " + firebaseData.dataPath());
    Serial.println("TYPE: " + firebaseData.dataType());
    Serial.println("ETag: " + firebaseData.ETag());
    Serial.println("------------------------------------");
    Serial.println();
}

void print_fail()
{
    Serial.println("------------------------------------");
    Serial.println("FAILED");
    Serial.println("REASON: " + firebaseData.errorReason());
    Serial.println("------------------------------------");
    Serial.println();
}

void displaydata()
{
  if (gps.location.isValid())
  {
    float latitude = (gps.location.lat());
    float longitude = (gps.location.lng());
    if(Firebase.setFloat(firebaseData, "/GPS/f_latitude", latitude))
      {print_ok();
        if(Firebase.setFloat(firebaseData, "/GPS/f_longitude", longitude))
        {print_ok();}
        }
    else
      {print_fail();}
  }

  else
  {
    Serial.println(F("Data error!!!"));
  }
  delay(5000);
}

String loc(){
  String a = String(gps.location.lat(), 6);
  String o = String(gps.location.lng(), 6);

  String Data = "{";
  Data += "\"Latitude\";\"" + a + "\",";
  Data += "\"Longitude\";\"" + o + "\",";
  Data += "}";
  return Data;
}


void post_to_firebase(String data)
{  
  //Start HTTP connection
  SIM800.println("AT+HTTPINIT");
  waitResponse();
  delay(DELAY_MS);
  //Enabling SSL 1.0
  if(USE_SSL == true){
    SIM800.println("AT+HTTPSSL=1");
    waitResponse();
    delay(DELAY_MS);
  }
  //Setting up parameters for HTTP session
  SIM800.println("AT+HTTPPARA=\"CID\",1");
  waitResponse();
  delay(DELAY_MS);
  //Set the HTTP URL - Firebase URL and FIREBASE SECRET
  SIM800.println("AT+HTTPPARA=\"URL\","+FIREBASE_HOSTSIM+".json?auth="+FIREBASE_AUTHSIM);
  waitResponse();
  delay(DELAY_MS);
  //Setting up re direct
  SIM800.println("AT+HTTPPARA=\"REDIR\",1");
  waitResponse();
  delay(DELAY_MS);
  //Setting up content type
  SIM800.println("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
  waitResponse();
  delay(DELAY_MS);
  //Setting up Data Size
  //+HTTPACTION: 1,601,0 - error occurs if data length is not correct
  SIM800.println("AT+HTTPDATA=" + String(data.length()) + ",10000");
  waitResponse("DOWNLOAD");
  //delay(DELAY_MS);
  //Sending Data
  SIM800.println(data);
  waitResponse();
  delay(DELAY_MS);
  //Sending HTTP POST request
  SIM800.println("AT+HTTPACTION=1");
  
  for (uint32_t start = millis(); millis() - start < 20000;){
    while(!SIM800.available());
    String response = SIM800.readString();
    if(response.indexOf("+HTTPACTION:") > 0)
    {
      Serial.println(response);
      break;
    }
  }
    
  delay(DELAY_MS);
  //+HTTPACTION: 1,603,0 (POST to Firebase failed)
  //+HTTPACTION: 0,200,0 (POST to Firebase successfull)
  //Read the response
  SIM800.println("AT+HTTPREAD");
  waitResponse("OK");
  delay(DELAY_MS);
  //Stop HTTP connection
  SIM800.println("AT+HTTPTERM");
  waitResponse("OK",1000);
  delay(DELAY_MS);
}
