# Linux Kernel Watchdog Timer Module

## Project Overview
This project involves developing a Linux kernel module that implements a **watchdog timer**. A watchdog timer is a safety mechanism used to detect system hangs and automatically reset the system if it becomes unresponsive. The module will use **high-resolution timers (hrtimers)** and expose both **sysfs** and **character device** interfaces to interact with user space.

## Requirements

### 1. High-Resolution Timer
- Use **hrtimers** to create a periodic timer that serves as the watchdog.
- The timer should reset periodically unless explicitly disabled.
- If the timer expires without being reset, it should simulate a system failure (e.g., logging an error message).

### 2. Sysfs Interface (`/sys/class/watchdog_timer/`)
- **`enabled`** (Boolean)
  - Writing `1` enables the watchdog timer.
  - Writing `0` disables it.
- **`timeout`** (Integer, seconds)
  - Determines how often the watchdog timer must be fed before it expires.
  - Writing a new value changes the timer interval.

### 3. Character Device Interface (`/dev/watchdog_timer`)
- **Feeding the watchdog**
  - Writing any data to the device resets the timer.
- **Triggering failure on close**
  - If the character device is opened but not written to, closing it should simulate a system failure.
- **Permissions**
  - Ensure proper file permissions so only privileged users can interact with the device.

### 4. Proper Synchronization
- Use a **mutex** to protect shared resources such as the timer and configuration settings.
- Use **atomic variables** where necessary to ensure efficient updates.
- Ensure correct cleanup in `rmmod`, including `del_timer_sync()`.

## Expected Behavior
1. **Module Loading (`insmod`)**
   - Creates the `/sys/class/watchdog_timer/` directory with `enabled` and `timeout` attributes.
   - Registers a character device at `/dev/watchdog_timer`.
   - Initializes the high-resolution timer but keeps it disabled by default.

2. **Enabling the Watchdog**
   - Writing `1` to `/sys/class/watchdog_timer/enabled` starts the watchdog timer.
   - The timer resets periodically.

3. **Feeding the Watchdog**
   - Writing any data to `/dev/watchdog_timer` resets the timer.

4. **Triggering a Timeout**
   - If `/dev/watchdog_timer` is opened but not written to, closing it triggers a system failure.

5. **Disabling the Watchdog**
   - Writing `0` to `/sys/class/watchdog_timer/enabled` stops the timer.

## Testing Code
The following commands can be used to test the module:

```sh
# Load the module
sudo insmod watchdog_timer.ko

# Enable the watchdog
echo 1 | sudo tee /sys/class/watchdog_timer/enabled

# Set timeout to 5 seconds
echo 5 | sudo tee /sys/class/watchdog_timer/timeout

# Open and feed the watchdog
echo "reset" > /dev/watchdog_timer

# Stop feeding the watchdog and trigger failure
sleep 6
cat /dev/watchdog_timer # Should log a failure event

# Disable the watchdog
echo 0 | sudo tee /sys/class/watchdog_timer/enabled

# Remove the module
sudo rmmod watchdog_timer
```

## Additional Considerations
- Ensure the module follows proper kernel coding standards (`checkpatch.pl`).
- Implement appropriate error handling for device registration and sysfs attribute creation.
- Properly unregister sysfs and cdev interfaces during module removal.

This project will reinforce concepts of **hrtimers, sysfs, character devices, synchronization, and proper kernel module structure**.


