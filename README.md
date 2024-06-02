esp32-motion
============

This is a simple script which can enable a Shelly using the [Gen 2 API](https://shelly-api-docs.shelly.cloud/gen2/). It first checks the state of the switch and when it is off or on a timer, it'll switch it on and restarts the timer.

It utilizes the `toggle_after` feature of the API which automatically toggles back after a certain amount of time.

With the check before, it is possible to switch the lights permanently on without this script interfering. Also multiple devices with this script can be used at the same time, as the countdown is handled by the Shelly device.

Configuration
-------------

To configure this script, rename/copy the `config.hpp.sample` to `config.hpp` and fill the values.