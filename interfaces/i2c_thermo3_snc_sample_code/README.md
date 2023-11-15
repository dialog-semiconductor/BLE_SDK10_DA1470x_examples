# I2C Adapter Demonstration Example In SNC/SYSCPU Context

## Example Description

This example demonstrates using the I2C adapter layer employing the I2C1 block in master mode. The DA1470x family of devices comprises three separate I2C blocks instances. All I2C operations are executed in the SNC context using the DG (dialog) co-operative scheduling. In this scheduler, a single-stack environment is used for all co-routines in contrast to the FreeRTOS environment where each task is assigned a separate stack instance. This imposes few restrictions when composing the co-routines; relevant comments can be found throughout the source code. In this demonstration the `THERMO3` click board, which is I2C- and microBUS-compatible is utilized. The board comes with the `TMP102` temperature sensor which can trigger an external signal once the temperature exceeds or falls within a configurable temperature range. Typically, the alert signal is connected to the WKUP controller which is triggered once the signal is asserted. To keep it simple, the alert functionality is not demonstrated in this example. Instead, a single task is created which is triggered with the help of a OS timer and performs a single temperature read operation.  All values read are pushed into a shared buffer and once a predefined number of bytes is received the remote master (SYSCPU) is notified to further process them. All three supported Inter-Process Communication (IPC) schemes are demonstrated, that is a simple SNC2SYS interrupt scheme, the mailbox service and the RPMsg-Lite framework.

## HW and SW configuration

### Hardware configuration

- This example runs on DA1470x Bluetooth Smart SoC devices. One DA1470x Pro Development kit and one THERMO3 click board are needed for this example. For more information on the temperature sensor demonstrated click [here]( https://www.mikroe.com/thermo-3-click).
- Connect the development kit to the host computer via the USB1 connector mounted on the motherboard and open a serial terminal. Two serial ports should be displayed; select the one with the lower number and then configure the terminal as depicted in the following table.

|  Setting  | Value  |
| :-------: | :----: |
| Baudrate  | 115200 |
| Data bits |   8    |
| Stop bits |   1    |
|  Parity   |  None  |

- Connect the click board to either mikroBUS #2 or mikroBUS #1. It's a prerequisite that the expansion headers have been soldered before attaching the click board.

### Software configuration

- Recommended to employ the latest SmartSnippets Studio version released.
- Download the latest SDK version
- SEGGER J-Link tools should be downloaded and installed.

## How to run the example

### The RAM build configurations should be used for debugging purposes only

### Initial Setup

- Download the source code from the Support Website.

- Import the `i2c_thermo3_m33_sample_code` and `i2c_thermo3_snc_sample_code` projects into your workspace (there are no path dependencies).

- The default IPC scheme is the mailbox service. Users can select any of the three available IPC schemes by setting the corresponding configuration macros in the `interface\app_common.h` file.  There are three valid combinations:

  - Simple SNC2SYS interrupt scheme:

  ```c
  #define dg_configUSE_MAILBOX            ( 0 )
  #define dg_configUSE_RPMSG_LITE         ( 0 )
  ```

  - Mailbox service

  ```c
  #define dg_configUSE_MAILBOX             ( 1 )
  #define dg_configUSE_RPMSG_LITE          ( 0 )
  ```

  - RPMSG-Lite framework

  ```c
  #define dg_configUSE_MAILBOX             ( 0 )
  #define dg_configUSE_RPMSG_LITE          ( 1 )
  ```

- The demonstration example is composed of two projects; one project compiled and run on the SYSCPU master and one project compiled and run on the SNC master.

  - Open the `sdk\mailbox\include\mailbox.h` file of either project and modify the `MAILBOX_INT_MAIN` and `MAILBOX_INT_SNC` enumeration structures as follow:

    ```c
    typedef enum {
            /* RPMsg-Lite mailbox interrupt */
    #if dg_configUSE_RPMSG_LITE
            MAILBOX_INT_SNC_RPMSG_LITE,
    #endif /* dg_configUSE_RPMSG_LITE */
    
            /* Add more mailbox interrupts */
            MAILBOX_INT_SNC_APP,                    /* Index associated to the shared space that hosts the raw data */
    
            MAILBOX_INT_SNC_MAX,                    /* Must not exceed 32 */
    } MAILBOX_INT_SNC;
    ```

    ```c
    typedef enum {
            /* RPMsg-Lite mailbox interrupt */
    #if dg_configUSE_RPMSG_LITE
            MAILBOX_INT_MAIN_RPMSG_LITE,
    #endif /* dg_configUSE_RPMSG_LITE */
    
            /* Add more mailbox interrupts */
            MAILBOX_INT_MAIN_APP,                   /* Index associated to the shared space that hosts the raw data */
    
            MAILBOX_INT_MAIN_MAX,                   /* Must not exceed 32 */
    } MAILBOX_INT_MAIN;
    ```

  - Go to the SNC project and select the release build configuration. Once the the project is built go to the build folder, that should be `DA1470x-00-SNC-I2C-Release` and copy the `snc_files/snc_fw_embed.h`  file. This file contains the SNC binary along with symbols that reflect objects accessible by both the SNC and SYSCPU masters.

  - Go to the SYSCPU project, and paste the previously copied file in the `snc` folder. Then build the project by selecting either the OQSPI debug or release build configuration. To facilitate users, the SNC binary file is already copied under the mentioned folder.

- Download the firmware image into the XiP (eXecution-in Place) flash memory used by selecting the `program_oqspi_jtag` or  `program_oqspi_serial` launcher. Read the `Console` window of the Eclipse environment in case more steps are required to be executed.

  - The SYSCPU project is compiled with the flash memory autodetect feature enabled meaning that any supported OQSPI flash memory can be used without modifying the custom configuration file, that is `custom_config_oqspi.h`.

    ```c
    #define dg_configFLASH_AUTODETECT               ( 1 )
    #define dg_configOQSPI_FLASH_AUTODETECT         ( 1 )
    ```

- Press the `B1(RSTn)` push button mounted on the daughterboard, so the device starts executing its firmware.

- A series of messages should be displayed on the terminal; In this example both SYSCPU and SNC masters print log messages using retarget operations. To distinguish which master prints which message,  the corresponding master name is appended before a retarget message, that is `[SNC]:` or `[M33]:`. This message extension is performed automatically by the retarget feature. Here is the message sequence:

  1. The SNC context performs temperature read operations over the I2C bus and pushes the raw values into a shared buffer:
  
     > [SNC]: Data pushed into the shared space...
     >
     > ...

  2. Once a predefined number of chunk of bytes have been pushed into the shared space the SYSCPU master gets notified:
  
     > [SNC]: 4 chunks are occupied; time to notify the remote master...
  
  3. The remote master (SYSPCU) gets notified, access the shared memory space, coverts the raw values to temperature values and prints them on the serial console:
  
     > [M33]: Successfully retrieved [2] bytes from the shared space.
  
     > [M33]: Timestamp: 0x1964
     >
     > [M33]: Temperature: +25.8
     >
     > ...
     >

## Known Limitations

There are no known limitations for this example.