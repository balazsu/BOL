#include <Wire.h>
#include "SFM3000CORE.h"

#include <stdint.h>

////////////////////////
/////// SFM3000 SENSOR//
////////////////////////
SFM3000CORE senseFlow(64);
int32_t offset_sfm3000 = 32768;         // Offset flow, given in datasheet 
int32_t scale_sfm3000 = 120.0;        // Scale factor for Air & N2 is 140.0, O2 is 142.8
int32_t air_flow_sfm3000 = 0;         // air flow is given in slm (standard liter per minute)
int32_t air_volume_sfm3000 = 0;       // total air volume so far in sl 

uint32_t previous_time_sfm3000 = 0;         // time of previous measurement
////////////////////////

void init_sfm3000(){
    senseFlow.init();
    // first measurement might not be valid (according to datasheet)
    // therefore is unused in init function
    unsigned int result = senseFlow.getvalue();
    
    air_volume_sfm3000=0;

    // take the first measurement to init the
    // integration of the volume
    uint32_t curr_time = micros();
    result = senseFlow.getvalue();
    float flow = ((float)result - offset_sfm3000) / scale_sfm3000;

    // update previous time
    previous_time_sfm3000 = curr_time;
    // update current flow in slm
    air_flow_sfm3000 = flow;
}

void reset_sfm3000(){
    // reset volumes 
    air_volume_sfm3000=0;
    
    // take the first measurement to init the
    // integration of the volume
    unsigned long curr_time = micros();
    unsigned int result = senseFlow.getvalue();
    float flow = ((float)result - offset_sfm3000) / scale_sfm3000;
    
    // update previous time
    previous_time_sfm3000 = curr_time;
    // update current flow in slm
    air_flow_sfm3000 = flow;
}

void poll_sfm3000(){
    uint32_t curr_time = micros();
    uint16_t result = senseFlow.getvalue();
    int32_t flow = (((int32_t) result) - offset_sfm3000) * 1000 / scale_sfm3000; // sml/min

    // time diff [ms]
    int32_t delta = (curr_time - previous_time_sfm3000)/1000;

    // update total volumes using rectangle method (volume in [ml])
    int32_t inc = delta*flow;
    int32_t n_inc = inc / (60*1000L);
    air_volume_sfm3000 +=  n_inc;

    // update previous time
    previous_time_sfm3000 = curr_time;
    // update current flow in slm
    air_flow_sfm3000 = flow;
    Serial.print("air_volume ");
    Serial.print(air_volume_sfm3000);
    Serial.print("\tair_flow ");
    Serial.print(air_flow_sfm3000);
    Serial.print("\tdelta ");
    Serial.print(delta);
    Serial.print("\tinc ");
    Serial.print(inc);
    Serial.print("\tn_inc ");
    Serial.println(n_inc);
}
void setup() {
Serial.begin(9600);
    senseFlow.init();
        init_sfm3000();
}

void loop(){

    poll_sfm3000();
    delay(1000);
}
