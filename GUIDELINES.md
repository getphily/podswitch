# PodSwitch General Guidance

## Video Output Standard (1080p30)

This software operates under the strict assumption that the final output resolution is **1920x1080 at 30 FPS (1080p30)**.

While you may connect higher-resolution video capture devices (e.g., 4K cameras), the **Scene Generator** algorithm is hardcoded to scale and map sources across a 1920x1080 canvas.

### Important:
For the highest quality and best performance, please ensure your OBS Video Settings are strictly configured as follows:
- **Base (Canvas) Resolution**: 1920x1080
- **Output (Scaled) Resolution**: 1920x1080
- **Common FPS Values**: 30

Failure to set your Base Canvas to 1920x1080 may result in the Scene Generator creating incorrectly scaled or improperly cropped scenes.
