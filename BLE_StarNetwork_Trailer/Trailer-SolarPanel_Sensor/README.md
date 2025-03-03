# PIC32CXBZ2_WBZ45x_BLE_Multi-Role_Multi-Link_Demo

<img src="Docs/IoT-Made-Easy-Logo.png" width=100>


> "IOT Made Easy!" 

Devices: **| PIC32CXBZ2 | WBZ45x |**<br>
Features: **| BLE | MODBUS |**


## ⚠ Disclaimer

<p><span style="color:red"><b>
THE SOFTWARE ARE PROVIDED "AS IS" AND GIVE A PATH FOR SELF-SUPPORT AND SELF-MAINTENANCE. This repository contains example code intended to help accelerate client product development. </br>

For additional Microchip repos, see: <a href="https://github.com/Microchip-MPLAB-Harmony" target="_blank">https://github.com/Microchip-MPLAB-Harmony</a>

Checkout the <a href="https://microchipsupport.force.com/s/" target="_blank">Technical support portal</a> to access our knowledge base, community forums or submit support ticket requests.
</span></p></b>

## Contents

1. [Introduction](#step1)
1. [Bill of materials](#step2)
1. [Software Setup](#step3)
1. [Harmony MCC Configuration](#step4) 
1. [Board Programming](#step5)
1. [Run the demo](#step6)
1. [Related Applications](#step7)

## 1. Introduction<a name="step1">

This example application demonstrates the Multi-Role Multi-Link application using WBZ451 Curiosity board.
![](Docs/setup.png)

## 2. Bill of Materials<a name="step2">

| Tools | Quantity |
| :- | :- |
| [PIC32CX-BZ2 and WBZ451 Curiosity Development Board](https://www.microchip.com/en-us/development-tool/EV96B94A) | 8 |


## 3. Software Setup<a name="step4">

- [MPLAB X IDE ](https://www.microchip.com/en-us/tools-resources/develop/mplab-x-ide#tabs)

    - Version: 6.20
	- XC32 Compiler v4.10
	- MPLAB® Code Configurator v5.1.17
	- PIC32CX-BZ_DFP v1.0.107
	- MCC Harmony
	  - csp version: v3.14.0
	  - core version: v3.11.1
	  - CMSIS-FreeRTOS: v10.4.6
	  - wireless_pic32cxbz_wbz: v1.1.0
	  - wireless_ble: v1.0.0	  
	  - dev_packs: v3.14.0
	  - wolfssl version: v5.4.0
	  - crypto version: v3.7.6
	    
- Any Serial Terminal application like [TERA TERM](https://download.cnet.com/Tera-Term/3000-2094_4-75766675.html) terminal application

- [MPLAB X IPE v6.20](https://microchipdeveloper.com/ipe:installation)

## 4. Harmony MCC Configuration<a name="step5">

### Getting Started with Modbus Server application with the WBZ451 Curiosity Board

| Tip | New users of MPLAB Code Configurator are recommended to go through the [overview](https://onlineDocs.microchip.com/pr/GUID-1F7007B8-9A46-4D03-AEED-650357BA760D-en-US-6/index.html?GUID-AFAB9227-B10C-4FAE-9785-98474664B50A) |
| :- | :- |

**Step 1** - Connect the WBZ451 Curiosity board to the device/system using a micro-USB cable.

**Step 2** - This application is built by using [BLE Sensor Application](https://github.com/Microchip-MPLAB-Harmony/wireless_apps_pic32cxbz2_wbz45/tree/master/apps/ble/advanced_applications/ble_sensor) as the building block. The project graph of the BLE Sensor application is shown below.

| Note | The BLE Sensor application repository can be cloned/downloaded from this [link](https://github.com/Microchip-MPLAB-Harmony/wireless_apps_pic32cxbz2_wbz45). |
| :- | :- |
| File path | wireless_apps_pic32cxbz2_wbz45/apps/ble/advanced_applications/ble_sensor/ firmware/ ble_sensor.x |

![](Docs/Project_graph.png)

- In the project graph, select Transaparent profile and configure as follows.

![](Docs/Transparent_profile.png)

- In the project graph, select BLE Stack and configure as follows.

![](Docs/Ble_stack.png)

- In the project graph, select Device Information Service and configure as follows.

![](Docs/dis.png)

**Step 3** - [Generate](https://onlinedocs.microchip.com/pr/GUID-1F7007B8-9A46-4D03-AEED-650357BA760D-en-US-6/index.html?GUID-2EE03524-41FE-4EBA-8646-6D10AA72F365) the code.

**Step 4** - Once generation is complete, the merge window will appear. Merge all the changes shown.  

**Step 5** -  Copy the mentioned files from this repository by navigating to the location mentioned below and replace the generated files.
 
| Note | This application repository should be cloned/downloaded to perform the following steps. |
| :- | :- |
| Path | The application folder can be found in the following [link](https://github.com/MicrochipTech/PIC32CXBZ2_WBZ45x_BLE_UART_MODBUS) |

- Copy the following files from the cloned repo(...\firmware\src).
	- "app.c" and "app.h",
	- "app_ble_sensor.c" and "app_ble_sensor.h",
	- "app_ble_conn_handler.c" and "app_ble_conn_handler.h"
	- "app_trpc.c" and "app_trpc.h"
	- "app_trps.c" and "app_trps.h"
- Replace the above mentioned files in your project folder location(...\firmware\src).
- Copy the "app_trspc_handler.c","app_trspc_handler.h","app_trsps_handler.c" and "app_trsps_handler.h"files, which can be found by navigating to the cloned repo path: "...\src\app_ble".
- Replace these files in your project folder location(...\firmware\src\app_ble).

- Following the below mentioned steps please add the "app_trpc.c" and "app_trpc.h" to th Source files and header files respectively.

#### To add the folder to your MPLAB project
- In Projects section, right click on Source files to add the ".c" file and right click on Header files to add the ".h" file.
- Select "Add existing items from folder".
- Select Add and browse the location of the folder (...\firmware\src).
- Make sure the "Files of type" is "C Source files" while adding ".c" files and "Header files" while adding ".h" files.
- Select the folder and click "add".

**Step 6** - In "app_user_edits.c", make sure the below code line is commented 

- "#error User action required - manually edit files as described here".

**Step 7** - From projects, go to "app_adv.h" file and add this code in line 104.

```
#define APP_ADV_PROD_TYPE_BLE_SENSOR_MRML                                           0x04          /**< Product Type: BLE Sesnor */
```

**Step 8** - Clean and build the project. To run the project, select "Make and program device" button.

## 6. Board Programming<a name="step6">

## Programming hex file:

### Program the precompiled hex file using MPLAB X IPE

- The Precompiled hex file is given in the hex folder.
Follow the steps provided in the link to [program the precompiled hex file](https://microchipdeveloper.com/ipe:programming-device) using MPLABX IPE to program the pre-compiled hex image. 

### Build and program the application using MPLAB X IDE

The application folder can be found by navigating to the following path: 

- ".../firmware/"

Follow the steps provided in the link to [Build and program the application](https://github.com/Microchip-MPLAB-Harmony/wireless_apps_pic32cxbz2_wbz45/tree/master/apps/ble/advanced_applications/ble_sensor#build-and-program-the-application-guid-3d55fb8a-5995-439d-bcd6-deae7e8e78ad-section).

## 7. Run the demo<a name="step7">



## 8. Related applications<a name="step8">

- [BLE Applications](https://github.com/Microchip-MPLAB-Harmony/wireless_apps_pic32cxbz2_wbz45/tree/master/apps/ble)
- [WBZ45x BLE UART WITH E-PAPER DISPLAY](https://github.com/MicrochipTech/PIC32CXBZ2_WBZ45x_BLE_UART_E_PAPER_Display/blob/main/WBZ451_E_PAPER_BLE_UART)
- [WBZ45x BLE Sensor Multi-Link Multi-Role Demo](https://github.com/MicrochipTech/PIC32CXBZ2_WBZ45x_BLE_SENSOR_Multi-Role)
