#include "esphome.h"

#define NPULSE 40  // Define the number of pulses to be captured

// ESPHome 2025.8.0+ compatible external component
class KeypadStatusSensor : public sensor::Sensor, public Component {
  private:
    // Sensor references
    sensor::Sensor *setpoint_temp_sensor;
    text_sensor::TextSensor *bit_string_sensor;
    sensor::Sensor *bit_count_sensor;
    binary_sensor::BinarySensor *cool_sensor;
    binary_sensor::BinarySensor *auto_md_sensor;
    binary_sensor::BinarySensor *run_sensor;
    binary_sensor::BinarySensor *fan_cont_sensor;
    binary_sensor::BinarySensor *fan_hi_sensor;
    binary_sensor::BinarySensor *fan_mid_sensor;
    binary_sensor::BinarySensor *fan_low_sensor;
    binary_sensor::BinarySensor *room3_sensor;
    binary_sensor::BinarySensor *room4_sensor;
    binary_sensor::BinarySensor *room2_sensor;
    binary_sensor::BinarySensor *heat_sensor;
    binary_sensor::BinarySensor *room1_sensor;
    
    // GPIO pin
    int pin_;
    
    // Internal state
    unsigned long last_intr_us;
    unsigned long last_work;
    char pulse_vec[NPULSE];
    volatile unsigned char nlow;
    volatile unsigned char nbits;
    volatile unsigned char dbg_nerr;
    volatile bool do_work;
    bool data_error;
    bool newdata;
    char p[NPULSE];

  public:
    // Constructor
    KeypadStatusSensor() : Sensor("keypad_status") {}
    
    // Setup method
    void setup() override {
      pinMode(pin_, INPUT);
      attachInterrupt(digitalPinToInterrupt(pin_), [this]() { this->handleIntr(); }, FALLING);
    }
    
    // Loop method
    void loop() override {
      mloop();
      
      if (newdata) {
        // Update bit string sensor
        std::string text = "";
        for(int i = 0; i < NPULSE; ++i) {
          text += (p[i] ? '1' : '0');
        }
        if (bit_string_sensor) bit_string_sensor->publish_state(text);
        
        // Update temperature sensor
        float display_value = get_display_value();
        if (display_value > 0 && setpoint_temp_sensor) {
          setpoint_temp_sensor->publish_state(display_value);
        }
        
        // Update all binary sensors
        if (cool_sensor) cool_sensor->publish_state(p[0] != 0);
        if (auto_md_sensor) auto_md_sensor->publish_state(p[1] != 0);
        if (run_sensor) run_sensor->publish_state(p[3] != 0);
        if (fan_cont_sensor) fan_cont_sensor->publish_state(p[8] != 0);
        if (fan_hi_sensor) fan_hi_sensor->publish_state(p[9] != 0);
        if (fan_mid_sensor) fan_mid_sensor->publish_state(p[10] != 0);
        if (fan_low_sensor) fan_low_sensor->publish_state(p[11] != 0);
        if (room3_sensor) room3_sensor->publish_state(p[12] != 0);
        if (room4_sensor) room4_sensor->publish_state(p[13] != 0);
        if (room2_sensor) room2_sensor->publish_state(p[14] != 0);
        if (heat_sensor) heat_sensor->publish_state(p[15] != 0);
        if (room1_sensor) room1_sensor->publish_state(p[21] != 0);
        
        newdata = false;
      }
    }
    
    // Set pin
    void set_pin(int pin) { pin_ = pin; }
    
    // Set sensor references
    void set_setpoint_temp_sensor(sensor::Sensor *sensor) { setpoint_temp_sensor = sensor; }
    void set_bit_string_sensor(text_sensor::TextSensor *sensor) { bit_string_sensor = sensor; }
    void set_bit_count_sensor(sensor::Sensor *sensor) { bit_count_sensor = sensor; }
    void set_cool_sensor(binary_sensor::BinarySensor *sensor) { cool_sensor = sensor; }
    void set_auto_md_sensor(binary_sensor::BinarySensor *sensor) { auto_md_sensor = sensor; }
    void set_run_sensor(binary_sensor::BinarySensor *sensor) { run_sensor = sensor; }
    void set_fan_cont_sensor(binary_sensor::BinarySensor *sensor) { fan_cont_sensor = sensor; }
    void set_fan_hi_sensor(binary_sensor::BinarySensor *sensor) { fan_hi_sensor = sensor; }
    void set_fan_mid_sensor(binary_sensor::BinarySensor *sensor) { fan_mid_sensor = sensor; }
    void set_fan_low_sensor(binary_sensor::BinarySensor *sensor) { fan_low_sensor = sensor; }
    void set_room3_sensor(binary_sensor::BinarySensor *sensor) { room3_sensor = sensor; }
    void set_room4_sensor(binary_sensor::BinarySensor *sensor) { room4_sensor = sensor; }
    void set_room2_sensor(binary_sensor::BinarySensor *sensor) { room2_sensor = sensor; }
    void set_heat_sensor(binary_sensor::BinarySensor *sensor) { heat_sensor = sensor; }
    void set_room1_sensor(binary_sensor::BinarySensor *sensor) { room1_sensor = sensor; }

  private:
    // Interrupt handler
    void handleIntr() {
      auto nowu = micros();
      unsigned long dtu = nowu - last_intr_us;
      last_intr_us = nowu;
      if (dtu > 2000) {
        nlow = 0;
        return;
      }
      if (nlow >= NPULSE) nlow = NPULSE - 1;
      pulse_vec[nlow] = dtu < 1000;
      ++nlow;
      do_work = 1;
    }

    // Main loop processing
    void mloop() {
      unsigned long now = micros();
      if (do_work) {
        do_work = 0;
        last_work = now;
      } else {
        unsigned long dt = now - last_work;
        if (dt > 50000 && nlow == 40) {
          if(memcmp(p, pulse_vec, sizeof p) != 0) newdata = true;
          memcpy(p, pulse_vec, sizeof p);
        }
      }
    }

    // Decode digit from segment bits
    char decode_digit(uint8_t hex_value) {
      switch (hex_value) {
        case 0x3F: return '0';
        case 0x06: return '1';
        case 0x5B: return '2';
        case 0x4F: return '3';
        case 0x66: return '4';
        case 0x6D: return '5';
        case 0x7C: return '6';
        case 0x07: return '7';
        case 0x7F: return '8';
        case 0x67: return '9';
        case 0x73: return 'P';
        default: return '?';
      }
    }

    // Get display value from segment bits
    float get_display_value() {
      uint8_t digit1_bits = (p[37] << 6) | (p[38] << 5) | (p[36] << 4) | (p[32] << 3) | (p[34] << 2) | (p[35] << 1) | p[39];
      uint8_t digit2_bits = (p[26] << 6) | (p[25] << 5) | (p[27] << 4) | (p[23] << 3) | (p[29] << 2) | (p[24] << 1) | p[30];
      uint8_t digit3_bits = (p[18] << 6) | (p[17] << 5) | (p[22] << 4) | (p[19] << 3) | (p[20] << 2) | (p[16] << 1) | p[21];

      std::string display_str;
      display_str += decode_digit(digit1_bits);
      display_str += decode_digit(digit2_bits);
      display_str += decode_digit(digit3_bits);

      for (char c : display_str) {
        if (!isdigit(c)) return -1.0f;
      }
      float display_value = std::stof(display_str);
      if (p[28]) display_value *= 0.1f; // Apply decimal point
      return display_value;
    }
};
