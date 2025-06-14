# Linux Kernel BMP280 Sensor Driver Module

## Project Overview

This project involves creating a (bare metal) Linux kernel module that acts as a driver for the **BMP280** sensor (I2C variant). The driver will communicate with the BMP280 over the I2C bus on a Raspberry Pi 3 running Raspberry Pi OS. The module will read temperature and pressure data from the sensor and expose them to user space through the **sysfs** interface.

## Requirements

### 1. I2C Integration

* Use Linux kernel I2C APIs to communicate with the BMP280 sensor.
* Properly detect and register the BMP280 device on the I2C bus.
* Define a compatible `i2c_device_id` and optionally a `device_tree` entry.

### 2. Reading Sensor Data

* Periodically read temperature and pressure data from the BMP280 sensor registers.
* Apply the calibration algorithm as defined in the BMP280 datasheet.

### 3. Sysfs Interface

Expose the following attributes in `/sys/class/bmp280/`:

* **`temperature`**: Read-only; returns temperature in millidegrees Celsius.
* **`pressure`**: Read-only; returns pressure in Pascals.
* **`poll_interval`**: Read-write; controls how often (in ms) to read data from the sensor.

### 4. Device Tree Support (Optional)

* Support loading the driver via devicetree overlay if needed.
* Document expected `compatible` string and configuration.

### 5. Synchronization and Cleanup

* Use proper synchronization when accessing shared data (e.g., mutex).
* Clean up sysfs and unregister from the I2C subsystem during module exit.

## Expected Behavior

1. **Module Load (`insmod`)**

   * Registers as an I2C driver.
   * Detects the BMP280 on the I2C bus.
   * Creates a sysfs entry under `/sys/class/bmp280/`.

2. **Sensor Polling**

   * Reads sensor data periodically.
   * Converts raw data to human-readable values using calibration data.

3. **User Space Access**

   * Users can read temperature and pressure from sysfs.
   * Users can adjust `poll_interval` to change update frequency.

4. **Module Unload (`rmmod`)**

   * Stops sensor polling.
   * Cleans up sysfs entries and unregisters the I2C device.

## Testing Commands

```sh
# Load the module
sudo insmod bmp280_driver.ko

# View temperature and pressure
cat /sys/class/bmp280/temperature
cat /sys/class/bmp280/pressure

# Change polling interval
echo 1000 | sudo tee /sys/class/bmp280/poll_interval

# Unload the module
sudo rmmod bmp280_driver
```

## Additional Considerations

* Follow proper kernel coding style (`checkpatch.pl`).
* Ensure correct handling of I2C errors and edge cases.
* Add meaningful `pr_info()` and `pr_err()` logging to aid debugging.

This project deepens understanding of **I2C communication, kernel I2C APIs, sysfs, device registration, synchronization, and sensor data handling**.

