# YOLOv4-Tiny FPGA Accelerator on Zynq-7020

A complete, bare-metal YOLOv4-Tiny object detector running on a Xilinx Zynq-7020 SoC.
The ARM cores (PS) handle file I/O, image decoding and detection post-processing; a
custom HLS-generated convolution engine (PL) executes every convolution, max-pool and
upsample in the network.

Drop a JPEG or PNG on an SD card, power the board, and it writes back the same image
with detection boxes drawn on it — no host PC, no OS, no network.

```
SD:/yolo_test/input.jpg  ──►  [ Zynq-7020 ]  ──►  SD:/yolo_test/result.jpg  +  LED D7 on
```

---

## Table of contents

- [How it works](#how-it-works)
- [Accelerator design and optimizations](#accelerator-design-and-optimizations)
- [Hardware](#hardware)
- [Repository layout](#repository-layout)
- [Prerequisites](#prerequisites)
- [Build from source](#build-from-source)
- [Preparing the SD card](#preparing-the-sd-card)
- [Running on the board](#running-on-the-board)
- [Weights](#weights)
- [Porting to a different board](#porting-to-a-different-board)
- [Troubleshooting](#troubleshooting)
- [Credits](#credits)

---

## How it works

The design is split across the two halves of the Zynq SoC:

| Runs on | What | Where in this repo |
|---|---|---|
| **PL** (FPGA fabric) | One `conv` IP core: 3×3 conv, 1×1 conv, max-pool, and upsample, selected at run time by a `kernel` register | `hls_prj/top/src/` |
| **PS** (dual ARM Cortex-A9) | Bare-metal C++ app: FatFs SD access, JPEG/PNG codec, network sequencing, YOLO decode + NMS, box drawing | `vitis_prj/app/src/` |

Each layer of the network is issued from software as a single call to the accelerator.
`conv_leakyrelu()` writes the source/weight/bias/destination DDR addresses and the layer
geometry into the IP's AXI-Lite registers, starts it, and polls until done. The IP masters
DDR directly over four parallel AXI ports, so no data is copied through the CPU.

**Numerics.** Weights and activations are `int16` fixed-point with a scale of 512
(`data_t`, `SCALE` in `sd_io.h`). Weight files on the SD card are float32 and are converted
during load. Batch-norm is folded into the preceding convolution offline, so the hardware
only ever performs `conv → bias → leaky-ReLU`.

**Network.** Standard YOLOv4-Tiny: a CSPDarknet-53-tiny backbone, an FPN neck with a 2×
upsample, and two detection heads at 13×13 and 26×26 over 80 COCO classes. Input is
416×416 RGB (bilinear resize, CHW). Detection uses a 0.20 confidence threshold and
per-class NMS at 0.30 IoU.

---

## Accelerator design and optimizations

Two things make the accelerator fast: how the PS and PL talk to each other, and how the
HLS kernel itself is written. Both are visible directly in `hls_prj/top/src/std_conv.cpp`
and `.h`.

### PS ↔ PL interface

The `conv` IP has no DMA engine and no interrupt — the interface is two separate AXI
planes plus a memory contract the firmware has to honour:

- **Control plane — one AXI-Lite slave at `0x43C0_0000`.** Six scalars (`ch_in`, `ch_out`,
  `fsize`, `stride`, `kernel`, `act`) and a start/done handshake. Deliberately narrow: a
  few dozen bytes per layer, so its latency is irrelevant.
- **Data plane — eight independent AXI master ports** (`FM1`–`FM4` for feature maps,
  `W1`–`W4` for weights), each bursting up to 256 beats. Splitting into four parallel
  channel-planes instead of one wide port lets the core stream several channels
  concurrently rather than serializing through a single interface.
- **Polling, not interrupts.** A bare-metal program with one job spins on `XConv_IsDone()`
  — simpler and lower-latency here than an ISR.
- **The real contract is cache coherency.** The HP ports have no hardware coherency with
  the ARM caches, so software must `Xil_DCacheFlush()` before `XConv_Start()` and
  `Xil_DCacheInvalidateRange()` after `XConv_IsDone()` — otherwise the accelerator reads
  stale input or the CPU reads stale output. See `conv_leakyrelu()` in `basic_op.cpp`.

The per-layer handshake, in full:

```
PS: write 13 buffer addresses + 6 geometry scalars
PS: Xil_DCacheFlush()
PS: XConv_Start()
PL: burst-reads DDR, computes, burst-writes results back
PS: poll XConv_IsDone(), then Xil_DCacheInvalidateRange()
```

### HLS kernel optimizations (`std_conv.cpp`)

Four transformations take the 3×3 convolution engine from a naive loop nest to using
every DSP slice on the device:

1. **Tiling** (`Tr=Tc=13, Tn=4, Tm=32`). The kernel works on a 13×13 output tile of 32
   output channels at a time, fed by a 27×27 input tile that already includes the 3×3
   halo (`compute_output()`). Everything outside the current tile stays in DDR.
2. **Double-buffered prefetch.** `fm_in_buff1/2` and `wt_buff1/2` ping-pong: while the
   current tile is being multiplied in `compute()`, the next input-channel tile is already
   loading from DDR (`compute_output()`'s `pingpong` loop). DDR latency is hidden behind
   compute instead of stalling it.
3. **Loop unrolling with `PIPELINE II=1`.** The `Tm` and `Tn` loops sit inside an
   initiation-interval-1 pipeline (`#pragma HLS PIPELINE II=1` in `compute()`), so HLS
   instantiates `Tm × Tn = 128` multiply-accumulates issuing in the same cycle.
4. **Steering index arithmetic off the DSPs.** `#pragma HLS RESOURCE ... core=Mul_LUT`
   forces address/offset multiplications into LUT fabric instead of DSP blocks, so all
   220 DSPs on the device stay free for actual convolution MACs.

The trade this buys: **69% of LUTs and 94% of slices spent to keep 100% of the device's
DSPs busy** (see [utilization numbers](#hardware) below) — this design is DSP-bound, not
routing-bound. Weights and activations use `ap_fixed<16,7,AP_RND,AP_SAT>` — one DSP per
multiply-accumulate, with rounding and saturation instead of wraparound.

---

## Hardware

Developed and tested on:

| Item | Value |
|---|---|
| Board | **Mind-Dev-Z7020** carrier + **Mind-C-Z7020** SoM (Mind Instrument) |
| Device | `xc7z020clg400-2` (XC7Z020-2CLG400I) |
| DDR | 1 GB DDR3, 32-bit, 533.33 MHz (2× MT41K256M16TW-107:P) |
| PL clock | 120 MHz (`FCLK_CLK0`) |
| Accelerator base address | `0x43C00000` |
| Boot | SD card (`SW1-1` **ON**, `SW1-2` **OFF**) |
| Debug UART | UART0 on MIO14/MIO15 → USB-C port `USBC4` (CH340X), **115200 8N1** |
| SD socket | SD0 on MIO40–45, card-detect on **MIO9**, `CON1` on the underside of the carrier |
| Status LED | `D7`, driven by **MIO7** — lights only after a fully successful run |

> The SD signals pass through a `TXS02612` level translator (bank is 1.8 V, card is 3.3 V).
> The carrier board's own `SW1` DIP selects the boot mode; see the board user manual.

Post-route utilization, from `vivado_prj/output/utilization_mindc.rpt` and
`timing_summary_mindc.rpt`:

| Resource | Used | Available | Utilization |
|---|---|---|---|
| DSP slices | 220 | 220 | **100%** |
| Slice LUTs | 36,519 | 53,200 | 68.6% |
| Block RAM tiles | 94.5 | 140 | 67.5% |
| Slice registers | 39,405 | 106,400 | 37.0% |

Timing closes with **WNS +0.347 ns**, **WHS +0.035 ns**, and zero failing endpoints out of
139,525. The design is DSP-bound: every multiplier on the device is committed to the 3×3
engine, so further speed-up on this part has to come from arithmetic (narrower data,
Winograd) rather than from routing or clocking headroom.

---

## Repository layout

```
hls_prj/                        Vitis HLS project — the accelerator core
  top/src/
    conv.cpp/.h                 Top-level IP: dispatches on `kernel`
    std_conv.cpp/.h             3×3 convolution        (kernel == 3)
    pwconv.cpp/.h               1×1 pointwise conv     (kernel == 1)
    maxpool.cpp/.h              2×2 max-pool           (kernel == 2)
    upsample.cpp/.h             2× nearest upsample    (kernel == 0)
    winoconv_test.*             Winograd variant (not currently instantiated)
    *_test.cpp, test_tb.cpp     C-simulation testbenches
  top/top/solution1/impl/ip/    Exported IP-XACT package consumed by Vivado

vivado_prj/                     Vivado project — PS + accelerator integration
  acc_project/…/design_1.bd     Block design: PS7 + conv IP + AXI interconnect
  scripts/
    build_hardware.tcl          Configures PS7, synthesizes, implements, exports XSA
    run_full_build.ps1          Runs the entire 4-stage build chain end to end
    check_project.tcl           Prints part / clock / MIO settings without building
    scan_hardware.tcl           Lists JTAG targets
  output/                       .bit and .xsa land here (git-ignored)

vitis_prj/                      Bare-metal firmware
  app/src/
    main.cpp                    Entry point: LED, SD mount, input discovery, LED-on
    debug.cpp/.h                yolo_test() — the inference orchestrator
    yolo4_tiny.h                Network topology (backbone + neck + two heads)
    basic_block.h               BasicConv / Resblock_body / CSPDarkNet classes
    basic_op.cpp/.h             Accelerator driver glue (XConv register programming)
    image_io.cpp/.h             JPEG/PNG decode + JPEG encode (stb, SD-backed)
    yolo_postprocess.cpp/.h     Resize/normalize, YOLO decode, NMS, box drawing
    sd_io.cpp/.h                FatFs mount, file I/O, float→fixed-point weight load
    boot_diag.cpp               Pre-main UART marker (diagnostic builds only)
    lscript.ld                  Linker script
    third_party/                stb_image.h, stb_image_write.h
  scripts/
    build_platform.tcl          Creates/updates the Vitis platform + BSP + FSBL
    build_app.ps1               Compiles and links the application ELF
    build_boot.ps1              Packs BOOT.BIN with bootgen
    run_jtag_led.tcl            Volatile JTAG bring-up test (does not touch QSPI)
  boot/qiming_yolo.bif          Boot image recipe: FSBL + bitstream + application

tools/predictor/                Host-side Python tooling
  bn_merge.py                   Folds BatchNorm into conv weights → folded_weights/
  yolo.py, predict.py           PyTorch reference model; generates/decodes test vectors
  utils/                        Box decode / NMS reference implementation
  yolo4-tiny/                   x86 build of the same C++ inference code (no hardware)

docs/                           Board bring-up and SD card notes
```

---

## Prerequisites

**Xilinx tools — Vivado and Vitis 2021.1** (2021.2 also works; the project auto-upgrades).

> **Important:** the installer must include **Zynq-7000 device support**. A Vivado
> installation set up for only 7-series FPGAs (Artix/Kintex/Virtex/Spartan) will fail
> with `Project part 'xc7z020clg400-2' is not available`. Re-run the Xilinx Unified
> Installer and tick **Zynq-7000** under device families.

Also needed:

- Windows with PowerShell (the build scripts are `.tcl` + `.ps1`)
- A micro-SD card, FAT32 formatted
- USB-C cable for the `USBC4` debug serial port
- Python 3 with `torch` and `numpy` — only if you want to regenerate weights

---

## Build from source

All four stages, in order, from the repository root:

```bash
powershell -ExecutionPolicy Bypass -File .\vivado_prj\scripts\run_full_build.ps1 -XilinxRoot 'C:\Xilinx' -XilinxVersion '2021.2'
```

That wrapper simply runs these in sequence — run them individually if you prefer:

```powershell
# 1. Hardware: configure PS7, synthesize, implement, export bitstream + XSA
& 'C:\Xilinx\Vivado\2021.2\bin\vivado.bat' -mode batch -source .\vivado_prj\scripts\build_hardware.tcl

# 2. Platform: build the Vitis standalone platform, BSP (with xilffs) and FSBL
& 'C:\Xilinx\Vitis\2021.2\bin\xsct.bat' .\vitis_prj\scripts\build_platform.tcl

# 3. Application: compile and link the firmware ELF
& .\vitis_prj\scripts\build_app.ps1 -XilinxRoot 'C:\Xilinx' -XilinxVersion '2021.2'

# 4. Boot image: pack FSBL + bitstream + ELF into BOOT.BIN
& .\vitis_prj\scripts\build_boot.ps1 -XilinxRoot 'C:\Xilinx' -XilinxVersion '2021.2'
```

Add `-Diagnostic` to step 3 to build the instrumented firmware — it prints progress over
UART, records stage codes and tensor statistics in `volatile` globals for JTAG inspection,
and writes `yolo_test/boot_stage.txt` to the SD card so you can tell how far it got even
with no serial console attached.

Outputs:

| File | Produced by |
|---|---|
| `vivado_prj/output/design_1_wrapper_mindc.bit` | step 1 |
| `vivado_prj/output/mindc_yolo.xsa` | step 1 |
| `vitis_prj/output/yolo_qiming.elf` | step 3 |
| `vitis_prj/boot/BOOT.BIN` | step 4 |

---

## Preparing the SD card

Format the card as **FAT32**, then copy:

```
<SD root>/
├── BOOT.BIN                    from vitis_prj/boot/
├── yolov4_tiny_weights/        folded weights (see below)
│   ├── BasicConv1/{w,b}.bin
│   ├── BasicConv2/{w,b}.bin
│   ├── BasicConv3/{w,b}.bin
│   ├── ResBlock1/{w1..w4,b1..b4}.bin
│   ├── ResBlock2/{w1..w4,b1..b4}.bin
│   ├── ResBlock3/{w1..w4,b1..b4}.bin
│   ├── conv_forP5/{w,b}.bin
│   ├── upsample/{w,b}.bin
│   ├── yolo_headP4/{w1,b1,w2,b2}.bin
│   └── yolo_headP5/{w1,b1,w2,b2}.bin
└── yolo_test/
    └── input.jpg               your image
```

The firmware looks for `yolo_test/input.jpg`, then `input.jpeg`, then `input.png`, and uses
the first one it finds. Limits: 16 MB file size, 12 megapixels, 4096 px per side.

---

## Running on the board

1. Set the boot DIP switch to SD: **`SW1-1` ON, `SW1-2` OFF**.
2. Insert the card into `CON1` (underside of the carrier board).
3. Connect a USB-C cable to `USBC4` and open a terminal at **115200 8N1**.
4. Power on.

Expected serial output:

```
YOLOv4-Tiny SD JPEG/PNG inference
[APP] mounting SD
Image loaded: yolo_test\input.jpg (640x480)
preprocessing image
reading yolov4_tiny_weights\BasicConv1\w.bin
...
time elapsed is 1234567 us
detections after NMS: 3
class=16 score=0.87 box=(120,45,410,388)
Result saved: yolo_test\result.jpg
inference completed
```

**LED `D7` lights only on complete success** — input found, weights loaded, inference done,
and `result.jpg` written. If it stays dark, something in that chain failed; the serial log
says which step.

---

## Weights

The `.bin` weight files are **not** in this repository (they are ~23 MB of binary). Generate
them yourself:

1. Get a YOLOv4-Tiny PyTorch checkpoint — e.g. `yolov4_tiny_weights_coco.pth` from
   [bubbliiiing/yolov4-tiny-pytorch](https://github.com/bubbliiiing/yolov4-tiny-pytorch),
   which this project's network layout and layer naming follow.
2. Place it in `tools/predictor/` and run:

   ```bash
   cd tools/predictor
   python bn_merge.py
   ```

3. Copy the resulting `folded_weights/` tree to the SD card as `yolov4_tiny_weights/`.

`bn_merge.py` fuses each BatchNorm into its preceding convolution
(`W' = W·γ/√(σ²+ε)`, `b' = β − μ·γ/√(σ²+ε)`) and writes each layer as raw float32. The
firmware converts to `int16` at load time by multiplying by `SCALE` (512) and rounding.

To validate against the hardware without a board, `tools/predictor/yolo4-tiny/` builds the
same C++ inference path for x86, and `predict.py` can decode raw `out0.bin`/`out1.bin`
tensors in Python.

---

## Porting to a different board

Everything board-specific lives in **one Tcl block** in
`vivado_prj/scripts/build_hardware.tcl`, plus one constant in the firmware. Check each
against your own board's schematic before the first build — a wrong pin here produces a
board that boots and then silently does nothing.

| What | This board | Change it in |
|---|---|---|
| FPGA part | `xc7z020clg400-2` | `acc_project.xpr`, or recreate the project |
| DDR3 part number | `MT41K256M16TW-107:P` | `CONFIG.PCW_UIPARAM_DDR_PARTNO` |
| DDR bus width / freq | 32-bit, 533.33 MHz | `CONFIG.PCW_UIPARAM_DDR_BUS_WIDTH`, `..._FREQ_MHZ` |
| DDR board delays | 0.25 ns / 0.0 ns | `CONFIG.PCW_UIPARAM_DDR_BOARD_DELAY0..3`, `..._DQS_TO_CLK_DELAY_0..3` |
| SD0 data/clk/cmd | MIO40–45 | `CONFIG.PCW_SD0_SD0_IO` |
| **SD card detect** | **MIO9** | `CONFIG.PCW_SD0_GRP_CD_IO` |
| Debug UART | UART0, MIO14–15 | `CONFIG.PCW_UART0_UART0_IO`; also `bsp config stdout/stdin` in `build_platform.tcl` |
| Status LED | MIO7 | `DONE_LED_PIN` in `vitis_prj/app/src/main.cpp` |
| PL clock | 120 MHz | `CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ` (a guard in the script asserts this) |

Notes from porting this design once already:

- **The DDR board-delay values are layout-specific.** They are carried over from a
  reference design with the same DDR3 part. If your PCB routing differs, get the correct
  values from your board vendor — bad delays give you a board that boots but corrupts data
  under load.
- **Card detect is the easiest pin to get wrong.** It is a plain GPIO the SD controller
  samples; if it points at a pin that is doing something else, `f_mount()` fails and the
  application exits before reading anything.
- **Check which UART is actually wired out.** MIO48/49 is a common default in reference
  designs, but on many boards those pins are used by on-board eMMC.
- The accelerator itself is board-independent. Only the PS7 configuration changes.

---

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| Nothing on serial | Wrong UART. Confirm which MIO pair is routed to your USB-serial bridge and that `bsp config stdout` matches. |
| Boots but LED never lights, no output file | SD card not mounting. Check `PCW_SD0_GRP_CD_IO` against the schematic, and confirm the card is FAT32. |
| `f_mount returned 3` | Card-detect / SD0 pin mismatch, or a card the controller can't initialize. |
| `no input image found` | SD mounted, but no `yolo_test/input.jpg\|jpeg\|png` present. |
| `Project part 'xc7z020clg400-2' is not available` | Vivado installed without Zynq-7000 device support. Re-run the installer. |
| Detections are nonsense | Weight files missing or truncated — a failed `read_params` is not fatal at run time. Build with `-Diagnostic` and check `g_diag_param_read_failures`. |
| Inference hangs mid-layer | Accelerator never asserts done — check that the bitstream in `BOOT.BIN` matches the XSA the platform was built from. |

For deeper bring-up, `vitis_prj/scripts/run_jtag_led.tcl` loads the bitstream and ELF over
JTAG into volatile configuration and DDR only — it never writes QSPI — and dumps the LED
GPIO registers before and after the run.

---

## Credits

- Network architecture and reference PyTorch implementation follow
  [bubbliiiing/yolov4-tiny-pytorch](https://github.com/bubbliiiing/yolov4-tiny-pytorch).
- Image codec: [`stb_image` / `stb_image_write`](https://github.com/nothings/stb) (public domain).
- FAT filesystem: FatFs, via the Xilinx `xilffs` BSP library.
