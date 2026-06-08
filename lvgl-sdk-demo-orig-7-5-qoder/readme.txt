Current project keeps camera preview, UART agreement, vin_test, and the root g2d files.
LVGL core, LVGL display/input drivers, LVGL configuration, LVGL UI code, LVGL touch adapter, and old LVGL demo assets have been removed.
lv_g2d_test/src now builds camera_uart_test for camera preview and UART agreement testing.

当前项目保留了摄像头预览、UART 协议、VIN 测试以及根目录下的 g2d 文件。 
LVGL 核心、LVGL 显示/输入驱动程序、LVGL 配置、LVGL Ul 代码、LVGL 触摸适配器以及旧版 LVGL 演示资源均已删除。
 Iv_g2d_test/src 目录现在已构建了 camera_uart_test，用于实现摄像头预览和 UART 协议测试。


分离了原 lv_g2d_test 的 lvgl8.3 
仅仅运行 lvgl9.5 可行
lvgl 9.5还没有移植进入工程

lvgl9.5已经移植计入工程，可以使用
但是 lvgl9.5 + camera 还没测试，这里的透明背景设置可能和lvgl8.3不一样 

1、lvgl9.5+camera+uart协议。
2、tinyxml2+freetype可以显示中文简体、中文繁体、英文、马来西亚文、俄文；不能显示韩文、越南文、阿拉伯文。
3、设置lvgl9.5背景透明可以显示camera

root@TinaLinux:/# find -name "*.ttf"
./usr/res/font/msyh.ttf
root@TinaLinux:/# find /usr/lib -name "libfreetype*"
/usr/lib/libfreetype.so.6
/usr/lib/libfreetype.so.6.17.0

/home/xiaozhi/Downloads/f133/mx-hxx003/tina5.0-f133/out/f133-mx-hxx/prototype/openwrt/staging_dir/target/usr/lib -lfreetype

添加了触摸，main中有按钮测试

使用qoder尝试移植 cpp-ipc-1.4.1 




























