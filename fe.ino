#include <hardware/timer.h>
#include <hardware/irq.h>
#include <MIDI.h>
#include "pitches.h"
#include "sounddata.h"
#include "Biquad.h"
#include <PWMAudio.h>
#define ADSR_IDLE 0
#define ADSR_ATTACK 1
#define ADSR_DECAY 2
#define ADSR_SUSTAIN 3
#define ADSR_RELEASE 4
float fs = 42000; // sample rate
float f0 = 1000; // center (cutoff) frequency
float Q = 4;
// dbGain parameter is required in Peaking, LowShelf, HighShelf
float dBGain = 2.0;
struct CustomBaudRateSettings : public MIDI_NAMESPACE::DefaultSerialSettings {
  static const long BaudRate = 115200;
};
Parameters params = {FilterType::LowPass, fs, f0, Q, dBGain};;

Biquad filter;
uint16_t frequency[128] = {8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 19, 21, 22, 23, 24, 26, 28, 29, 31, 33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62, 65, 69, 73, 78, 82, 87, 92, 98, 104, 110, 117, 123, 131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988, 1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976, 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951, 4186, 4435, 4699, 4978, 5274, 5588, 5920, 5920, 6645, 7040, 7459, 7902, 8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544};
MIDI_CREATE_DEFAULT_INSTANCE();
#define ALARM_NUM 1
#define ALARM_IRQ TIMER_IRQ_1
#define ALARM_FREQ 24000
float delay_1[2] = {0.0,0.0};
byte curr_state[8] = {0,0,0,0,0,0,0,0};
byte attack_rate[8] = {1,5,5,255,5,1,1,1};
byte max_attack[8] = {255,255,255,255,255,255,255,255};
byte decay_rate[8] = {1,1,1,1,2,1,1,1};
byte sustain_level[8] = {50,50,60,70,244,1,1,1};
byte release_rate[8] = {1,1,1,1,1,1,1,1};
byte filter_en[8] = {1,0,0,0,0,0,0,0};
// Phase Accumulator Variables
uint32_t osc_array_acc[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
float bend_float = 0;
uint32_t current_freq;
bool adsr_note_started[8] = {false, false, false, false, false, false, false, false} ;
// Tuning variables
uint32_t osc_array_tun[8] = { 20000, 500, 500, 200, 50, 20, 200, 20 };
uint16_t osc_array_pw[8] = { 32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768 };
uint32_t alarmPeriod;
byte oldVal[8] = {1,1,1,1,1,1,1,1};
byte newVal[8] = {1,1,1,1,1,1,1,1};
byte div1[8] = {1,1,1,1,1,1,1,1};
uint8_t out[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
byte volume[8] = {255,255,255,255,0,0,0,0};
byte waveform[8] = { 1, 2, 2, 2, 1, 1, 1, 1 };
byte adsr_en[8] = { 0,0,0,0,0,0,0,0 };
byte sync_en[8] = { 0,0,0,0,1,1,1,1 };
// Loop for the oscillators.
static void alarm_irq(void) {
  hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
  alarm_in_us_arm(alarmPeriod);
  digitalWrite(1, HIGH);
  for (int i = 0; i < 7; i++) {
    div1[i]++;

    if (div1[i] >= 20 && adsr_en[i] == 1){
           switch(curr_state[i]) {
    case ADSR_IDLE:
      if (adsr_note_started[i] == true) {
        curr_state[i] = ADSR_ATTACK;
      }
      break;
    case ADSR_ATTACK:
      if (adsr_note_started[i] == false) {
        curr_state[i] = ADSR_RELEASE;
      }
      volume[i] += attack_rate[i];
      if (volume[i] >= max_attack[i]) {
        curr_state[i] = ADSR_DECAY;
      }
      break;
    case ADSR_DECAY:
      if (adsr_note_started[i] == false) {
        curr_state[i] = ADSR_RELEASE;
      }
      volume[i] -= decay_rate[i];
      if (volume[i] <= sustain_level[i]) {
        curr_state[i] = ADSR_SUSTAIN;
      }
      break;
    case ADSR_SUSTAIN:
      if (adsr_note_started[i] == false) {
        curr_state[i] = ADSR_RELEASE;
      }
      break;
    case ADSR_RELEASE:
      volume[i] -= release_rate[i];
      if (volume[i] == 0) {
        curr_state[i] = ADSR_IDLE;
      }
      break;
    default:
      //Shit be really borken.

      break;
    }
    div1[i] = 1;
    }

    //while (osc_array_jp[i] <= 24) {
    //  
    //  osc_array_jp[i]++;
   // }
    oldVal[i] = newVal[i];
    newVal[i] = (osc_array_acc[i-1] >> 31);
      if (newVal[i] > oldVal[i] && sync_en[i] == 1) {
        osc_array_acc[i] = 0;
      }
      

    osc_array_acc[i] = osc_array_acc[i] + osc_array_tun[i];
    switch (waveform[i]) {
      case 1:


        out[i] = (lut[(osc_array_acc[i] >> 24) & 0xff] * volume[i]) / 255;
        break;
      case 2:
        if (((osc_array_acc[i] >> 16) & 0xffff) > osc_array_pw[i]) {
          out[i] = (0xFF * volume[i]) / 255;
        } else {
          out[i] = 0x0;
        }
        break;
    }
  }
  
  analogWrite(7, filter.process(mix_samples()) + 32768);
  
  digitalWrite(1, LOW);

}








int16_t filt = 0;
uint32_t prev_freq = 0;
uint16_t mix_samples() {
  digitalWrite(0, HIGH);
  int16_t mixed_sample = 0;
  for (int i = 0; i < 7; i++) {
    mixed_sample = ((int16_t)mixed_sample + (int16_t)out[i]);
  }
  mixed_sample = mixed_sample*16;

  // Clipping
  if (mixed_sample > 65535) {
    mixed_sample = 65535;
  } else if (mixed_sample < 0) {
    mixed_sample = 0;
  }
  digitalWrite(0, LOW);
  return (uint16_t)mixed_sample;
;
}
// Stuff for generating the 84KHz sample rate for the oscillators and everything else.
static void alarm_in_us_arm(uint32_t delay_us) {
  uint64_t target = timer_hw->timerawl + delay_us;
  timer_hw->alarm[ALARM_NUM] = (uint32_t)target;
}

static void alarm_in_us(uint32_t delay_us) {
  hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
  irq_set_exclusive_handler(ALARM_IRQ, alarm_irq);
  irq_set_enabled(ALARM_IRQ, true);
  alarm_in_us_arm(delay_us);
}

float noteToFreq(int note) {
  
  return frequency[note];
}

void noteOn(byte channel, byte pitch, byte velocity) {

  osc_array_tun[channel-1] = current_freq = noteToFreq(pitch) * (pow(2, 32) / 24000);
  adsr_note_started[channel-1] = true;

}
void noteOff(byte channel, byte pitch, byte velocity) {
  current_freq = 0;
  adsr_note_started[channel-1] = false;
}

void pitchBend(byte channel, int bend) {
  bend_float = (float) bend;                    //Convert bend (int) to float type to get also the after zero numbers
  float bendfactor = (1 + bend_float/8190);      // Calculate the bend factor, with wich the tone() in [hz] shall be bended. Bendfactor shall be between 0.1 and 2, if no bending is applied --> 1.
  if ((current_freq*bendfactor) > 50) {
       osc_array_tun[channel-1] = (current_freq*bendfactor);
  }            // To prevent the output from beeing in an unstable state when note is not properly ended (e.g. not "OFF" has been send
         


}

void controlChange(byte channel, byte cc, byte value) {
    // To prevent the output from beeing in an unstable state when note is not properly ended (e.g. not "OFF" has been send
  if (cc == 110){
    osc_array_pw[channel-1] = value * 500;
  }
  if (cc == 111){
    volume[channel-1] = value * 2;
  }
    if (cc == 112){
    waveform[channel-1] = value;

    }
    if (cc == 113){
    sync_en[channel-1] = (value >= 63) ? 1 : 0;
  }
      if (cc == 114){
        f0 = value * 32 + 10;
        params = {FilterType::LowShelf, fs, f0, Q, dBGain};
    filter.setParams(params);
  }
       if (cc == 115){
    adsr_en[channel-1] = (value >= 63) ? 1 : 0;
  }
}





void setup() {
  analogWriteResolution(16);
  filter.setParams(params);
  // put your setup code here, to run once:

  analogWriteFreq(200000);
  pinMode(1, OUTPUT);
   pinMode(0, OUTPUT);
  alarmPeriod = 1000000 / ALARM_FREQ;
  alarm_in_us(alarmPeriod);
}
void loop() {
  // put your main code here, to run repeatedly:
  
}
void setup1() {
    MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(noteOn);
  MIDI.setHandleNoteOff(noteOff);
  MIDI.setHandlePitchBend(pitchBend);
  MIDI.setHandleControlChange(controlChange);
}
void loop1(){
  MIDI.read();
}