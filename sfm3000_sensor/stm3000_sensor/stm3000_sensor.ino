#include <Wire.h>
#include <SFM3000CORE.h>

////////////////////////
/////// SFM3000 SENSOR//
////////////////////////
SFM3000CORE senseFlow(64);
int offset_sfm3000 = 32000;         // Offset flow, given in datasheet 
float scale_sfm3000 = 140.0;        // Scale factor for Air & N2 is 140.0, O2 is 142.8
float scale_o2_sfm3000 = 142.9;     // Scale factor for Air & N2 is 140.0, O2 is 142.8
float air_flow_sfm3000 = 0;         // air flow is given in slm (standard liter per minute)
float air_volume_sfm3000 = 0;       // total air volume so far in sl 
float air_flow_o2_sfm3000 = 0;      // O2 flow is given in slm (standard liter per minute)
float air_volume_o2_sfm3000 = 0;    // total O2 volume so far in sl

unsigned long previous_time_sfm3000 = 0;         // time of previous measurement
////////////////////////

void init_sfm3000(){
    senseFlow.init();
    // first measurement might not be valid (according to datasheet)
    // therefore is unused in init function
    unsigned int result = senseFlow.getvalue();
    
    air_volume_sfm3000=0;
    air_volume_o2_sfm3000=0;

    // take the first measurement to init the
    // integration of the volume
    unsigned long curr_time = micros();
    result = senseFlow.getvalue();
    float flow = ((float)result - offset_sfm3000) / scale_sfm3000;
    float flow_o2 = ((float)result - offset_sfm3000) / scale_o2_sfm3000;

    // update previous time
    previous_time_sfm3000 = curr_time;
    // update current flow in slm
    air_flow_sfm3000 = flow;
    air_flow_o2_sfm3000 = flow_o2;
}

void reset_sfm3000(){
    // reset volumes 
    air_volume_sfm3000=0;
    air_volume_o2_sfm3000=0;
    
    // take the first measurement to init the
    // integration of the volume
    unsigned long curr_time = micros();
    unsigned int result = senseFlow.getvalue();
    float flow = ((float)result - offset_sfm3000) / scale_sfm3000;
    float flow_o2 = ((float)result - offset_sfm3000) / scale_o2_sfm3000;
    
    // update previous time
    previous_time_sfm3000 = curr_time;
    // update current flow in slm
    air_flow_sfm3000 = flow;
    air_flow_o2_sfm3000 = flow_o2;

}

void poll_sfm3000(){
    unsigned long curr_time = micros();
    unsigned int result = senseFlow.getvalue();
    float flow = ((float)result - offset_sfm3000) / scale_sfm3000;
    float flow_o2 = ((float)result - offset_sfm3000) / scale_o2_sfm3000;

    // time diff in minutes
    float delta = (float (curr_time - previous_time_sfm3000))/(60*1E6); // in minutes

    // update total volumes using trapezoid method
    // https://en.wikipedia.org/wiki/Trapezoidal_rule
    air_volume_sfm3000 +=   (delta) *  (flow + air_flow_sfm3000)/2;
    air_volume_o2_sfm3000 +=   (delta) *  (flow_o2 + air_flow_o2_sfm3000)/2;

    // update previous time
    previous_time_sfm3000 = curr_time;
    // update current flow in slm
    air_flow_sfm3000 = flow;
    air_flow_o2_sfm3000 = flow_o2;
}
void setup() {
    senseFlow.init();
        init_sfm3000();
}

void loop(){

    poll_sfm3000();
}
