# SD Card Image Inference on Zynq-7020

This project targets the Mind-Dev-Z7020 carrier board with Vivado/Vitis 2021.1.
After booting from the SD card, the board reads a JPG, JPEG or PNG image, runs
YOLOv4-Tiny inference, and writes the result — with detection boxes drawn on it —
back to the SD card.

## SD card layout

```text
SD card root
├─ BOOT.BIN
├─ yolov4_tiny_weights\
└─ yolo_test\
   ├─ input.jpg
   ├─ test.jpg
   └─ result.jpg       created or overwritten by the board
```

The program looks for its input in this fixed order and stops at the first file
that exists:

1. `yolo_test\input.jpg`
2. `yolo_test\input.jpeg`
3. `yolo_test\input.png`

The output is always `yolo_test\result.jpg`. It keeps the input image's
resolution, with detection boxes and COCO class numbers (0 to 79) drawn on top.

## On-board processing flow

1. Mount SD0 through FatFs and read the input image.
2. Decode the JPEG or PNG to RGB using STB.
3. Bilinear-resize to 416 x 416, arranged as RGB in CHW order and normalised.
4. Run the forward pass on the existing YOLOv4-Tiny PL accelerator.
5. Decode the YOLO heads, filter by confidence and run per-class NMS on the PS.
6. Draw the detection boxes onto the original image, encode to JPEG and
   overwrite `result.jpg`.
7. Once the file is written back, drive MIO7 high to light the carrier board's
   `PS_LED0`.

## Board parameters

- Device: `xc7z020clg400-2`
- DDR: 1 GB, 32-bit, 533.33 MHz
- SD0: MIO40 to MIO45, card detect on MIO9
- Debug serial: UART0, MIO14 to MIO15, 115200, 8N1
- PL clock: 120 MHz
- Convolution IP address: `0x43C00000`
- Completion indicator: carrier board `PS_LED0`, MIO7, active high

Image files may be at most 16 MB, and at most 12 megapixels once decoded, with a
maximum of 4096 pixels on either side. The confidence threshold is 0.2, the NMS
IoU threshold is 0.3, and the output JPEG quality is 90.

## Build outputs

- XSA: `vivado_prj\output\mindc_yolo.xsa`
- Bitstream: `vivado_prj\output\design_1_wrapper_mindc.bit`
- FSBL: `vitis_prj\workspace_mindc\mindc_yolo_platform\export\mindc_yolo_platform\sw\mindc_yolo_platform\boot\fsbl.elf`
- Application: `vitis_prj\output\yolo_qiming.elf`
- Boot image: `vitis_prj\boot\BOOT.BIN`
- Bootgen description: `vitis_prj\boot\qiming_yolo.bif`

`BOOT.BIN` contains its partitions in this order: FSBL, bitstream, YOLO
application ELF.

## Running on the board

1. Safely eject the SD card in Windows and insert it into the board.
2. Set the boot mode switch to SD boot (`SW1-1` ON, `SW1-2` OFF).
3. Connect the debug UART, set the terminal to 115200 8N1, then power-cycle the
   board.
4. Wait for `PS_LED0` to light. The serial port should show `inference completed`
   and `Result saved` at the same time.
5. Power down, remove the SD card and check `yolo_test\result.jpg`.

The inference path has been verified on real hardware over JTAG: MIO7's
direction, output enable and output data bit 7 all go to 1 once the run
completes. That JTAG check writes nothing to QSPI.
