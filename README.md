# Little NRG Connect

This project is a minimal alternative to the NRGkick Connect, designed for the 
[ESP32-POE board](https://www.olimex.com/Products/IoT/ESP32/ESP32-POE/open-source-hardware).

This implementation is mainly created from and for the [evcc project](https://github.com/evcc-io/evcc).


## Limitations

* only supports ESP32-POE board and only Ethernet
* no configuration (use DHCP to give device a static IP)
* only a bare minimum of the API is implemented

## Getting Started
1. Clone this repository.
2. Build and upload the firmware using PlatformIO.

## Pantabox API

The NRGkick Connect API does not support car detection.

As a workaround also the Pantabox API is implemented and must be configured as:
```yaml
chargers:
  - name: nrgkick
    type: pantabox
    uri: http://[IP]/pantabox/[MAC]/[PIN]/
```