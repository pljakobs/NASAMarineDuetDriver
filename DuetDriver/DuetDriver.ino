#include <arduino-timer.h>
#include <avr/wdt.h>

/* nasamarine duet driver
*
*
*connecting to the device:
*
*
*   conn        **************
*  XXXXXXX           PIC
*               **************
*                      ^  ^^^
*                      |  /|\
*                      S|C|G|E
*                      p|h|a|c
*                      e|i|t|h
*                      e|r|e|o
*                      d|p
*
* Pin map for Atmega 32u4
*
* Speed -> rx  (int2) 
* Gate  -> scl (int0)
* Echo  -> sda (int1)
* 
*/


#define p_speed 2
#define p_gate 0
#define p_echo 1

#define print_interval 500 //output values every 500ms 
#define knots_per_1000hertz 198 //knots at 100Hz - this is used as a builtin 10x factor to be able to have one digit precision while using ints
#define led LED_BUILTIN

#define measurement_inval 0

#define measurement_start 1
#define measurement_valid 2
#define measurement_gated 3

auto timer = timer_create_default();

volatile uint32_t t_echo;
volatile uint8_t depth_valid;
volatile uint32_t t_gate;
uint32_t depth,speed;
uint8_t  depth_h,depth_l;
uint32_t depth_arr[8],speed_arr[8];
uint8_t sample_d,sample_s,i,t_flag;

static void startMeasure(){
  t_gate=micros();
  //depth_valid=measurement_start;
  t_flag++;
  //digitalWrite(led,HIGH);
}

static void stopMeasure(){
  t_echo=micros()-t_gate-400;
  t_flag--;
  if(t_flag&&1!=0 | digitalRead(p_gate)!=HIGH) {
    t_echo=0;
    t_flag=0;
  }
  depth_arr[sample_d==8?sample_d=0:sample_d++]=t_echo;
  //digitalWrite(led,LOW);
}

static void printValues(){
  char buffer[64];
  depth=0;
  speed=0;
  /*
   * summing up the values for eight measurements as a simplistic high pass filter
   * the data colletion for depth and speed are very different.
   * depth will hold the returns for the last eight pings which, depending on how
   * often this printValues() function will be called, may or may not have overlap 
   * with the previous period. It's basically an average of the last eigh no matter.
   * For speed, there's a new bucket every time we call this function and every bucket
   * just counts the number of times the paddle wheel has 'clicked'.
   * For this specific device, it turns out that 1Hz corresponds to 0.19knots, if the 
   * printValues function is called every 500ms then the eight buckets are collected over
   * four seconds. To be a little more flexible, that's set as the print_interval define
   * and I'll calculate the actual time here
   * 
   */
  for(i=0;i<=7;i++){
    depth=depth+depth_arr[i];
    speed=speed+speed_arr[i];
  }
  /* 
   * depth first, average of last eight measurements 
   */
  depth=depth/8*10/133;

  /*
   * increase the bucket counter and
   * zero the new bucket to use
   */
  sample_s==8?sample_s=0:sample_s++;
  speed_arr[sample_s]=0;

  speed=(speed*knots_per_1000hertz/100)/(8*print_interval/1000);

  noInterrupts();
  digitalWrite(led,HIGH);
  printSentenceWithChecksum("$SDDBT,,f,"+String(depth/100)+"."+String(depth%100)+",M,F*", false);
  printSentenceWithChecksum("$VWVHW,,,,,"+String(speed/10)+"."+String(speed%10)+",N,,,*", false);
  /*
   * Serial.print("$SDDBT,,f,");
   * printf2(depth,2);
   * Serial.print("M,");
   * Serial.print(",F;\r\n");
   * Serial.print("$VWVHW,,,,");
   * printf2(speed,1); 
   * Serial.print("N,,,;\r\n");  
  */
  digitalWrite(led,LOW);
  interrupts();
  wdt_reset();
  return true;
}

static void speedCount(){
  speed_arr[sample_s]++;
}

void setup() {
  /*
   * we will reset the device if the watchdog was not reset within two secods, 
   * which is four print cycles
   */
  wdt_enable(WDTO_2S);

  /* 
   * Pin modes for the three input and the one output pins 
   */
  pinMode(p_speed, INPUT);
  
  pinMode(p_gate, INPUT);
  digitalWrite(p_gate,HIGH);
  
  pinMode(p_echo, INPUT); // for testing only
  digitalWrite(p_echo,HIGH);
  
  pinMode(led,OUTPUT);
  
  Serial.begin(115200);
  
  sample_d=0;
  sample_s=0;
  
  attachInterrupt(p_gate,startMeasure,FALLING);
  attachInterrupt(p_echo,stopMeasure,RISING);
  attachInterrupt(p_speed,speedCount,FALLING);
  
  timer.every(print_interval,printValues); // this calls the printValues routine every 500ms. Just around the serialPrint statements, the above interrupts will be disabled
  Serial.println("started");
}


void loop() {
  /*
   * nothing to do in loop than to keep the time
   */
  timer.tick();
}

uint32_t m2ft(uint32_t l){
  return l*328/100;
}

uint32_t m2f(uint32_t l){
  return l*554/1000;
}

uint32_t printf2(uint32_t v,uint8_t d){
  int e;
  /*
   * the switch statement is technically a dirty hack but for reasons unexplained,
   * pow(10,2) evaluated to 99 rather than to 100
   */
  switch (d){
    case 1:
      e=10;
      break;
    case 2:
      e=100;
      break;
  }
  Serial.print(v/e);
  Serial.print(".");
  Serial.print(v%e);
}
