A project given by ChatGPT

# Timed Logger Kernel Module

## Overview  
This project implements a **Linux kernel module that logs system uptime every X seconds**, configurable via `sysfs`.  
- Uses **`hrtimer`** for high-resolution periodic execution.  
- Uses a **workqueue** to safely print logs outside of timer context.  
- Allows dynamic interval changes via `/sys/module/timed_logger/parameters/interval_sec`.  

---

## Project Requirements  

### 1. Kernel Timer for Periodic Logging  
- Uses **`hrtimer`** to execute periodically.  
- Reschedules itself after logging uptime.  
- Stops when the module is removed.  

### 2. Workqueue for Deferred Execution  
- Timer callback queues work to a **workqueue**.  
- Workqueue function logs uptime using `pr_info()`.  

### 3. Sysfs Interface for Configurable Interval  
- Users can modify the logging interval via:  
  ```
  /sys/module/timed_logger/parameters/interval_sec
  ```
- Interval changes should take effect **immediately**.  

### 4. Proper Resource Management  
- **Synchronizes access** to prevent race conditions.  
- **Frees resources properly** on `rmmod`.  

---

## Expected Behavior  

### 1. Default Behavior  
- On `insmod`, logs system uptime **every 5 seconds** (default).  
- Messages appear in `dmesg`.  

### 2. Dynamic Interval Adjustment  
- Writing to `/sys/module/timed_logger/parameters/interval_sec` should update the interval immediately.  

### 3. Proper Cleanup  
- On `rmmod`, the **timer and workqueue should be properly deallocated**.  

---

## Testing  

### 1. Load the Module with Default Interval (5s)  
```sh
sudo insmod timed_logger.ko interval_sec=5  
```
- Verify logs appear every 5 seconds:  
```sh
dmesg | tail -n 10  
```

### 2. Change the Logging Interval to 10 Seconds  
```sh
echo 10 | sudo tee /sys/module/timed_logger/parameters/interval_sec  
```
- Verify logs now appear **every 10 seconds**.  

### 3. Ensure It Handles Rapid Interval Changes  
```sh
echo 1 | sudo tee /sys/module/timed_logger/parameters/interval_sec  
sleep 3  
echo 8 | sudo tee /sys/module/timed_logger/parameters/interval_sec  
```
- Logs should correctly follow new intervals.  

### 4. Remove the Module and Ensure Proper Cleanup  
```sh
sudo rmmod timed_logger  
```
- Verify no further log messages appear in `dmesg`.  
- Check `lsmod` to confirm unloading.  
