/*****************************************************
 * name:WeatherStation
 * function: Get readings from humidity sensor (DHT22/AM2302),  
 * barometric sensor (BMP180)
 * libraries DHT from AdaFruit and BMP from SunFounder
 * serial output: date time, Temp F, Pressure, Humidity %, Heat Index F
 *****************************************************/
#include "DHT.h"
#include <SFE_BMP180.h>
#include <Wire.h>
#include <LiquidCrystal.h>

#define DHTPIN 7
#define DHTTYPE DHT22
#define ALTITUDE 20.56

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

const int switchPin = 6;
int switchState = 0;
int prevSwitchState = 0;
int reply = 4;
bool badBMP;

SFE_BMP180 bmp;
DHT dht(DHTPIN, DHTTYPE);

void setup()
{
  Serial.begin(9600); //initialize the serial monitor
  dht.begin();
  Serial.println("DHT Running");
  badBMP = false;
  
  // Initialize the sensor (it is important to get calibration values stored on the device).
  if (bmp.begin())
    Serial.println("BMP180 Running");
  else
  {
    // Oops, something went wrong, this is usually a connection problem
    sendErr("BMP180 init fail");
    badBMP = true;
    //while(1); // Pause forever.
  }
  lcd.begin(16,2);
  lcd.print("Weather Station");
}

void loop()
{
  delay(3000);

  char status;
  double P,nP,p0,a,alt;
  float inches;

  float h = dht.readHumidity();
  float tmpC = dht.readTemperature();
  float tmpF = dht.readTemperature(true);
  nP = 0;
  inches = 0.0;

  // also: float tmpF = 1.8*tmpC + 32.0; // convert °C to °F

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(tmpC) || isnan(tmpF)) {
    sendErr("Failed to read DHT sensor!");
    return;
  }

  float hif = dht.computeHeatIndex(tmpF, h);
  float hic = dht.computeHeatIndex(tmpC, h, false);

  // send values to USB comma delimited
  sendValue("HeatIndex",hif,"F");
  sendValue("HeatIndex",hic,"C");
  sendValue("Humidity",h,"%");
  sendValue("Temperature",tmpF,"F");
  sendValue("Temperature",tmpC,"C");
    
  if (badBMP) {
    Serial.println("error BMP not working");
    // return;
  }
  else
  {
    // Start a pressure measurement:
    // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
    // If request is successful, the number of ms to wait is returned.
    // If request is unsuccessful, 0 is returned.
    status = bmp.startPressure(3);
    if (status != 0)
    {
      // Wait for the measurement to complete:
      delay(status);
  
      // Retrieve the completed pressure measurement:
      // Function returns 1 if successful, 0 if failure.
      double T = (double)tmpC;
      status = bmp.getPressure(P,T);  
      if (status != 0)
      {
        //alt for Albuquerque in meters
        alt = 1981;
        nP = bmp.sealevel(P,alt);
        //To convert millibars to inches of mercury = nP * 0.0295301
        inches = nP*0.0295301;
        
        sendValue("Pressure",(float)nP,"mb");
        sendValue("Pressure",inches,"inches");
      }
      else sendErr("Cannot retrieve pressure from BMP");
    }
    else sendErr("Cannot start pressure measurement from BMP");
  }

  ++reply;
  
  if (reply > 3)
  {
    reply = 0;
  }
  // switch between each possible value so LCD cycles thru each one - not all at once
  switch(reply)
  {
    case 0:
      readout("Heat Index: ", hif, "F");
      delay(3000);
      readout("Heat Index: ", hic, "C");
      break;
      
    case 1:
      readout("Humidity",h,"%");
      break;

    case 2:
      readout("Temp: ",tmpF, "F");
      delay(3000);
      readout("Temp: ",tmpC, "C");
      break;

    case 3:
      readout("Pressure: ",inches,"inches");
      delay(3000);
      readout("Pressure: ",nP,"mb");
      break;
  }
  
  Serial.println("**************************");
}

void readout(String title, float value, String units){
  clearSet();
  lcd.print(title);
  lcd.setCursor(0,1);
  lcd.print(String(value,4) + " " + units); 
}

void sendValue(String tag, float value, String units){
  Serial.println(tag + "," + String(value,4) + " " + units);
}

void sendErr(String msg){
  Serial.println("Error: " + msg); 
}

void clearSet(){
  lcd.clear();
  lcd.setCursor(0, 0);  
}
