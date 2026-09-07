/*
 * ================================================================
 * 文件名称：main.cpp
 * 工程名称：启明星 Zynq-7020 YOLOv4-Tiny
 *
 * 功能说明：
 *  1. 初始化 Zynq PS 端 GPIO，并控制底板 PS_LED0。
 *  2. 挂载 SD 卡文件系统。
 *  3. 在 SD 卡的 yolo_test 目录中查找：
 *         input.jpg
 *         input.jpeg
 *         input.png
 *     三种输入图片中的任意一种。
 *  4. 调用 yolo_test() 完成 YOLOv4-Tiny 推理。
 *  5. 将目标检测结果保存到：
 *         yolo_test/result.jpg
 *  6. 当且仅当整个流程成功完成后，点亮 PS_LED0。
 *
 * 如果定义 QIMING_DIAGNOSTIC：
 *  程序还会启用额外的板端诊断功能，用于：
 *      - 记录程序执行阶段
 *      - 记录 SD 卡初始化状态
 *      - 记录文件查找状态
 *      - 记录权重读取状态
 *      - 记录推理时间
 *      - 记录输出张量统计信息
 *      - 将输入/输出图片复制到 DDR，方便通过 JTAG 查看
 * ================================================================
 */


/* C++ 标准输入输出库
 *
 * 主要用于：
 *      cout << ...
 *      endl
 *
 * 在 Vitis standalone 环境中，如果 BSP 中配置了标准输出设备，
 * cout 最终可以通过 UART 输出。
 */
#include <iostream>

/*
 * FatFs 文件系统头文件。
 *
 * 提供：
 *      FIL
 *      FILINFO
 *      f_open()
 *      f_read()
 *      f_write()
 *      f_close()
 *      f_stat()
 *
 * 等 SD 卡 FAT 文件系统操作接口。
 */
#include "ff.h"

/*
 * Zynq PS GPIO 驱动。
 *
 * 用于控制 Processing System（PS）侧的 MIO GPIO。
 * 本工程使用 MIO7 控制底板 PS_LED0。
 */
#include "xgpiops.h"

/*
 * Xilinx 轻量级 printf。
 *
 * xil_printf() 通常比完整 printf() 更适合裸机系统，
 * 因为占用的代码空间更小。
 */
#include "xil_printf.h"

/*
 * Vivado 导出硬件平台后自动生成的参数定义文件。
 *
 * 其中包含：
 *      XPAR_XGPIOPS_0_DEVICE_ID
 *
 * 等硬件 IP 参数。
 */
#include "xparameters.h"

/*
 * 工程内部调试 / YOLO 推理相关声明。
 *
 * 根据当前工程结构，SD_Init()、yolo_test() 等函数
 * 很可能通过该头文件或其进一步包含的头文件进行声明。
 */
#include "debug.h"


/*
 * GCC 编译优化设置：
 *
 * optimize(3)
 *      相当于较高级别的编译优化。
 *
 * Ofast
 *      进一步追求程序执行速度。
 *
 * inline
 *      尽可能执行函数内联。
 *
 * 对 YOLO 推理这种计算密集型程序，
 * 可以降低部分软件执行开销。
 *
 * 注意：
 * Ofast 会使用一些比标准 C/C++ 更激进的优化，
 * 尤其会影响浮点运算严格性。
 */
#pragma GCC optimize(3, "Ofast", "inline")


/*
 * 允许直接使用：
 *
 *      string
 *      cout
 *      endl
 *
 * 而不需要每次写：
 *
 *      std::string
 *      std::cout
 *      std::endl
 */
using namespace std;


/*
 * ================================================================
 * 匿名命名空间
 * ================================================================
 *
 * 匿名 namespace 中定义的变量和函数仅在当前源文件中可见。
 *
 * 类似于 C 语言中的 static 全局变量 / static 函数，
 * 可以防止与工程其他源文件中的同名符号发生冲突。
 */
namespace {


/*
 * PS_LED0 对应的 GPIO Pin。
 *
 * 当前值：
 *
 *      7U
 *
 * 表示使用 Zynq PS GPIO 的 MIO7。
 *
 * U 表示该常量是 unsigned 类型。
 */
const u32 DONE_LED_PIN = 7U;


/*
 * PS GPIO 驱动实例。
 *
 * 后续初始化、设置方向以及写 GPIO 电平，
 * 都通过该对象完成。
 */
XGpioPs ps_gpio;


/*
 * ================================================================
 * 函数名称：initialize_done_led
 *
 * 功能：
 *      初始化用于表示 YOLO 推理完成状态的 PS_LED0。
 *
 * 初始化过程：
 *
 *      1. 查询 PS GPIO 配置信息
 *      2. 初始化 GPIO 驱动
 *      3. 将 MIO7 设置为输出
 *      4. 使能 MIO7 输出
 *      5. 默认输出低电平，使 LED 熄灭
 *
 * 返回值：
 *
 *      true
 *          初始化成功
 *
 *      false
 *          初始化失败
 *
 * 设计意图：
 *
 *      程序刚启动时 LED 必须保持熄灭。
 *      只有：
 *
 *          SD 卡成功
 *          输入图片成功
 *          权重读取成功
 *          YOLO 推理成功
 *          result.jpg 写入成功
 *
 *      后，才调用 set_done_led() 点亮 LED。
 * ================================================================
 */
bool initialize_done_led()
{
    /*
     * 根据 Vivado/Vitis 自动生成的 Device ID，
     * 查找 PS GPIO 的硬件配置。
     */
    XGpioPs_Config* config =
        XGpioPs_LookupConfig(XPAR_XGPIOPS_0_DEVICE_ID);


    /*
     * 判断：
     *
     * 1. 是否成功找到 GPIO 配置
     * 2. GPIO 驱动是否初始化成功
     *
     * XST_SUCCESS 表示 Xilinx 驱动函数执行成功。
     */
    if (config == 0 ||
        XGpioPs_CfgInitialize(
            &ps_gpio,          // GPIO 驱动实例
            config,            // GPIO 配置参数
            config->BaseAddr   // GPIO 寄存器基地址
        ) != XST_SUCCESS) {

        /*
         * 初始化失败时通过串口打印错误信息。
         */
        xil_printf(
            "ERROR: PS_LED0 GPIO initialization failed\r\n"
        );

        return false;
    }


    /*
     * 设置 MIO7 为输出方向。
     *
     * 第三个参数：
     *
     *      1U = 输出
     *      0U = 输入
     */
    XGpioPs_SetDirectionPin(
        &ps_gpio,
        DONE_LED_PIN,
        1U
    );


    /*
     * 使能 MIO7 输出。
     *
     * 即允许 GPIO 驱动实际向引脚输出逻辑电平。
     */
    XGpioPs_SetOutputEnablePin(
        &ps_gpio,
        DONE_LED_PIN,
        1U
    );


    /*
     * 程序启动时首先输出低电平。
     *
     * 因此：
     *
     *      PS_LED0 初始状态 = 熄灭
     *
     * 这样可以确保 LED 不会因为之前的 GPIO 状态
     * 或程序异常而误表示“推理成功”。
     */
    XGpioPs_WritePin(
        &ps_gpio,
        DONE_LED_PIN,
        0U
    );


    return true;
}


/*
 * ================================================================
 * 函数名称：set_done_led
 *
 * 功能：
 *      点亮 PS_LED0。
 *
 * 当前硬件设计中：
 *
 *      MIO7 = 1
 *
 * 即输出高电平时 LED 点亮。
 *
 * 该函数只应在整个 YOLO 推理流程成功后调用。
 * ================================================================
 */
void set_done_led()
{
    XGpioPs_WritePin(
        &ps_gpio,
        DONE_LED_PIN,
        1U
    );
}

} // namespace



/*
 * ================================================================
 * QIMING_DIAGNOSTIC 诊断模式
 * ================================================================
 *
 * 只有在编译时定义：
 *
 *      QIMING_DIAGNOSTIC
 *
 * 下面的代码才会被编译。
 *
 * 例如可以在编译参数中加入：
 *
 *      -DQIMING_DIAGNOSTIC
 *
 * 这些变量主要用于 JTAG 调试。
 *
 * 由于使用：
 *
 *      volatile
 *
 * 编译器不会轻易优化掉这些变量，
 * 因此可以使用 XSCT、Vitis Debugger 或 JTAG
 * 在程序停止后读取变量值。
 */
#ifdef QIMING_DIAGNOSTIC


/*
 * 当前程序执行到哪个阶段。
 *
 * 可以根据这个值判断程序停在哪里。
 */
extern "C" volatile unsigned int g_diag_stage = 0;


/*
 * SD_Init() 返回值。
 *
 * 默认：
 *
 *      -1 = 尚未执行
 */
extern "C" volatile int g_diag_sd_init_rc = -1;


/*
 * boot_stage.txt 文件操作返回值。
 */
extern "C" volatile int g_diag_stage_file_rc = -1;


/*
 * 三种输入图片：
 *
 *      input.jpg
 *      input.jpeg
 *      input.png
 *
 * 对应 f_stat() 的返回状态。
 */
extern "C" volatile int g_diag_stat_rc[3] =
{
    -1,
    -1,
    -1
};


/*
 * YOLO 最终检测到的目标数量。
 *
 * 该变量通常由其他 YOLO 代码更新。
 */
extern "C" volatile int g_diag_detection_count = -1;


/*
 * 对 result.jpg 执行 f_stat() 的返回值。
 */
extern "C" volatile int g_diag_result_stat_rc = -1;


/*
 * 最终 result.jpg 的文件大小，单位：Byte。
 */
extern "C" volatile unsigned int g_diag_result_size = 0;


/*
 * 模型参数读取次数。
 */
extern "C" volatile unsigned int g_diag_param_read_count = 0;


/*
 * 模型参数读取失败次数。
 */
extern "C" volatile unsigned int g_diag_param_read_failures = 0;


/*
 * 已读取模型参数总字节数。
 */
extern "C" volatile unsigned int g_diag_param_bytes = 0;


/*
 * 第一个 YOLO 输出张量的最小值。
 */
extern "C" volatile int g_diag_out0_min = 0;


/*
 * 第一个 YOLO 输出张量的最大值。
 */
extern "C" volatile int g_diag_out0_max = 0;


/*
 * 第一个输出张量中非零数据数量。
 *
 * 用于判断神经网络是否产生有效输出。
 */
extern "C" volatile unsigned int g_diag_out0_nonzero = 0;


/*
 * 第二个 YOLO 输出张量的最小值。
 */
extern "C" volatile int g_diag_out1_min = 0;


/*
 * 第二个 YOLO 输出张量的最大值。
 */
extern "C" volatile int g_diag_out1_max = 0;


/*
 * 第二个输出张量非零数据数量。
 */
extern "C" volatile unsigned int g_diag_out1_nonzero = 0;


/*
 * 整个 YOLO 推理耗时。
 *
 * 单位：
 *
 *      us（微秒）
 */
extern "C" volatile unsigned int g_diag_inference_us = 0;


/*
 * 两个 YOLO Detection Head 中检测到的
 * 最大 objectness 值。
 *
 * YOLOv4-Tiny 有两个尺度的检测头，因此数组长度为 2。
 */
extern "C" volatile unsigned int
g_diag_head_max_objectness[2] =
{
    0,
    0
};


/*
 * 两个检测头的最大分类得分。
 */
extern "C" volatile unsigned int
g_diag_head_max_score[2] =
{
    0,
    0
};


/*
 * 两个检测头当前得分最高目标对应的类别 ID。
 */
extern "C" volatile int
g_diag_head_best_class[2] =
{
    -1,
    -1
};


/*
 * 将 result.jpg 拷贝到 DDR 后，
 * 保存 DDR 缓冲区地址。
 *
 * 这样 JTAG 可以根据该地址读取整个 JPEG 文件。
 */
extern "C" volatile unsigned int
g_diag_result_copy_address = 0;


/*
 * 实际复制到 DDR 的 result.jpg 字节数。
 */
extern "C" volatile unsigned int
g_diag_result_copy_bytes = 0;


/*
 * result.jpg DDR 拷贝操作返回状态。
 */
extern "C" volatile int
g_diag_result_copy_rc = -1;


/*
 * cat 类别检测到的最大 score。
 *
 * 这是针对测试图片 / 特定类别加入的诊断信息。
 */
extern "C" volatile unsigned int
g_diag_cat_max_score = 0;


/*
 * dog 类别检测到的最大 score。
 */
extern "C" volatile unsigned int
g_diag_dog_max_score = 0;


/*
 * 输入图片复制到固定 DDR 地址后，
 * 实际复制的字节数量。
 */
extern "C" volatile unsigned int
g_diag_input_copy_bytes = 0;


/*
 * 输入图片复制操作返回状态。
 */
extern "C" volatile int
g_diag_input_copy_rc = -1;



/*
 * ================================================================
 * 函数名称：write_boot_stage
 *
 * 参数：
 *
 *      text
 *          要写入 boot_stage.txt 的文本。
 *
 * 功能：
 *
 *      在 SD 卡中创建/覆盖：
 *
 *          yolo_test/boot_stage.txt
 *
 *      然后将当前启动阶段信息写进去。
 *
 * 目的：
 *
 *      即使 UART / JTAG 输出不方便查看，
 *      也可以拔出 SD 卡，在电脑上直接查看该文件，
 *      判断程序至少运行到了哪个阶段。
 *
 * 返回值：
 *
 *      FatFs 的 FRESULT 状态码。
 * ================================================================
 */
static int write_boot_stage(const char* text)
{
    /*
     * FatFs 文件对象。
     */
    FIL file;


    /*
     * 实际写入的字节数。
     */
    UINT written = 0;


    /*
     * 创建或覆盖 boot_stage.txt。
     *
     * FA_CREATE_ALWAYS：
     *      文件存在则清空；
     *      文件不存在则创建。
     *
     * FA_WRITE：
     *      以写模式打开。
     */
    FRESULT result =
        f_open(
            &file,
            "yolo_test\\boot_stage.txt",
            FA_CREATE_ALWAYS | FA_WRITE
        );


    /*
     * 只有文件打开成功后才能继续写入。
     */
    if (result == FR_OK) {

        /*
         * 计算 text 字符串长度。
         *
         * 没有调用 strlen()，
         * 而是手动遍历直到 '\0'。
         */
        unsigned int length = 0;

        while (text[length] != '\0') {
            ++length;
        }


        /*
         * 将诊断文本写入文件。
         */
        result =
            f_write(
                &file,
                text,
                length,
                &written
            );


        /*
         * 即使 f_write 返回 FR_OK，
         * 如果实际写入字节数 != 预期字节数，
         * 仍然认为发生磁盘写入错误。
         */
        if (result == FR_OK &&
            written != length) {

            result = FR_DISK_ERR;
        }


        /*
         * 关闭文件。
         */
        const FRESULT close_result =
            f_close(&file);


        /*
         * 如果之前的写操作成功，
         * 则进一步检查关闭文件时是否发生错误。
         */
        if (result == FR_OK) {
            result = close_result;
        }
    }


    /*
     * 将 FatFs FRESULT 转换成 int 返回。
     */
    return static_cast<int>(result);
}



/*
 * ================================================================
 * 函数名称：diagnostic_stop
 *
 * 参数：
 *
 *      stage
 *          当前程序阶段 / 错误码。
 *
 * 功能：
 *
 *      1. 将诊断阶段写入 g_diag_stage
 *      2. 进入死循环
 *
 * 为什么故意死循环：
 *
 *      在 JTAG 调试模式下，
 *      如果程序直接 return 或复位，
 *      很多运行现场可能丢失。
 *
 *      停在 while(1) 后，可以通过 JTAG
 *      查看所有 g_diag_xxx 变量，
 *      从而分析故障原因。
 * ================================================================
 */
static void diagnostic_stop(unsigned int stage)
{
    /*
     * 保存当前诊断阶段。
     */
    g_diag_stage = stage;


    /*
     * 永久停止在这里。
     *
     * 等待 JTAG / Debugger 检查变量。
     */
    while (1) {
    }
}

#endif // QIMING_DIAGNOSTIC



/*
 * ================================================================
 * main()
 *
 * YOLOv4-Tiny 板端程序主入口。
 *
 * 主流程：
 *
 *      GPIO初始化
 *          ↓
 *      LED熄灭
 *          ↓
 *      初始化SD卡
 *          ↓
 *      查找输入图片
 *          ↓
 *      YOLO推理
 *          ↓
 *      result.jpg写回SD卡
 *          ↓
 *      LED点亮
 *
 * 如果任何一个关键步骤失败：
 *
 *      LED 均保持熄灭。
 * ================================================================
 */
int main()
{
    /*
     * ------------------------------------------------------------
     * 1. 初始化 PS_LED0
     * ------------------------------------------------------------
     *
     * 初始化成功后 LED 默认保持低电平。
     *
     * 如果 GPIO 初始化本身失败，
     * 程序立即结束。
     */
    if (!initialize_done_led()) {
        return 1;
    }


#ifdef QIMING_DIAGNOSTIC

    /*
     * 诊断阶段 1：
     *
     * 已经成功进入 main()。
     */
    g_diag_stage = 1;


    /*
     * UART 打印程序进入 main。
     */
    xil_printf(
        "\r\n[APP] main entered\r\n"
    );

#endif


    /*
     * ------------------------------------------------------------
     * 2. YOLO 权重文件目录
     * ------------------------------------------------------------
     *
     * SD 卡中应存在：
     *
     *      yolov4_tiny_weights
     *
     * 后续 yolo_test() 会从该目录读取模型参数。
     */
    string weight_dir =
        "yolov4_tiny_weights";


    /*
     * ------------------------------------------------------------
     * 3. 输入图片候选路径
     * ------------------------------------------------------------
     *
     * 为了兼容不同图片格式，
     * 程序依次检查：
     *
     *      yolo_test/input.jpg
     *      yolo_test/input.jpeg
     *      yolo_test/input.png
     *
     * 找到第一个存在的文件就停止继续查找。
     *
     * 注意：
     * FatFs 支持 '\' 作为路径分隔，
     * C/C++ 字符串中 '\' 需要写成 '\\'。
     */
    const char* input_candidates[] =
    {
        "yolo_test\\input.jpg",
        "yolo_test\\input.jpeg",
        "yolo_test\\input.png"
    };


    /*
     * YOLO 推理完成后的输出图片。
     *
     * 检测框、类别、置信度等结果应当被绘制到该图片。
     */
    string output_jpeg =
        "yolo_test\\result.jpg";


    /*
     * 串口输出程序功能提示。
     */
    cout
        << "YOLOv4-Tiny SD JPEG/PNG inference"
        << endl;


    /*
     * ------------------------------------------------------------
     * 4. 初始化 / 挂载 SD 卡
     * ------------------------------------------------------------
     */
    xil_printf(
        "[APP] mounting SD\r\n"
    );


    /*
     * 调用工程内部 SD_Init()。
     *
     * 预期它负责：
     *
     *      SD控制器初始化
     *      FatFs注册
     *      FAT分区挂载
     *
     * 返回：
     *
     *      XST_SUCCESS = 成功
     */
    const int sd_init_rc = SD_Init();


#ifdef QIMING_DIAGNOSTIC

    /*
     * 保存 SD 初始化返回值，
     * 方便 JTAG 调试。
     */
    g_diag_sd_init_rc = sd_init_rc;

#endif


    /*
     * 如果 SD 卡初始化失败，
     * 后面的权重、图片都无法读取，
     * 因此直接结束程序。
     */
    if (sd_init_rc != XST_SUCCESS) {

#ifdef QIMING_DIAGNOSTIC

        /*
         * 错误码：
         *
         *      0xE001
         *
         * 表示 SD 初始化阶段失败。
         */
        diagnostic_stop(0xE001);

#endif

        return 1;
    }


#ifdef QIMING_DIAGNOSTIC

    /*
     * 阶段 2：
     *
     * SD 卡已经成功挂载。
     */
    g_diag_stage = 2;


    xil_printf(
        "[APP] SD mounted\r\n"
    );


    /*
     * 向 SD 卡写 boot_stage.txt。
     *
     * 这样即使串口没有输出，
     * 也可以通过电脑检查 SD 卡，
     * 判断程序是否成功执行到这里。
     */
    g_diag_stage_file_rc =
        write_boot_stage(
            "main entered\r\n"
            "SD mounted\r\n"
        );


    /*
     * 阶段 3：
     *
     * SD 挂载并完成 boot stage 文件记录。
     */
    g_diag_stage = 3;

#endif


    /*
     * ------------------------------------------------------------
     * 5. 查找输入图片
     * ------------------------------------------------------------
     *
     * input_image 最终保存实际找到的图片路径。
     *
     * 初始为空。
     */
    string input_image;


    /*
     * FatFs 文件信息结构体。
     *
     * f_stat() 会向其中写入：
     *
     *      文件大小
     *      属性
     *      日期
     *      时间
     *
     * 等信息。
     */
    FILINFO info;


    /*
     * 依次尝试三个文件名。
     */
    for (
        unsigned int i = 0;
        i <
            sizeof(input_candidates) /
            sizeof(input_candidates[0]);
        ++i
    ) {

        /*
         * 判断该文件是否存在。
         *
         * FR_OK：
         *      文件存在并成功读取属性。
         */
        const FRESULT stat_result =
            f_stat(
                input_candidates[i],
                &info
            );


#ifdef QIMING_DIAGNOSTIC

        /*
         * 保存每一种文件格式的 f_stat 返回值。
         *
         * 例如：
         *
         * g_diag_stat_rc[0] → input.jpg
         * g_diag_stat_rc[1] → input.jpeg
         * g_diag_stat_rc[2] → input.png
         */
        g_diag_stat_rc[i] =
            static_cast<int>(
                stat_result
            );

#endif


        /*
         * 找到文件后：
         *
         * 保存实际输入图片路径，
         * 然后退出循环。
         *
         * 因此优先级是：
         *
         *      JPG
         *       >
         *      JPEG
         *       >
         *      PNG
         */
        if (stat_result == FR_OK) {

            input_image =
                input_candidates[i];

            break;
        }
    }


    /*
     * 如果三个候选文件全部不存在，
     * input_image 仍为空。
     */
    if (input_image.empty()) {

        cout
            << "no input image found "
               "(input.jpg, input.jpeg, input.png)"
            << endl;


#ifdef QIMING_DIAGNOSTIC

        /*
         * 错误码：
         *
         *      0xE002
         *
         * 表示 SD 卡已经成功，
         * 但没有找到输入图片。
         */
        diagnostic_stop(0xE002);

#endif

        return 1;
    }


#ifdef QIMING_DIAGNOSTIC

    /*
     * ------------------------------------------------------------
     * 6. Diagnostic：记录已经找到输入图片
     * ------------------------------------------------------------
     */
    g_diag_stage = 4;


    xil_printf(
        "[APP] input found: %s\r\n",
        input_image.c_str()
    );


    /*
     * 更新 SD 卡中的 boot_stage.txt。
     */
    g_diag_stage_file_rc =
        write_boot_stage(
            "main entered\r\n"
            "SD mounted\r\n"
            "input found\r\n"
        );


    /*
     * ------------------------------------------------------------
     * Diagnostic：将原始输入图片复制到 DDR
     * ------------------------------------------------------------
     *
     * 目的不是 YOLO 正常推理，
     * 而是为了 JTAG 调试。
     *
     * 输入图片会复制到：
     *
     *      0x30000000
     *
     * 这样调试时可以直接从 DDR 中读取文件内容，
     * 判断 SD 卡读出的图片是否正确。
     */


    /*
     * 输入图片文件对象。
     */
    FIL input_copy_file;


    /*
     * 打开输入图片，只读。
     */
    g_diag_input_copy_rc =
        static_cast<int>(
            f_open(
                &input_copy_file,
                input_image.c_str(),
                FA_READ
            )
        );


    /*
     * 文件打开成功。
     */
    if (g_diag_input_copy_rc == FR_OK) {

        /*
         * 读取输入图片的完整文件大小。
         */
        const UINT input_copy_size =
            static_cast<UINT>(
                f_size(&input_copy_file)
            );


        /*
         * 记录实际读取字节数。
         */
        UINT input_copy_bytes = 0;


        /*
         * 将完整输入图片读取到固定 DDR 地址：
         *
         *      0x30000000
         *
         * reinterpret_cast<void*>：
         *
         *      将整数地址转换成内存指针。
         */
        g_diag_input_copy_rc =
            static_cast<int>(
                f_read(
                    &input_copy_file,
                    reinterpret_cast<void*>(
                        0x30000000U
                    ),
                    input_copy_size,
                    &input_copy_bytes
                )
            );


        /*
         * 保存实际读取字节数。
         */
        g_diag_input_copy_bytes =
            input_copy_bytes;


        /*
         * 关闭输入文件。
         */
        const FRESULT close_result =
            f_close(
                &input_copy_file
            );


        /*
         * 如果读取本身没有错误，
         * 则使用文件关闭状态作为最终状态。
         */
        if (g_diag_input_copy_rc == FR_OK) {

            g_diag_input_copy_rc =
                static_cast<int>(
                    close_result
                );
        }
    }


    /*
     * 阶段 5：
     *
     * 输入图片已经完成诊断性 DDR 拷贝。
     */
    g_diag_stage = 5;

#endif


    /*
     * UART / 标准输出打印最终选择的输入图片路径。
     */
    cout
        << "input image: "
        << input_image
        << endl;


    /*
     * ------------------------------------------------------------
     * 7. 执行 YOLOv4-Tiny 推理
     * ------------------------------------------------------------
     *
     * 调用：
     *
     *      yolo_test(
     *          权重目录,
     *          输入图片,
     *          输出图片
     *      )
     *
     * 正常情况下 yolo_test() 内部负责：
     *
     *  1. 读取 YOLO 权重
     *  2. JPEG / PNG 解码
     *  3. 图片 resize / letterbox 等预处理
     *  4. 网络前向传播
     *  5. 调用 HLS 卷积加速 IP
     *  6. YOLO Detection Head 解码
     *  7. NMS 非极大值抑制
     *  8. 绘制检测框
     *  9. JPEG 编码
     * 10. 保存 result.jpg
     *
     * yolo_test() 返回：
     *
     *      true  = 全流程成功
     *      false = 某一步失败
     */
    if (
        !yolo_test(
            weight_dir,
            input_image,
            output_jpeg
        )
    ) {

        /*
         * YOLO 推理失败。
         *
         * 此时不会调用 set_done_led()，
         * 因此 PS_LED0 继续保持熄灭。
         */
        cout
            << "inference failed"
            << endl;


#ifdef QIMING_DIAGNOSTIC

        /*
         * 错误码：
         *
         *      0xE003
         *
         * 表示 YOLO 推理阶段失败。
         */
        diagnostic_stop(0xE003);

#endif

        return 1;
    }


    /*
     * ------------------------------------------------------------
     * 8. 推理完全成功后点亮 PS_LED0
     * ------------------------------------------------------------
     *
     * 能执行到这里意味着 yolo_test() 返回 true。
     *
     * 按本工程约定：
     *
     *      输入、权重、推理、result.jpg 写回
     *
     * 应均已经成功完成。
     *
     * 因此将 MIO7 拉高。
     */
    set_done_led();


#ifdef QIMING_DIAGNOSTIC

    /*
     * ------------------------------------------------------------
     * 9. Diagnostic：检查 result.jpg
     * ------------------------------------------------------------
     */


    /*
     * 用于保存 result.jpg 的文件信息。
     */
    FILINFO result_info;


    /*
     * 检查 result.jpg 是否真实存在。
     */
    g_diag_result_stat_rc =
        static_cast<int>(
            f_stat(
                output_jpeg.c_str(),
                &result_info
            )
        );


    /*
     * 如果 result.jpg 存在：
     *
     * 将完整 JPEG 文件读入 DDR，
     * 方便后续 JTAG 导出检查。
     */
    if (g_diag_result_stat_rc == FR_OK) {

        /*
         * 保存 result.jpg 大小。
         */
        g_diag_result_size =
            static_cast<unsigned int>(
                result_info.fsize
            );


        /*
         * 在堆上申请一块和 result.jpg
         * 大小相同的内存。
         *
         * 这里返回的是实际 DDR 地址。
         */
        unsigned char* result_copy =
            new unsigned char[
                g_diag_result_size
            ];


        /*
         * result.jpg 文件对象。
         */
        FIL result_file;


        /*
         * 实际读取的字节数。
         */
        UINT result_bytes = 0;


        /*
         * 以只读方式打开 result.jpg。
         */
        g_diag_result_copy_rc =
            static_cast<int>(
                f_open(
                    &result_file,
                    output_jpeg.c_str(),
                    FA_READ
                )
            );


        /*
         * 打开成功后，将整个文件读取到 result_copy。
         */
        if (g_diag_result_copy_rc == FR_OK) {

            g_diag_result_copy_rc =
                static_cast<int>(
                    f_read(
                        &result_file,
                        result_copy,
                        g_diag_result_size,
                        &result_bytes
                    )
                );


            /*
             * 关闭 result.jpg。
             */
            const FRESULT close_result =
                f_close(
                    &result_file
                );


            /*
             * 如果读取成功，
             * 再检查关闭文件是否成功。
             */
            if (
                g_diag_result_copy_rc ==
                FR_OK
            ) {

                g_diag_result_copy_rc =
                    static_cast<int>(
                        close_result
                    );
            }
        }


        /*
         * 保存 result_copy 的 DDR 地址。
         *
         * JTAG 后续可以读取：
         *
         *      g_diag_result_copy_address
         *
         * 得到 JPEG 文件在 DDR 中的位置。
         */
        g_diag_result_copy_address =
            reinterpret_cast<unsigned int>(
                result_copy
            );


        /*
         * 保存实际读取到 DDR 的字节数。
         */
        g_diag_result_copy_bytes =
            result_bytes;
    }


    /*
     * 0x9000：
     *
     * 表示整个程序已经成功走完。
     */
    g_diag_stage = 0x9000;


    /*
     * 诊断模式下：
     *
     * 即使已经成功，也不会直接 return。
     *
     * 而是停在死循环，
     * 方便通过 JTAG 读取：
     *
     *      检测数量
     *      推理耗时
     *      输出张量
     *      result.jpg 地址
     *      result.jpg 大小
     *
     * 等诊断信息。
     */
    diagnostic_stop(0x9000);

#endif


    /*
     * ------------------------------------------------------------
     * 10. 正常模式下程序执行结束
     * ------------------------------------------------------------
     */
    cout
        << "inference completed"
        << endl;


    /*
     * 返回 0：
     *
     * 表示应用程序正常完成。
     */
    return 0;
}