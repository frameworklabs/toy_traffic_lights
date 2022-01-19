# toy_traffic_lights

This is a simple toy app build from M5Stack microcontrollers and LED units attached to LEGO bricks to simmulate traffic lights.

It uses an activity oriented programming model to simplify concurrent programming and mode switching.

The traffic light currently supports 3 modes:

- Day Mode: Semaphores for cars and pedestrians alternate between green, yellow and red. A single press on the button will shorten the red phase for pedestrians.
- Night Mode: Both semaphores blink yellow. Activated on double-press.
- Off Mode: All lights are off but rotating the potentiometer will enable to set the brightness of all lights. Activated on long-press.
