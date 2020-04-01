#define PIN_SENSOR 0
#define MAX_uV_A_READ 5000000

void setup() {
  pinMode(PIN_SENSOR,INPUT);

  Serial.begin(9600);
} 

uint32_t u_volts(uint32_t val){
  return ((val * MAX_uV_A_READ) / 1024);
}
 
int32_t tmp_uC(uint32_t u_volts){
  if (u_volts>=3300000){
    return 150000000;
  } else if(u_volts <500000){
    return -50000000;
  } else {
    return (200000000 - (u_volts-500000)*71)-50000000;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  uint32_t val = analogRead(PIN_SENSOR);
  Serial.println(val);
  Serial.println(u_volts(val));
  Serial.println(tmp_uC(u_volts(val)));
  Serial.println("");
  delay(1000  );
}
