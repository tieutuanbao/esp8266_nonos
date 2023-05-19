# Project folder lập trình cho ESP8266
# 1 - Tải về và giải nén vào ổ C:/Espressif: 
https://dl.espressif.com/dl/xtensa-lx106-elf-gcc8_4_0-esp-2020r3-win32.zip
# 2 - Cài đặt MSYS2, và làm theo hướng dẫn để cài các package (make, gcc) theo các bước (1-9) (Chỉ cần làm đến bước 7):
https://www.msys2.org/
# 3 - Cài đặt python 2.7.18 sau đó cài đặt các package:
python -m pip install pyserial
# 3 - Clone Example Project:
https://github.com/tieutuanbao/esp8266_example_project.git
# 4 - Chạy các lệnh sau:
> cd esp8266_example_project
> git clone --recursive https://github.com/espressif/ESP8266_NONOS_SDK.git
# 5 - Cấu hình các thông số cần thiết trong makefile:

# 6 - Compile và Nạp:
- Sử dụng git bash để chạy lệnh:
    + make clean        // Clean project
    + make              // Build project
    + make flash        // Nạp chương trình vào esp, cấu hình cổng COM trong makefile trước khi nạp
    + make unbrick      // Unbrick project trong trường hợp trước đó esp đã nạp code arduino
    + make buildfs      // Tạo file bin từ dữ liệu trong ./data
    + make uploadfs     // Nạp file bin được tạo từ lệnh buildfs vào flash
# Lỗi:
- builtint trong file monitor:
    + python -m pip install future