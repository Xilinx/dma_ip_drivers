# Xilinx QDMA Driver (Windows)

This project contains the driver and user-space software for the Xilinx PCIe Multi-Queue DMA Subsystem IP.

_____________________________________________________________________________
Contents

1.   Directory Structure
2.   Dependencies
3.   Building from Source
     3.1   From Visual Studio
     3.2   From Command Line
4.   Driver Installation
5.   Test Utilities
     5.1 dma-arw
     5.2 dma-rw
     5.3 dma-ctl
6.   Known Issues
7.   References
_____________________________________________________________________________


1.  Directory Structure:

    <project_root>/
    |__ build/                - Generated directory containing build output binaries.
    |__ apps/                 - Contains application source code which demonstrates the usage of QDMA driver.
    |  |__ dma-arw/           - Asynchrnous Read/Write PCIe BARs of QDMA IP or perform tx/rx DMA transfers.
    |  |__ dma-rw/            - Synchronous Read/Write PCIe BARs of QDMA IP or perform tx/rx DMA transfers.
    |  |__ dma-ctl/           - DMA queues control and configuration for DMA transfers.
    |__ docs/                 - QDMA documentation
    |__ sys/                  - Contains the QDMA driver source code.
    |__ README.md             - This file.
    |__ QDMA.sln              - Visual Studio Solution.

2.  Dependencies

    * Windows 10 target machine
    * Windows 10 development machine
    * Windows Driver Development Kit 10.0.17134.0 (or later)
    * Visual Studio 2017

3.  Building from Source

    3.1 From Visual Studio
    ----------------------

        The Windows driver and sample applications can be built using Visual Studio.
        1. Open the *QDMA.sln* solution.
        2. Select the appropriate *Build Configuration* (Debug/Release) from the menu bar.
        2. Click *Build* from the menu bar.
        3. Click *Build Solution*.

        This driver project settings currently support 64bit Windows 10 OS.
        In order to target a different Windows OS, go to the driver project *Properties->Driver Settings->General* and change *Target OS Version* to the desired OS.

        Also *Release* and *Debug* build configurations exist and are configurable via the *Configuration Manager*.

        The compiled build products can then be found in the *build/x64/`CONFIG`/* folder. This folder contains three folders:
        - *bin/* contains sample and test applications
        - *libqdma/* contains libqdma static library
        - *sys/* contains the driver

    3.2 From Command Line
    ---------------------

        1. Open *Developer Command Prompt for VS2017*
        2. Change directory to the project root directory
        3. Run the following command:

                msbuild /t:clean /t:build QDMA.sln

        4. The build should run and display the following

                Build succeeded.
                    0 Warning(s)
                    0 Error(s)

                Time Elapsed 00:01:05.55

        For more information on building Windows drivers visit the [MSDN website][ref4].

4.  Driver Installation

    The easiest way to install the driver is via Windows' *Device Manager*
    (_Control Panel->System->Device Manager_).

    _**Note**: The driver does not provide a certified signature and uses a test signature instead.
    Please be aware that, depending on your target operating system, you may need to enable test-signed drivers in your windows boot configuration
    in order to enable installation of this driver. See [MSDN website][ref5] for further information._

    1. Open the *Device Manager*
    2. Initially the device will be displayed as a *PCI Serial Port* or *PCI Memory Controller*.
    3. Right-Click on the device and select *Update Driver Software* and
       select the folder of the built QDMA driver (typically *build/x64/`CONFIG`/sys/QDMA/* (where `CONFIG` is *Debug* or *Release*).
    4. If prompted about unverified driver publisher, select *Install this driver software anyway*.
    5. *Xilinx Drivers -> Xilinx PCIe Multi-Queue DMA* should now be visible in the *Device Manager*

5.  Test Utilities

    The Xilinx dma-arw and dma-rw are test utilities can perform the following functions

    AXI-MM
        - H2C/C2H AXI-MM transfer.

    AXI-ST-H2C
        - Enables the user to perform AXI-ST H2C transfers and checks data for correctness.

    AXI-ST-C2H
        - Enables the user to perform AXI-ST C2H transfers and checks data for correctness.

    register access
        - read a register space
        - write to a register space
    5.1 dma-arw
    -----------
        e.g.
        1. Get the dma-arw help
            > dma-arw -h
                dma-arw usage:
                dma-arw -v  : prints the version information

                dma-arw qdma<N> mode <0 | 1> <DEVNODE> <read|write> <ADDR> [OPTIONS] [DATA]

                - qdma<N>   : unique qdma device name (<N> is BBDDF where BB -> PCI Bus No, DD -> PCI Dev No, F -> PCI Fun No)
                - mode      : 0 : this mode uses ReadFile and WriteFile async implementation
                            : 1 : this mode uses ReadFileEx and WriteFileEx async implementation
                - DEVNODE   : One of: queue_mm_ | queue_st_*
                              where the * is a numeric wildcard (0 - 511for queue).
                - ADDR      : The target offset address of the read/write operation.
                              Applicable only for control, user, queue_mm device nodes.
                              Can be in hex or decimal.
                - OPTIONS   :
                              -a set alignment requirement for host-side buffer (default: PAGE_SIZE)
                              -b open file as binary
                              -f use contents of file as input or write output into file.
                              -l length of data to read/write (default: 4 bytes or whole file if '-f' flag is used)
                - DATA      : Space separated bytes (big endian) in decimal or hex,
                              e.g.: 17 34 51 68
                              or:   0x11 0x22 0x33 0x44
        2. Read/Write four bytes from AXI-MM at zeroth offset on queue zero of PF 0
            > dma-arw qdma17000 mode 0 queue_mm_0 read 0 -l 4
            > dma-arw qdma17000 mode 0 queue_mm_0 write 0 0xA 0xB 0XC 0xD
            > dma-arw qdma17000 mode 1 queue_mm_0 read 0 -l 4
            > dma-arw qdma17000 mode 1 queue_mm_0 write 0 0xA 0xB 0XC 0xD

        3. Read four bytes from AXI-ST-H2C on queue zero
            > dma-arw qdma17000 mode 0 queue_st_0 read -l 4
            > dma-arw qdma17000 mode 1 queue_st_0 read -l 4

        4. Write four bytes to AXI-ST-C2H on queue zero
            > dma-arw qdma17000 mode 0 queue_st_0 write -l 4
            > dma-arw qdma17000 mode 1 queue_st_0 write -l 4

        5. Read/Write four bytes from control register space at zeroth offset
            > dma-arw qdma17000 mode 0 control read 0 -l 4
            > dma-arw qdma17000 mode 0 control write 0 0xA 0xB 0XC 0xD
            > dma-arw qdma17000 mode 1 control read 0 -l 4
            > dma-arw qdma17000 mode 1 control write 0 0xA 0xB 0XC 0xD

    5.2 dma-rw
    -----------
        e.g.
        1. Get the dma-rw help
            > dma-rw -h
                dma-rw usage:
                dma-rw -v  : prints the version information

                dma-rw qdma<N> <DEVNODE> <read|write> <ADDR> [OPTIONS] [DATA]

                - qdma<N>   : unique qdma device name (<N> is BBDDF where BB -> PCI Bus No, DD -> PCI Dev No, F -> PCI Fun No)
                - DEVNODE   : One of: control | user | queue_mm_ | queue_st_*
                              where the * is a numeric wildcard (0 - 511for queue).
                - ADDR      : The target offset address of the read/write operation.
                              Applicable only for control, user, queue_mm device nodes.
                              Can be in hex or decimal.
                - OPTIONS   :
                              -a set alignment requirement for host-side buffer (default: PAGE_SIZE)
                              -b open file as binary
                              -f use contents of file as input or write output into file.
                              -l length of data to read/write (default: 4 bytes or whole file if '-f' flag is used)
                - DATA      : Space separated bytes (big endian) in decimal or hex,
                              e.g.: 17 34 51 68
                              or:   0x11 0x22 0x33 0x44
        2. Read/Write four bytes from AXI-MM at zeroth offset on queue zero of PF 0
            > dma-rw qdma17000 queue_mm_0 read 0 -l 4
            > dma-rw qdma17000 queue_mm_0 write 0 0xA 0xB 0XC 0xD

        3. Read four bytes from AXI-ST-H2C on queue zero
            > dma-rw qdma17000 queue_st_0 read -l 4

        4. Write four bytes to AXI-ST-C2H on queue zero
            > dma-rw qdma17000 queue_st_0 write -l 4

        5. Read/Write four bytes from control register space at zeroth offset
            > dma-rw qdma17000 control read 0 -l 4
            > dma-rw qdma17000 control write 0 0xA 0xB 0XC 0xD

    5.3 dma-ctl
    ------------
        The Xilinx dma-ctl is a helper utility can perform the following functions for a given PF

        Add/Start/Stop/Delete/State queues
            - can add and start a queue
            - can stop and delete a queue
            - can retrieve and show the state of a queue

        Dump the CSR registers Information
            - List all the CSR registers and its values

        Provide the HW and SW capabilities Information

        e.g.:
        1. Get the dma-ctl help
            > dma-ctl -h
                usage: dma-ctl [dev | qdma<N>] [operation]
                       dma-ctl -h - Prints this help
                       dma-ctl -v - Prints the version information

                dev [operation] : system wide FPGA operations
                     list       : list all qdma functions

                qdma<N> [operation]     : per QDMA FPGA operations
                    <N>         : N is BBDDF where BB -> PCI Bus No, DD -> PCI Dev No, F -> PCI Fun No
                    csr <dump>
                         dump   : Dump QDMA CSR Information

                    devinfo     : lists the Hardware and Software version and capabilities

                    queue <add|start|stop|delete|listall>
                           add mode <mm|st> qid <N> [en_mm_cmpl] idx_h2c_ringsz <0:15> idx_c2h_ringsz <0:15>
                               [idx_c2h_timer <0:15>] [idx_c2h_th <0:15>] [idx_c2h_bufsz <0:15>] [cmptsz <0|1|2|3>] - add a queue
                               [trigmode <every|usr_cnt|usr|usr_tmr|usr_tmr_cnt>] [sw_desc_sz <3>] [desc_bypass_en] [pfch_en] [pfch_bypass_en]
                               [cmpl_ovf_dis]
                           start qid <N> - start a single queue
                           stop qid <N> - stop a single queue
                           delete qid <N> - delete a queue
                           state qid <N> - print the state of the queue
                           dump qid <N> dir <h2c|c2h> desc <start> <end> - dump desc ring entries <start> to <end>
                           dump qid <N> cmpt <start> <end> - dump completion ring entries <start> to <end>
                           dump qid <N> ctx dir <h2c|c2h> - dump context information of <qid>
                           cmpt_read qid <N> - Read the completion data
                    intring dump vector <N> <start_idx> <end_idx> - interrupt ring dump for vector number <N>
                                                                    for intrrupt entries :<start_idx> --- <end_idx>
                    reg dump - register dump

        2. Get the qdma devices list
            > dma-ctl dev list
                qdma17000	0000:17:00:0	maxQP: 512, 0~511
                qdma17001	0000:17:00:1	maxQP: 512, 512~1023
                qdma17002	0000:17:00:2	maxQP: 512, 1024~1535
                qdma17003	0000:17:00:3	maxQP: 512, 1536~2047


6.  Known Issues

    * Driver installation gives warning due to test signature.
    * Driver is not fully tuned to achieve maximum IP performance

7.  References

    [ref1]: https://www.xilinx.com/products/intellectual-property/pcie-dma.html
    [ref2]: https://www.xilinx.com/support/documentation/ip_documentation/qdma/v3_0/pg302-qdma.pdf
    [ref3]: https://developer.microsoft.com/en-us/windows/hardware/windows-driver-kit
    [ref4]: https://msdn.microsoft.com/en-us/windows/hardware/drivers/develop/building-a-driver
    [ref5]: https://msdn.microsoft.com/en-us/windows/hardware/drivers/install/the-testsigning-boot-configuration-option