<p align="center" width="100%">
    <img width="75%" src="on_air_sign.jpeg" alt="On Air Sign">
</p>

This project subscribes to PulseAudio source events. As soon as an audio application starts recording from a specified audio source, an action is taken. 

In my particular use-case, a "On Air" indicator sign is switched on so the user knows the microphone is active. I am using [usbrelay](https://github.com/darrylb123/usbrelay) to achieve this. Of course, the source code can be adapted to execute arbitrary functions.

### Dependencies:

* `libhidapi-dev`
* `libpulse-dev`

These refer to Ubuntu 22.04. Adapt to your environment as needed.

### Attributions:

"On Air" sign by [freepik](https://www.freepik.com/free-vector/realistic-red-air-sign_11187754.htm).
