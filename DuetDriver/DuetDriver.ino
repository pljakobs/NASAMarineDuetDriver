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
* ===================
* ==   Changelog   ==
* ===================
* 
* Jul 5th 2021
* 
* depth measurements were extremely erroneous, often way too low.
* - increased the number of samples to be averaged from 8 to 32 and made the 
*   number of samples a #define (see depth_samples and speed_buckets) 
* - switched all pins from "INPUT" to "INPUT_PULLUP" as there were spurious
*   signals detected by the speed log, resulting in a 1Kn displayed speed at rest
*   that was not an issue once there were true pulses coming in. Seems the built in
*   micro is a bit over-sensitive.
*   
* Jul 8th 2021
* depth measurement was faulty as almost every 2nd measurement showed 0. Implemented a gate of min 20ms, 
* now the correct depth is shown.
* ready to use.
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
#define depth_samples 16
#define speed_buckets 8

auto timer = timer_create_default();

volatile uint32_t t_echo;
volatile uint8_t depth_valid;
volatile uint32_t t_gate;
uint32_t depth,speed;
uint8_t  depth_h,depth_l;
uint32_t depth_arr[depth_samples],speed_arr[speed_buckets];
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
  if(t_flag!=0 | digitalRead(p_gate)!=HIGH) {
    /* we have called startMeasure() more than once
     * without calling stopMeasure(), 
     * or the echo came back while gate was still 
     * active. in both cases, the result is invalid
     * reset flag to 0 to begin a new measurement
     * and don't record the erroneous value
     */
    t_echo=0;
    t_flag=0;
  }else{
    depth_arr[sample_d==depth_samples?sample_d=0:sample_d++]=t_echo;
  }
  //digitalWrite(led,LOW);
}

bool printDepthDebug(void *){
  for(i=0;i<=depth_samples-1;i++){
    Serial.print("t[");
    Serial.print(i);
    Serial.print("]=");
    Serial.println(depth_arr[i]);
    Serial.println("==========");
  }
  wdt_reset();
  return true;
}

bool printValues(void *){
  //char buffer[64];
  depth=0;
  speed=0;
  /*
   * summing up the values for 'depth_samples' measurements as a simplistic high pass filter
   * the data colletion for depth and speed are very different.
   * depth will hold the returns for the last 'depth samples' pings which, depending on how
   * often this printValues() function will be called, may or may not have overlap 
   * with the previous period. It's basically an average of the last measurements no matter.
   * For speed, there's a new bucket every time we call this function and every bucket
   * just counts the number of times the paddle wheel has 'clicked'.
   * For this specific device, it turns out that 1Hz corresponds to 0.19knots, if the 
   * printValues function is called every 500ms then the eight buckets are collected over
   * four seconds. To be a little more flexible, that's set as the print_interval define
   * and I'll calculate the actual time here
   * 
   */
  
  
  /* 
   * depth first, average of last measurements 
   */
  
  depth=0;
  for(i=0;i<=depth_samples-1;i++){
    depth=depth+depth_arr[i];
  }
  
  depth=depth/depth_samples*10/133;

  /*
   * speed next, also an average over the last 'speed_buckets' values
   */
  
  speed=0;
  for(i=0;i<=speed_buckets-1;i++){
    speed=speed+speed_arr[i];
  }
  
  speed=(speed*knots_per_1000hertz/100)/(speed_buckets*print_interval/1000);

  /*
   * increase the bucket counter and
   * zero the new bucket to use
   */
  sample_s==speed_buckets?sample_s=0:sample_s++;
  speed_arr[sample_s]=0;

  noInterrupts();
  //digitalWrite(led,HIGH);
  printSentenceWithChecksum("$SDDBT,,f,"+String(depth/100)+"."+String(depth%100)+",M,F*", false);
  printSentenceWithChecksum("$VWVHW,,,,,"+String(speed/10)+"."+String(speed%10)+",N,,,*", false);
  interrupts();
  wdt_reset();
  //digitalWrite(led,LOW);
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
  pinMode(p_speed, INPUT_PULLUP);
  
  pinMode(p_gate, INPUT_PULLUP);
  digitalWrite(p_gate,HIGH);
  
  pinMode(p_echo, INPUT_PULLUP); // for testing only
  digitalWrite(p_echo,HIGH);
  
  pinMode(led,OUTPUT);
  
  Serial.begin(115200);
  
  sample_d=0;
  sample_s=0;
  
  attachInterrupt(p_gate,startMeasure,FALLING);
  attachInterrupt(p_echo,stopMeasure,RISING);
  attachInterrupt(p_speed,speedCount,FALLING);
  
  timer.every(print_interval,printValues); // this calls the printValues routine every 500ms. Just around the serialPrint statements, the above interrupts will be disabled
  //timer.every(print_interval,printDepthDebug);
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

void printf2(uint32_t v,uint8_t d){
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
