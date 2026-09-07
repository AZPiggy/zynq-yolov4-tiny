# yolo4-tiny Implementation using cpp

A pure C++ implementation of the YOLOv4-Tiny network, built for the host (x86) so
the inference code can be checked without hardware.

The weight files are hosted separately — see the "Weights" section of the
repository README for how to generate them with `tools/predictor/bn_merge.py`.

Note: the `yolo_test` folder holds the input (`img.bin`) and the known-good
outputs (`out0.bin`, `out1.bin`). The C++ implementation takes `img.bin` as its
input, produces its own outputs, and compares them against `out0.bin` and
`out1.bin` to verify correctness.

Build it with any C++ compiler, for example:

```bash
g++ -O2 -std=gnu++11 main.cpp basic_op.cpp basic_block.cpp debug.cpp yolo4_tiny.cpp -o main
```
