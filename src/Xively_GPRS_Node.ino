/* Water Automation GPRS Node.
 *
 * GPRS Node for the Water Irrigation System.
 * Receives data form the controller node and uploads them to the Xively API.
 */

// ----------------------- Includes and Defines ------------------------------------
// Include the GSM library
#include <GSM.h>
#include <HttpClient.h>
#include <Xively.h>
#include <avr/wdt.h>
#include <Timer.h>
#include <TimerOne.h>


// Xively login information
#define APIKEY         "9fi8Fw6zLVcIq72SMjEGLWqtifg0Nsxi3QaAKiZ7UeK1ppbq"
#define FEEDID         1905310318
#define USERAGENT      "WA_Delicias"

// PIN Number
#define PINNUMBER ""

// APN data
// m2m SIM
//#define GPRS_APN       "m2m.amx"
//#define GPRS_LOGIN     "clozoya"
//#define GPRS_PASSWORD  "RAutomatico"
// Amigo SIM
#define GPRS_APN       "internet.itelcel.com"
#define GPRS_LOGIN     "webgprs"
#define GPRS_PASSWORD  "webgprs2003"
// ---------------------------------------------------------------------------------


// ----------------------- Global Variables ----------------------------------------
// initialize the library instance
GSMClient client;
GPRS gprs;
GSM gsmAccess;

// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
// IPAddress server(216,52,233,121);     // numeric IP for api.xively.com
char server[] = "api.xively.com";       // name address for Xively API
unsigned long lastConnectionTime = 0;           // last time you connected to the server, in milliseconds
boolean lastConnected = false;                  // state of the connection last time through the main loop
const unsigned long postingInterval = 1000;  // delay between updates to Xively.com

// Reading buffers
char serialData[85];
unsigned char ucharSerialData[85];

// Data relations
unsigned char SOH, EOT;
float moisture_1_1, moisture_1_2, moisture_1_3, moisture_2_1, moisture_2_2, moisture_2_3;
float moisture_3_1, moisture_3_2, moisture_3_3, moisture_4_1, moisture_4_2, moisture_4_3;
float atm_hum, atm_temp, wind_spd;
float cons_hum_1, cons_hum_2, cons_hum_3, cons_hum_4, eto;
unsigned int rad, pump_ste_1, pump_ste_2, pump_ste_3, pump_ste_4, wtr_flw_1, wtr_flw_2, wtr_flw_3, wtr_flw_4;
String dataString;

// Flags
boolean hasData = false, on_connection_flag = false;

// Timer
//Timer conn_timer;
//int timer_id; 
int counter, hour_counter;
// ---------------------------------------------------------------------------------


// ----------------------- Setup ---------------------------------------------------
void setup()
{
  wdt_disable();
  // initialize serial communications and wait for port to open:
  Serial.begin(9600);
  
  //Serial.println("Water Automation GRPS");
  //Serial.println("Waiting for GPRS signal");
  
  pinMode(43, OUTPUT);
  pinMode(53, OUTPUT);
    
  // After starting the modem with GSM.begin()
  // attach the shield to the GPRS network with the APN, login and password 
  gsmConnect();
  
  digitalWrite(43, LOW);
  //Serial.println("Connected to GPRS network");
  Timer1.initialize();
  Timer1.attachInterrupt(wtdCountdown);
  //Serial.println("Hour timer initiated");
}
// ---------------------------------------------------------------------------------


// ----------------------- Loop ----------------------------------------------------
void loop()
{
  // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only
  if (client.available())
  {
    char c = client.read();
    //Serial.print(c);
  }

  // if there's no net connection, but there was one last time
  // through the loop, then stop the client
  if (!client.connected() && lastConnected)
  {
    //Serial.println();
    //Serial.println("disconnecting.");
    client.stop();
  }

  // if you're not connected, and ten seconds have passed since
  // your last connection, then connect again and send data
  if(!client.connected() && (millis() - lastConnectionTime > postingInterval) && hasData)
  {
    Timer1.stop();
    digitalWrite(53, HIGH);
    sendData(dataString);
    hasData = false;
    digitalWrite(53, LOW);
  }
  // store the state of the connection for next time through
  // the loop
  lastConnected = client.connected();
}
// ---------------------------------------------------------------------------------


// ----------------------- Metods --------------------------------------------------
// this method makes a HTTP connection to the server
void sendData(String thisData)
{
  wdt_enable(WDTO_8S); // Enable watchdog as precaution
  
  // if there's a successful connection:
  if (client.connect(server, 80))
  {
    digitalWrite(43, HIGH);
    //Serial.println("connecting...");
    
    // send the HTTP PUT request:
    client.print("PUT /v2/feeds/");
    client.print(FEEDID);
    client.println(".csv HTTP/1.1");
    client.println("Host: api.xively.com");
    client.print("X-ApiKey: ");
    client.println(APIKEY);
    client.print("User-Agent: ");
    client.println(USERAGENT);
    client.print("Content-Length: ");
    client.println(thisData.length());

    // last pieces of the HTTP PUT request
    client.println("Content-Type: text/csv");
    client.println("Connection: close");
    client.println();
    
    // here's the actual content of the PUT request
    client.println(thisData);
    
    // For debuggin ----
    /*Serial.print("PUT /v2/feeds/");
    Serial.print(FEEDID);
    Serial.println(".csv HTTP/1.1");
    Serial.println("Host: api.xively.com");
    Serial.print("X-ApiKey: ");
    Serial.println(APIKEY);
    Serial.print("User-Agent: ");
    Serial.println(USERAGENT);
    Serial.print("Content-Length: ");
    Serial.println(thisData.length());

    // last pieces of the HTTP PUT request
    Serial.println("Content-Type: text/csv");
    Serial.println("Connection: close");
    Serial.println();
    Serial.println(thisData);
    Serial.println("Data sent");
    // -----------------------*/
  } 
  else
  {
    // if you couldn't make a connection
    //Serial.println("connection failed");
    //Serial.println();
    //Serial.println("disconnecting.");
    closeConnection();
  }  
  // note the time that the connection was made or attempted:
  lastConnectionTime = millis();
  digitalWrite(43, LOW);
  
  wdt_reset();
  wdt_disable();
}

String protocolHandler(unsigned char data[])
{
  SOH = data[15];
  EOT = data[82];

  if((SOH == 0x01) & (EOT == 0x04))
  {
    //Serial.println("Correct package");

    moisture_1_1 = float(data[17]) + (float(data[18])/100);
    moisture_1_2 = float(data[19]) + (float(data[20])/100);
    moisture_1_3 = float(data[21]) + (float(data[22])/100);
    moisture_2_1 = float(data[24]) + (float(data[25])/100);
    moisture_2_2 = float(data[26]) + (float(data[27])/100);
    moisture_2_3 = float(data[28]) + (float(data[29])/100);
    moisture_3_1 = float(data[31]) + (float(data[32])/100);
    moisture_3_2 = float(data[33]) + (float(data[34])/100);
    moisture_3_3 = float(data[35]) + (float(data[36])/100);
    moisture_4_1 = float(data[38]) + (float(data[39])/100);
    moisture_4_2 = float(data[40]) + (float(data[41])/100);
    moisture_4_3 = float(data[42]) + (float(data[43])/100);

    rad = (data[45]<<8) | data[46];
    atm_hum = float(data[47]) + (float(data[48])/100);
    atm_temp = float(data[49]) + (float(data[50])/100);
    wind_spd = float(data[51]) + (float(data[52])/100);
    eto = float(data[53]) + (float(data[54])/100);

    pump_ste_1 = int(data[56]);
    pump_ste_2 = int(data[57]);
    pump_ste_3 = int(data[58]);
    pump_ste_4 = int(data[59]);
    wtr_flw_1 = float(data[60]) + (float(data[61])/100);
    wtr_flw_2 = float(data[62]) + (float(data[63])/100);
    wtr_flw_3 = float(data[64]) + (float(data[65])/100);
    wtr_flw_4 = float(data[66]) + (float(data[67])/100);

    cons_hum_1 = float(data[69]) + (float(data[70])/100);
    cons_hum_2 = float(data[71]) + (float(data[72])/100);
    cons_hum_3 = float(data[73]) + (float(data[74])/100);
    cons_hum_4 = float(data[75]) + (float(data[76])/100);
    
    String dataString = "";
            
    dataString = "NS01A_1,";
    dataString += floatToString(moisture_1_1);
    dataString += "\nNS01A_2,";
    dataString += floatToString(moisture_1_2);
    dataString += "\nNS01A_3,";
    dataString += floatToString(moisture_1_3);
    dataString += "\nNS02A_1,";
    dataString += floatToString(moisture_2_1);
    dataString += "\nNS02A_2,";
    dataString += floatToString(moisture_2_2);
    dataString += "\nNS02A_3,";
    dataString += floatToString(moisture_2_3);
    dataString += "\nNS03A_1,";
    dataString += floatToString(moisture_3_1);
    dataString += "\nNS03A_2,";
    dataString += floatToString(moisture_3_2);
    dataString += "\nNS03A_3,";
    dataString += floatToString(moisture_3_3);
    dataString += "\nNS04A_1,";
    dataString += floatToString(moisture_4_1);
    dataString += "\nNS04A_2,";
    dataString += floatToString(moisture_4_2);
    dataString += "\nNS04A_3,";
    dataString += floatToString(moisture_4_3);
    dataString += "\nAtm_Humidity,";
    dataString += floatToString(atm_hum);
    dataString += "\nAtm_Temperature,";
    dataString += floatToString(atm_temp);
    dataString += "\nRadiation,";
    dataString += rad;
    dataString += "\nWind_Speed,";
    dataString += floatToString(wind_spd);
    dataString += "\nPump_State_1,";
    dataString += pump_ste_1;
    dataString += "\nPump_State_2,";
    dataString += pump_ste_2;
    dataString += "\nPump_State_3,";
    dataString += pump_ste_3;
    dataString += "\nPump_State_4,";
    dataString += pump_ste_4;
    dataString += "\nWater_Flow_1,";
    dataString += floatToString(wtr_flw_1);
    dataString += "\nWater_Flow_2,";
    dataString += floatToString(wtr_flw_2);
    dataString += "\nWater_Flow_3,";
    dataString += floatToString(wtr_flw_3);
    dataString += "\nWater_Flow_4,";
    dataString += floatToString(wtr_flw_4);
    dataString += "\nCons_Humidity_1,";
    dataString += floatToString(cons_hum_1);
    dataString += "\nCons_Humidity_2,";
    dataString += floatToString(cons_hum_2);
    dataString += "\nCons_Humidity_3,";
    dataString += floatToString(cons_hum_3);
    dataString += "\nCons_Humidity_4,";
    dataString += floatToString(cons_hum_4);
    dataString += "\nEvapotranspiration,";
    dataString += floatToString(eto);
    
    //Serial.println(dataString);
    return dataString;
  } else
  {
    //Serial.println("Incorrect package");
    return "";
  }

    char i;
    for(i=0; i<=85; i++) data[i] = 0;
}

String floatToString(float number)
{
  char tmp[6];
  dtostrf(number, 4, 2, tmp);
  return tmp;
}

void serialEvent()
{  
  Serial.readBytes(serialData, 85);
  int i;
  for(i=0; i<=85; i++) ucharSerialData[i] = serialData[i] & 0xFF;
  
  dataString = protocolHandler(ucharSerialData);
  
  hasData = true;
}

void closeConnection()
{
  client.stop();
}

void gsmConnect()
{
  // connection state
  boolean notConnected = true;

  //Serial.println("Beginning connection...");
  
  while(notConnected)
  {
    digitalWrite(43, HIGH);
    
    Timer1.initialize();
    Timer1.attachInterrupt(wtdCountdown);
    on_connection_flag = true;
    //Serial.println("Connecting to GSM");

    if(gsmAccess.begin(PINNUMBER) == GSM_READY)
    {
      //Serial.println("GSM_READY");
      counter = 0;
    }
    else
    {
      softReset();
    }
    
    //Serial.println("Connecting to GPRS");

    if(gprs.attachGPRS(GPRS_APN,GPRS_LOGIN, GPRS_PASSWORD, true) == GPRS_READY)
    {
      counter = 0;
      //Serial.println("GPRS_READY");     
    }
    else
    {
      softReset();
    }
    on_connection_flag = false;
    Timer1.stop();

    notConnected = false;
  }

  //Timer1.detachInterrupt();
  //Timer1.stop();
  digitalWrite(43, LOW);
}

void softReset() // Soft reset function
{ 
  Timer1.detachInterrupt();
  //Serial.println("Reseting...");
  char turnOff = 0;

  while(turnOff < 5)
  {
    digitalWrite(53, HIGH);
    digitalWrite(43, HIGH);
    delay(500);
    digitalWrite(53, LOW);
    digitalWrite(43, LOW);
    delay(500);
    turnOff++;
  }

  wdt_enable(WDTO_1S); 
  while(true);
}

void wtdCountdown()
{
  if((counter >= 60) || (hour_counter >= 600))
  {
    softReset();
  }
  else if(on_connection_flag)
  {
    counter++;
  }
  else
  {
    hour_counter++;
  }
  //Serial.print("Counter: ");
  //Serial.println(counter);
  //Serial.print("Hour Counter: ");
  //Serial.println(hour_counter);
}
// ---------------------------------------------------------------------------------



