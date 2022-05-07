# Mechanical Shutter System

## High-level Description

This project aims to build a simple mechanical system, controlled via microcontroller, to operate the shutter on an analog camera. The mechanical operation is performed by a solenoid that is microcontrolled to press the the shutter button set in "Bulb" mode for an amount of time as determined by a photoresistor. The shutter speed is determined by the MCUs ADC peripheral with light intensity as the analog signal. The MCU used is and ARM Cortex M0 equipped STM32F072B board by STMicroelectronics.

## Circuit Schematic

Parts needed:

- STM32F072B Discovery board
- 12V push-type solenoid
- 12V 2A DC power supply
- IRLZ44NPBF N-type MOSFET
- 1kOhm and 10kOhm resistors
- Photoresistor
- D1N007 diode
- Breadboard
- Jumper wires

Below is the circuit diagram:

![Circuit Schematic](/images/schematic.PNG)

The flyback diode in parallel with the solenoid is used to prevent backwards voltage at power-off to prevent damage to components. The resistors are used as pull-down resistors to prevent floating signals.

Here is the assembled circuit:

![Assembled Circuit](/images/photo_circuit.jpg)

## Basic Operation

The embedded environment can be set up using ARM development tools. For this project I used STM32CubeMX and Keil uVision5. The main.c file uses HAL libraries so once the the project has been successfully set up on your own machine, substitute the main.c file in the repo into the proper location and build the project onto the board. The solenoid is operated by the push button on the board. When the button is pushed, light data will be picked up by the photoresistor and used to determine the amount of time that the solenoid will be actively pushe the shutter on the camera. It is important to have the camera in "Bulb" setting so that the shutter is open for the duration that the solenoid is extended.  