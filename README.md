## Avnet PSOC™ Edge DEEPCRAFT™ Baby Monitor

This demo project is the integration of Infineon's 
[PSOC&trade; Edge MCU: Machine learning – DEEPCRAFT™ deploy audio](https://github.com/Infineon/mtb-example-psoc-edge-ml-deepcraft-deploy-audio/tree/release-v2.1.0)
and [Avnet /IOTCONNECT ModusToolbox&trade; SDK](https://github.com/avnet-iotconnect/avnet-iotc-mtb-sdk). 

This code example demonstrates how to deploy a machine learning (ML) model to detect audio generated using DEEPCRAFT&trade; Studio on Infineon's PSOC&trade; Edge MCU.

This example deploys DEEPCRAFT&trade; Studio's baby cry detection starter model on the PSOC&trade; Edge MCU. This model uses data from the pulse-density modulation to detect whether a baby is crying or not.

This project has a three project structure: CM33 secure, CM33 non-secure, and CM55 projects. All three projects are programmed to the external QSPI flash and executed in Execute in Place (XIP) mode. Extended boot launches the CM33 secure project from a fixed location in the external flash, which then configures the protection settings and launches the CM33 non-secure application. Additionally, CM33 non-secure application enables CM55 CPU and launches the CM55 application.

The M55 processor performs the DEEPCRAFT™ model heavy lifting and reports the data via IPC to the M33 processor.
The M33 Non-Secure application is a custom /IOTCONNECT application that is receiving the IPC messages, 
processing the data and sending it to /IOTCONNECT. 
This application can receive Cloud-To-Device commands as well and control one of the board LEDs or control the application flow.    

## Requirements

- [ModusToolbox&trade;](https://www.infineon.com/modustoolbox) with MTB Tools v3.6 or later (tested with v3.6)
- Board support package (BSP) minimum required version: 1.0.0
- Programming language: C
- Associated parts: All [PSOC&trade; Edge MCU](https://www.infineon.com/products/microcontroller/32-bit-psoc-arm-cortex/32-bit-psoc-edge-arm) parts

## Supported toolchains (make variable 'TOOLCHAIN')

- GNU Arm&reg; Embedded Compiler v14.2.1 (`GCC_ARM`) – Default value of `TOOLCHAIN`

> **Note:**
> This code example fails to build in RELEASE mode with the GCC_ARM toolchain v14.2.1 as it does not recognize some of the Helium instructions of the CMSIS-DSP library.

## Supported kits (make variable 'TARGET')

- [PSOC&trade; Edge E84 AI Kit](https://www.infineon.com/KIT_PSE84_AI) (`KIT_PSE84_AI`) -
[Purchase Link](https://www.newark.com/infineon/kitpse84aitobo1/ai-eval-kit-32bit-arm-cortex-m55f/dp/49AM4459)
- [PSOC&trade; Edge E84 Evaluation Kit](https://www.infineon.com/KIT_PSE84_EVAL) (`KIT_PSE84_EVAL_EPC2`) -
[Purchase Link](https://www.newark.com/infineon/kitpse84evaltobo1/eval-kit-32bit-arm-cortex-m55f/dp/49AM4460)

## Hardware setup

This example uses the board's default configuration. 
See the kit user guide to ensure that the board is configured correctly.

Ensure the following jumper and pin configuration on board.
- BOOT SW must be in the HIGH/ON position
- J20 and J21 must be in the tristate/not connected (NC) position

> **Note:** This hardware setup is not required for KIT_PSE84_AI.

## Setup The Project

To setup the project, please refer to the 
[/IOTCONNECT ModusToolbox&trade; PSOC Edge Developer Guide](DEVELOPER_GUIDE.md)

## Running the Demo

- Once the board connects to /IOTCONNECT, 
it will start processing microphone input and attempt to detect the corresponding sound. 
This can be tested by placing the board in such way so that the microphone close to the PC speaker. Observe if the model detects and reports the correct baby cry sound.


- The following YouTube sound clips can be used for testing:
  * [Baby Cry](https://www.youtube.com/watch?v=Rwj1_eWltJQ&t=227s)


- After a few seconds, the device will begin sending telemetry packets similar to the example below:
```
>: {"d":[{"d":{"version":"1.1.1","random":32,"confidence":86,"class_id":2,"class":"baby_cry","event_detected":true}}]}
```
- The following commands can be sent to the device using the /IOTCONNECT Web UI:

    | Command                  | Argument Type     | Description                                                                                             |
    |:-------------------------|-------------------|:--------------------------------------------------------------------------------------------------------|
    | `board-user-led`         | String (on/off)   | Turn the board LED on or off  (Red LED on the EVK, Green on the AI)                                     |
    | `set-reporting-interval` | Number (eg. 2000) | Set telemetry reporting interval in milliseconds.  By default, the application will report every 2000ms |
