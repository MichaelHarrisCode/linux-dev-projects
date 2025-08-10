# TSL2561 IIO Kernel Driver (Learning Project)

This is a learning-focused Linux kernel driver for the **TSL2561 ambient light sensor**, integrating with the **IIO (Industrial I/O) subsystem**.

---

## Objective

* Learn Linux kernel subsystem integration via **IIO**
* Practice driver development using a new device and simulated environment
* Expose and read raw data from a dual-channel light sensor (IR and broadband)

---

## Device Overview

* **Device**: TSL2561 Digital Ambient Light Sensor
* **Protocol**: I²C
* **Address**: 0x39
* **Features**:

  * Dual photodiode channels (IR and full spectrum)
  * Selectable gain (1x or 16x)
  * Configurable integration time (13.7ms, 101ms, 402ms)

---

## Files & Structure

* `tsl2561.c` – Main driver source
* `Kconfig` / `Makefile` – For kernel build integration
* (Optional) `tsl2561.h` – Register definitions and constants

---

## Driver Features

### Initialization

* Registers with the I²C and IIO subsystems
* Reads ID register (`0x0A`) to validate presence

### IIO Channels

* `in_illuminance_ir_raw`
* `in_illuminance_broadband_raw`

### Configurable Sysfs Parameters

* `in_illuminance_gain` — Values: `1` or `16`
* `in_illuminance_integration_time` — Values: `13`, `101`, `402` (ms)

### Data Reading Flow

* Powers sensor
* Waits for integration duration
* Reads ADC data from registers
* Powers down device

---

## Simulated Testing Setup

### Load the Stub Driver

```sh
sudo modprobe i2c-stub chip_addr=0x39
```

### Simulate Register Values

```sh
# Set CH0 (broadband) to 0xBBAA
i2cset -y 10 0x39 0x0C 0xAA
i2cset -y 10 0x39 0x0D 0xBB

# Set CH1 (IR) to 0xDDCC
i2cset -y 10 0x39 0x0E 0xCC
i2cset -y 10 0x39 0x0F 0xDD
```

---

## Sysfs Interface Usage

```sh
# View device
ls /sys/bus/iio/devices/iio:device0/

# Read sensor values
cat /sys/bus/iio/devices/iio:device0/in_illuminance_broadband_raw
cat /sys/bus/iio/devices/iio:device0/in_illuminance_ir_raw

# Adjust configuration
echo 16 > /sys/bus/iio/devices/iio:device0/in_illuminance_gain
echo 402 > /sys/bus/iio/devices/iio:device0/in_illuminance_integration_time
```

---

## Completion Criteria

* Driver loads cleanly and registers with IIO
* Valid data can be read from IR and broadband channels
* Gain and integration time can be read and set via sysfs
* Behavior confirmed via simulated I2C register values

---

## Resources

* [TSL2561 Datasheet (PDF)](https://cdn-shop.adafruit.com/datasheets/TSL2561.pdf)
* Linux kernel: `drivers/iio/light/` (for reference)

