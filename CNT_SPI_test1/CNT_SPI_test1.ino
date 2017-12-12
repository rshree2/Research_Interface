#include <SPI.h>
#include <Wire.h>
const int BUFF_SIZE = 1;

char incomingbyte; 
//-----------------RESETTING MILLIS() (NOT FROM THE CORE)-----------
extern volatile unsigned long timer0_millis;
unsigned long new_value = 0;
//------------------STRUCTURE FOR SENSOR DATA CALCULATION-----------
typedef struct resistance
{
  double R, T;
  double H;
  struct resistance *next;
} res;
res *head = NULL;
res *tail = NULL;
res *temp = NULL;
int curr_buffsize = 0;
double temp_resistance, temp_temperature, temp_humidity;
double Average, Average_Temperature, Average_Humidity;
long int time = 0;

//--------------------------I2C HUMIDITY SENSOR---------------------
byte WritetoSlave = 1001110;
byte ReadfromSlave = 1001111;
uint8_t byteA = 0;
uint8_t byteB = 0;
//---------------HUMIDITY SENSOR VARIALBES-----------------------------
float slope = 0.031483;
float offset = 0.826;
float humidity;
const int inputHumidity = A0;
const int inputtemperature = A1;
//---------------SPI INTERFACE AND BACK CALCULATION VARIABLES-----------
byte byteOne = 0;
byte byteTwo = 0;
const int slaveSelectPin = 10;
const int DataIn = 12;
const int slk = 13;
double Voltage, Vx, Vminus, Vcnt;
double resistance, temperature, digitalVal, Rcnt, Icnt;
//--------------CONSTNT TIME INTERVAL DECLARATION-----------------------
int previousmillis = 0;

//--------------RUNNING AVERAGE FUNCTION--------------------------------
void GetAverage()
{ temp_resistance = 0;
  temp_temperature = 0;
  temp_humidity = 0;
  temp = head;
  while ( temp != NULL ) //travel through the linked list
  {
    temp_resistance += temp->R;
    temp_temperature += temp->T;
    temp_humidity += temp->H;
    temp = temp->next;
  }
  /****************get averages***************/
  Average = temp_resistance / curr_buffsize;
  Average_Temperature = temp_temperature / curr_buffsize;
  Average_Humidity = temp_humidity / curr_buffsize;
  temp = head->next;
  free (head);
  head = temp;
  curr_buffsize --;
  time++;
  //Average_hum_byte= String(Average_Humidity);
  //Average_temp_byte= String(Average_Temperature);
  //Average_byte= String(Average);
 // Serial.print(time);
 // Serial.print("\t");
  Serial.print(Average_Humidity,4);
  Serial.print("\n");
  Serial.print(Average_Temperature,4);
  Serial.print("\n");
  Serial.print(Average,4);
  Serial.print("\n");
  Serial.flush();

}

//char buffer[5];

void setup()
{
  pinMode(slaveSelectPin, OUTPUT);
  pinMode(DataIn, OUTPUT);
  pinMode(slk, OUTPUT);
  SPI.begin();
  SPI.setDataMode(SPI_MODE1);
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV64);
  pinMode(A0, INPUT);

  Wire.begin();
  Wire.beginTransmission(WritetoSlave);
  Wire.endTransmission();

  // file.open("/home/roshan/Documents/test_test.txt",ios_base::out |ios_base::app);
  /*  if (!file)
    {
      Serial.println("File nit found");
      exit(1);
    }
    else
    {
      file<<"TIME"<<"\t"<<"RH%"<<"\t"<<"TEMPERATURE"<<"\t"<<"RESISTANCE"<<endl;
    }*/
  Serial.begin(9600);
}

void loop()
{
  unsigned long currentmillis = millis();
  
 // if (Serial.available() > 0)
 // {
 //   incomingbyte=Serial.read();
   // if(incomingbyte=='A')
   // {
  if (currentmillis - previousmillis >= 1000)
  {
    //---------------------------------SPI INTERFACE----------------------------------
    digitalWrite(slaveSelectPin, HIGH);
    digitalWrite(slaveSelectPin, LOW);
    byteOne = SPI.transfer(0x00);
    byteTwo = SPI.transfer(0x00);
    digitalWrite(slaveSelectPin, HIGH);
    uint16_t result = byteOne & 0x1F;
    result = result << 8;
    result = result | byteTwo;
    result = result >> 1;
    digitalVal = (((float) result / 4096.0) * 5.0);
    // Serial.print("Voltage: ");
    //Serial.println(digitalVal,3);
    // Serial.print(",");

    //------------------------RELATIVE HUMIDITY------------------------------------------
    float digitalhumidity = analogRead(inputHumidity);
    float humidity_temp = ((digitalhumidity / 1024.0) * 5000.0);
    //Serial.println(humidity_temp);
    humidity = (humidity_temp / 5000.0) * 100.0;
    // Serial.print("Relative Humidity %:");
    //Serial.println(humidity,3);
    // Serial.print(",");
    //------------------------RELATIVE TEMPERATURE----------------------------------------
    float digitaltemperature = analogRead(A1);
    // Serial.println(digitaltemperature);
    float voltage_temp = ((digitaltemperature / 1024.0) * 5.0) * 1000.0;
    temperature = ((voltage_temp / 5000.0) - 0.242424) * 165.0;
    //Serial.print("Temperature %:");
    // Serial.print(temperature,3);
    // Serial.print(",");
    //-----------------------RELATIVE HUMIDITY-2---------------------------------------
    // Wire.requestFrom(0x27, 2);
    //Wire.beginTransmission(ReadfromSlave);
    byteA = Wire.read();
    //Serial.println(byteA,BIN);
    byteB = Wire.read();
    //Serial.println(byteB,BIN);
    uint16_t result_I2C = byteA << 8;
    result_I2C = result_I2C | byteB;
    result_I2C = result_I2C >> 2;
    float RH_i2c = (result_I2C / (16382.0) * 100.00);
    // Serial.print("RH  ");
    // Serial.println(RH_i2c,3);
    //-----------------------BACK CALCULATION FOR RESISTANCE DETECTION---------------------
    Vx = (3 * (-digitalVal)) + 5;
    //Serial.println(Vx,5);
    Vminus = (-(Vx) / 23.454545) + 0.454545;
    // Serial.println(Vminus,5);
    Vcnt = Vminus - 0.08238;
    //Serial.println(Vcnt,5);
    Icnt = 0.00172;
    Rcnt = Vcnt / Icnt;
    // Serial.println(Rcnt,4);
    //------------------------RUNNNING AVERAGE FOR SENSOR CALCULATION----------------------
    if (head == NULL )
    {
      head = (res *)malloc (sizeof(res));
      head->R = Rcnt;
      head->T = temperature;
      head->H = humidity;
      head->next = NULL;
      temp = head;
      tail = head;
      curr_buffsize++;
    }
    else
    {
      tail->next = (res *)malloc (sizeof(res));
      tail = tail->next;
      tail->R = Rcnt;
      tail->T = temperature;
      tail->H = humidity;
      tail->next = NULL;
      curr_buffsize++;
    }
    if (curr_buffsize >= BUFF_SIZE ) //&& curr_buffsize_temp>= BUFF_SIZE && curr_buffsize_hum>=BUFF_SIZE)
    {
      GetAverage();
     // Serial.println(currentmillis-previousmillis);
    }
   // previousmillis = currentmillis;
    delay(1000);
 // }
  setMillis(1000);
  }
}



//-----------------------------CODE TO RESET MILLIS BACK TO 0------------------
void setMillis(unsigned long new_millis){
  uint8_t oldSREG = SREG;
  cli();
  timer0_millis = new_millis;
  SREG = oldSREG;
}
