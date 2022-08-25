#define OUTPUT1_mq7 25
#define OUTPUT2_mq7 26
#define INPUT_PWM_SIG 34
#define PWM_SIG 15
#define reference_voltage_value_heat 5
#define reference_voltage_value_logic 3.3
#define period1 60*1000L
#define period2 90*1000L
#define period3 1000L
#define MQ7_LOAD_RESISTOR 10000
#define MQ7_CLEANAIR_RSRO 11.7

// задаємо властивості PWM сигналу
const int freq = 100;
const int ledChannel = 0;
const int resolution = 8;
// задаємо кількисть усереднень для серед. арифм. фільтр
const int NUM_READ = 1000;  

int DutyCycle;
uint32_t tmr;
uint32_t tmr2;
bool flag = true;
float mq7_RsRo1;
float mq7_RsRo2;
//float mq7_volts; 
float mq7_volts1;
float mq7_volts2; 
bool flag_sensor=false;
int mq7_Ro1 = 10000;
int mq7_Ro2 = 10000;
int ppm1;
int ppm2;



// середнє арифметичне для float
float midArifm() {
  float sum = 0;                      
  for (int i = 0; i < NUM_READ; i++)  
    sum += analogRead(INPUT_PWM_SIG);                  
  return (sum / NUM_READ);
}

// функція перетворення значень ADC в вольт для імпульсного режиму роботи
float analog_pin_voltage_arithmeticMean(int number_analog_pin, float reference_voltage_value){
  float voltage = (float)(midArifm() * reference_voltage_value) / 4095;
  
  return voltage;
}
// функція перетворення значень ADC в вольт
float analog_pin_voltage(int number_analog_pin, float reference_voltage_value){
  float voltage = (float)(analogRead(number_analog_pin) * reference_voltage_value) / 4095;
  
  return voltage;
}

// функція, яка знаходить значення ppm
int mq7_rawValue(bool flag, float reference_voltage_value, float number_analog_pin) {
  int ppm;
  int Rs;
  float RsRo;
  float mq7_volts = analog_pin_voltage(number_analog_pin, reference_voltage_value);
 
  if(flag==0){ mq7_volts1=mq7_volts;}
  if(flag==1) { mq7_volts2 = mq7_volts; }
  
  Rs=((reference_voltage_value*MQ7_LOAD_RESISTOR)/mq7_volts)-MQ7_LOAD_RESISTOR;
  //RsRo=(float)Rs/(float)mq7_Ro;
  if(flag==0){ 
    mq7_volts1=mq7_volts;
    RsRo=(float)Rs/(float)mq7_Ro1;
    mq7_RsRo1 = RsRo;
  }
  if(flag==1) { 
    mq7_volts2 = mq7_volts;
    RsRo=(float)Rs/(float)mq7_Ro2;
    mq7_RsRo2 = RsRo;
   }

  if(RsRo>0.09) {
    ppm=(pow((0.196/RsRo),(1/0.72)))*1000;
  }
  else {ppm=3999;}

  return (int)ppm;
  
}

// функція, яка калібрує значення Ro
int calib_Ro(float reference_voltage_value, int number_analog_pin ){
  
  int Ro;
  int Rs;
  float mq7_volts = analog_pin_voltage(number_analog_pin, reference_voltage_value);
  
  if(mq7_volts<0.1){
     Rs=((reference_voltage_value*MQ7_LOAD_RESISTOR)/0.1)-MQ7_LOAD_RESISTOR;
  }
  else {
    Rs=((reference_voltage_value*MQ7_LOAD_RESISTOR)/mq7_volts)-MQ7_LOAD_RESISTOR;
  }
  
  Ro = Rs/MQ7_CLEANAIR_RSRO;
  return Ro;
}
// Фуннкція, яка калібрує коеф. заповнення PWM сигналу
void freq_setup() {
    // калібруємо напругу на транзисторія для режиму вимірювання
    for(int dutyCycle = 0; dutyCycle <= 255; dutyCycle++){   
    
      ledcWrite(ledChannel, dutyCycle);
      //delay(50);
      
      float volt_level = analog_pin_voltage_arithmeticMean(INPUT_PWM_SIG, reference_voltage_value_heat); 
      if(volt_level >= 3.43 && volt_level <= 3.51 )
      {
      DutyCycle = dutyCycle;
      Serial.print("voltage: ");  
      Serial.println(volt_level);
      Serial.println(DutyCycle);
      break;
      }
    }
  
}

void setup() {
  //analogReference(DEFAULT);
  // put your setup code here, to run once:
  ledcSetup(ledChannel, freq, resolution);
  // привязываем канал к портам 
  ledcAttachPin(PWM_SIG, ledChannel);

  //int cycle = freq_setup();

  Serial.begin(115200);
  
  freq_setup();
  ledcWrite(ledChannel, 255);
  delay(60000);
  ledcWrite(ledChannel, DutyCycle);
  delay(90000);
  mq7_Ro1=calib_Ro(reference_voltage_value_logic, OUTPUT1_mq7);
  mq7_Ro2=calib_Ro(reference_voltage_value_logic, OUTPUT2_mq7);

  //Serial.println(cycle);
  Serial.print("mq7_Ro1: ");
  Serial.print(mq7_Ro1);
  Serial.print("; mq7_Ro2: ");
  Serial.println(mq7_Ro2);
  
}

void loop() {
  // put your main code here, to run repeatedly:
  if (millis() - tmr >= (flag ? period2 : period1)) {
    tmr = millis();
    
    // код
      if(flag==0)
      {
        ledcWrite(ledChannel, 255);
      }
      if(flag==1)
      {
        ledcWrite(ledChannel, DutyCycle);
      }
     flag = !flag;
    }
    if (millis() - tmr2 >= period3)
    {
      tmr2 = millis();
      ppm1=mq7_rawValue(flag_sensor, reference_voltage_value_logic, OUTPUT1_mq7);
      ppm2=mq7_rawValue(flag_sensor, reference_voltage_value_logic, OUTPUT2_mq7);
      Serial.print("ppm1: ");  
      Serial.print(ppm1 );
      Serial.print("; ppm2: ");  
      Serial.print(ppm2 );
      Serial.print("; deltaPPM: ");  
      Serial.println(ppm2-ppm1 );
    }
}
    
