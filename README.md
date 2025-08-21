# ESP32 Keypad Status External Component

This is an ESPHome external component for processing keypad input from air conditioning units.

## Installation

Add to your ESPHome configuration:

```yaml
external_components:
  - source: github://hazzas/esp32-keypad-status
    components: [ sensor.keypad_status ]
```

## Usage

The component provides:
- Keypad interrupt handling
- Pulse train processing (40 pulses)
- Temperature display decoding
- LED status monitoring for air conditioning functions

## Components

- `sensor.keypad_status` - Main keypad status sensor

## License

MIT License
