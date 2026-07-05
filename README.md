# Welcome to P-Vision!
<img src=\Media\Pics\Title.png width="1000">

**P-Vision** is a graphics card for Amiga 600/1200 computers, which can be plugged into the PCMCIA port of your Amiga. It features a digital video output which can be directly connected to modern displays and television sets.

The design of P-Vision features all of the functions you expect from an Amiga graphics card, including 32-bit true color graphics, a programmable display processor handling multiple screen contexts and a fast 64-bit hardware accelerated BitBlt engine supporting copy, pattern fill and color expansion functions.

Designed for the PCMCIA port, you are free to use the **P-Vision** with your already existing hardware set-up, significantly extending the scope of applications with existing 68020/030/040/060 accelerator cards.

In order to make use of these functions, P-Vision is supported by the P96 RTG system, sold by Individual Computers.

### _P-Vision enables RTG for ALL Amiga 600/1200 users!_

## P-Vision Feature List

* Fast **16-bit PCMCIA interface**
	* 3.5 MB/s on Amiga 600
	* Up to 9.5 MB/s on Amiga 1200
* Digital video output interface
	* **8, 16 and 32-bit** color modes
	* Wide range of supported display resolutions **(12-85MHz)**, supporting **Full-HD** resolution
	* 64x64 hardware sprite
	* Pixel-wise smooth scrolling
	* Programmable **display processor** for flexible display context management and synchronisation
* 32 MB video memory
	* 200 MHz yielding up to **360 MB/s** total memory bandwidth
	* Hardware **memory paging** support for MMU equipped Amigas
	* **4 MB VRAM** for non-MMU equipped Amigas
* 64-bit hardware BitBlt engine
	* 90 MHz, up to **360 Mio pix/s**
	* Fast rectangular copy blits, up to 32767x32767 pixels in size
	* Logical function unit with pixel and color bit masking support
	* 16x16 mono pattern with color expansion & direct host interface for dithering and text rendering
* **Upgradable FPGA Core** via **JTAG**, or **Amiga-based flash tool**.
* **Open Hardware** - Gerbers and FPGA Bitstream <ins>freely available</ins> **after May 17th, 2027**.	

### _Minimum Amiga Configuration_
* Amiga 600
* 68EC020 @14MHz
* 1 MB Chip / 4 MB Fast
* OS 3.1
* P96 V3.3.0

### _Recommended Amiga Configuration_
* Amiga 1200
* 68030 @25Mhz
* 2 MB Chip / 16 MB Fast
* OS 3.9 or 3.2.x
* P96 V3.6.2
  
## P-Vision Parts & Function Overview
<img src=\Media\Pics\PCB_Outline.png width="1000">
The above diagram shows all relevant ports and components of the P-Vision card.

### _List of Components_

|**P-Vision User Manual**|
|-|
|[Download](https://github.com/necronomfive/AmiGUS-pub/tree/main/Documentation/P-Vision/P-Vision_User_Manual.pdf)|

|Part|Function| Documentation|
|-|-|-|
|**Altera 10M25 FPGA**| All image processing functions are implemented in this device. |[Register Map](https://github.com/necronomfive/P-Vision_pub/raw/main/Documentation/P-Vision/P-Vision_Register_Map.xlsx)|
|**32MB SDRAM**| Memory for all display output and graphics processing.  | [Datasheet](https://github.com/necronomfive/P-Vision_pub/tree/main/Documentation/Datasheets/42_45S83200J_16160J-462626.pdf)  |

### _List of ports_
|Port|Function|
|-|-|
|**PCMCIA**| Connects to either an Amiga 600 or Amiga 1200 computer. Please do note that this card does not support hot-plugging. |
|**Digital Video Output**| Connects to a modern digital monitor or television providing a high resolution, true color video output. |

## How to Build your P-Vision
The exclusive rights to manufacture and sell P-Vision PCBs has been exclusively granted to Alinea Computer for one year, ending on May 17th, 2027. During this time, you can purchase fully assembled P-Vision units or P-Vision PCBs for your own assembly. After expiration, the P-Vision PCB files will be released under Creative Commons license for everyone to build.

### _PCB Data_

### _Will be released on May 17th, 2027_

| File  | Description |
|-|-|
|[Download](https://github.com/necronomfive/AmiGUS-pub/raw/main/PCB/BOM/AmiGUS_BOM_Rev13.xlsx)| BOM for all components and Mouser references (if available) |

## P-Vision Software
### _FPGA Core Firmware_

Firmware release can be found [here.](https://github.com/necronomfive/P-Vision_pub/tree/main/FPGA/Releases/)

The [Quartus folder](https://github.com/necronomfive/P-Vision_pub/tree/main/FPGA/Releases/Quartus) contains .pof files for Altera's Quartus software.
These are needed for the initial bring-up of self-made P-Vision cards especially.

The [Amiga .upd files](https://github.com/necronomfive/P-Vision_pub/tree/main/FPGA/Releases/Amiga) can be flashed using the P-Vision Flash tool.

## Supported Resellers
<img src=\Media\Pics\alinealogo.png width="500">

## Links
Here you will find links to the software components needed to use the P-Vision
| Link  | Description |
| ------------- | ------------- |
| [P96 RTG System](https://icomp.de) | P96 RTG software.|
| [P-Vision Driver](https://aminet.net/driver/video/pVision.lha) | P-Vision driver for P96. |

## Licensing
### _P-Vision Printed Circuit Board_
[![CC BY-NC-ND 4.0][cc-by-nc-nd-shield]][cc-by-nc-nd]\
P-Vision PCB\
Copyright (C) 2026 by Oliver Achten

This work is licensed under a
[Creative Commons Attribution-NonCommercial-NoDerivs 4.0 International License][cc-by-nc-nd].

[![CC BY-NC-ND 4.0][cc-by-nc-nd-image]][cc-by-nc-nd]

[cc-by-nc-nd]: http://creativecommons.org/licenses/by-nc-nd/4.0/
[cc-by-nc-nd-image]: https://licensebuttons.net/l/by-nc-nd/4.0/88x31.png
[cc-by-nc-nd-shield]: https://img.shields.io/badge/License-CC%20BY--NC--ND%204.0-lightgrey.svg

### _P-Vision Tools_

P-Vision Tools\
Copyright (C) 2026 by Oliver Achten

These programs are free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

These programs are distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with these programs.  If not, see <http://www.gnu.org/licenses/>.

### _P-Vision P96 Driver_

Please consult the pVision.readme file from https://aminet.net/driver/video/pVision.lha .

## Credits / Thanks
* **Oliver Achten** - product idea, concept, PCB design, FPGA logic, docs, base software
* **Thomas Richter** - P-Vision P96 driver & technical advisor on the Blitter design
* **Simon Neumann & Torsten Hees** - P-Vision prototype production coordination & support
* **Kymon Zonias** - Being a great guy and lots of moral support! ;-)
* **José Manuel Menéndez** - P-Vision case design
* **Oliver Renz de Font** - P-Vision case manufacturing & technical advisor

