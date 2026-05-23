# 普中103摄像头显示与拍照作业

## 功能

- 电脑摄像头或手机 IP 摄像头采集视频。
- Python 上位机把画面缩放为 160x120 RGB565，通过 USART1 按行发送到普中 103。
- STM32F103 通过 FSMC 驱动 320x240 TFT，把视频 2 倍放大显示到屏幕。
- 在上位机窗口按 `p` 拍照，照片保存到 `host_tools/photos`。

## 硬件连接

- 普中 103 开发板 USART1：PA9/TX、PA10/RX，连接 USB 转串口或板载串口。
- TFT LCD 使用普中 103 常见 16 位 FSMC 并口接口。
- 串口波特率默认 `230400`，与成功案例 `camera_to_tft.py` 保持一致。

## 使用方法

1. 用 Keil 打开 `Template.uvprojx`，编译并下载到普中 103。
2. 安装电脑端依赖：

```bash
pip install -r host_tools\requirements.txt
```

3. 运行电脑摄像头：

```bash
python host_tools\camera_to_tft.py --port COM5 --camera 0
```

下载新固件后，LCD 会先显示测试色条；运行 Python 后，摄像头画面会逐行覆盖到 LCD 上。

4. 使用手机摄像头时，先让手机和电脑连接同一个 Wi-Fi，在手机打开 IP Webcam / DroidCam 一类软件，再运行：

```bash
python host_tools\camera_to_tft.py --port COM5 --camera http://手机IP:端口/video
```

5. 上位机预览窗口中按 `p` 拍照，按 `q` 退出。

如果电脑端在发送但 LCD 没有变化，先把每行发送间隔调大：

```bash
python host_tools\camera_to_tft.py --port COM5 --camera 0 --line-delay 0.01
```

## 作业提交建议

提交作业时建议包含以下材料：

1. Keil 工程文件夹，包含 `Template.uvprojx`、`User`、`APP`、`Public`、`Libraries`、`host_tools`。
2. 拍照结果：`host_tools/photos/photo_*.jpg`。
3. 一张或一段演示材料：本人出现在摄像头画面中，同时普中 103 屏幕正在显示该画面。

如果串口不稳定，把 `User/main.c` 中 `USART1_Init(230400)` 和脚本参数同时改低，例如：

```bash
python host_tools\camera_to_tft.py --port COM5 --baud 115200 --camera 0
```

## 串口协议

- 视频行包：`A5 5A 01 line_index length_hi length_lo rgb565_line checksum`
- 每行 `160` 个像素，每个像素 `2` 字节，大端 RGB565，所以 `length=320`。
- 校验：`packet_type + line_index + length_hi + length_lo + payload` 的低 8 位。
