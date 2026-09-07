# 启明星 Zynq-7020 SD 卡图片推理说明

本工程已按正点原子启明星 Zynq-7020 和 Vivado/Vitis 2021.1 重新生成硬件与启动文件。板卡从 SD 卡启动后，会读取 JPG、JPEG 或 PNG 图片，完成 YOLOv4-Tiny 推理，并将带检测框的结果写回 SD 卡。

## SD 卡目录

```text
SD 卡根目录
├─ BOOT.BIN
├─ yolov4_tiny_weights\
└─ yolo_test\
   ├─ input.jpg
   ├─ test.jpg
   └─ result.jpg       板卡运行后生成或覆盖
```

程序按以下固定顺序寻找输入，找到第一个存在的文件后停止查找：

1. `yolo_test\input.jpg`
2. `yolo_test\input.jpeg`
3. `yolo_test\input.png`

输出固定为 `yolo_test\result.jpg`。结果图保留输入图分辨率，并绘制检测框和 COCO 类别编号（0 到 79）。

## 板端处理流程

1. 通过 FatFs 挂载 SD0 并读取输入图片。
2. 使用 STB 将 JPEG 或 PNG 解码为 RGB。
3. 双线性缩放为 416 x 416，按 RGB、CHW 排列并归一化。
4. 调用原有 YOLOv4-Tiny PL 加速器完成前向推理。
5. 在 PS 端进行 YOLO 解码、置信度筛选和分类 NMS。
6. 在原图上绘制检测框，编码为 JPEG 并覆盖写入 `result.jpg`。
7. 文件写回成功后，将 MIO7 拉高并点亮底板 `PS_LED0`。

## 板卡参数

- 器件：`xc7z020clg400-2`
- DDR：1 GB，32 位，533.33 MHz
- SD0：MIO40 到 MIO45，卡检测 MIO10
- 调试串口：UART1，MIO48 到 MIO49，115200，8N1
- PL 时钟：120 MHz
- 卷积 IP 地址：`0x43C00000`
- 完成指示灯：底板 `PS_LED0`，MIO7，高电平点亮

图片文件最大 16 MB，解码后最大 1200 万像素，单边最大 4096 像素。置信度阈值为 0.2，NMS IoU 阈值为 0.3，输出 JPEG 质量为 90。

## 构建产物

- XSA：`vivado_prj\output\qiming_yolo.xsa`
- 位流：`vivado_prj\output\design_1_wrapper.bit`
- FSBL：`vitis_prj\workspace\qiming_yolo_platform\export\qiming_yolo_platform\sw\qiming_yolo_platform\boot\fsbl.elf`
- 应用：`vitis_prj\output\yolo_qiming.elf`
- 启动文件：`vitis_prj\boot\BOOT.BIN`
- Bootgen 描述：`vitis_prj\boot\qiming_yolo.bif`

`BOOT.BIN` 的分区顺序为 FSBL、启明星位流、YOLO 应用 ELF。

## 上板步骤

1. 在 Windows 中安全弹出 SD 卡，将卡插入启明星板卡。
2. 将板卡启动方式切换到 SD 启动。
3. 连接 UART1，设置为 115200、8N1，然后给板卡重新上电。
4. 等待底板 `PS_LED0` 点亮；串口可同时观察 `inference completed` 和 `Result saved`。
5. 断电后取出 SD 卡，检查 `yolo_test\result.jpg`。

已通过 JTAG 实板验证完成推理路径：MIO7 的方向、输出使能和输出数据 bit7 均在完成后变为 1。JTAG 验证未写入或覆盖 QSPI。
