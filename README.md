# Embedded System Project - Keyboard with Mode Switching

## Tính năng chuyển đổi chế độ

Dự án này hỗ trợ hai chế độ hoạt động:

### 1. COMBO_KEY Mode (Chế độ mặc định)
- Cho phép nhấn nhiều phím cùng lúc
- Sử dụng `keyboard_task()` trong main loop
- Hỗ trợ các tính năng như tap layer, rapid trigger

### 2. SNAPTAP Mode
- Chỉ cho phép một phím được nhấn tại một thời điểm
- Sử dụng `keyboard_task_snaptap()` trong main loop
- Tự động thả phím cũ khi nhấn phím mới

## Cách chuyển đổi chế độ

- **Phím chuyển đổi**: Phím thứ 16 (góc dưới bên phải - MACRO_CTRL_A)
- **Cách sử dụng**: Nhấn phím thứ 16 để chuyển đổi giữa hai chế độ
- **Feedback**: Thông báo chế độ mới sẽ được in ra qua debug console

## Cấu trúc code

### Files chính:
- `firmware/Core/Src/keyboard.c` - Logic chính của keyboard
- `firmware/Core/Src/main.c` - Main loop với logic chuyển đổi chế độ
- `firmware/Core/Src/hid.c` - Xử lý HID reports

### Các hàm quan trọng:
- `keyboard_check_and_toggle_mode()` - Kiểm tra và chuyển đổi chế độ
- `keyboard_get_current_mode()` - Lấy chế độ hiện tại
- `keyboard_task()` - Task cho COMBO_KEY mode
- `keyboard_task_snaptap()` - Task cho SNAPTAP mode

## Lưu ý

- Phím thứ 16 sẽ không thực hiện macro CTRL+A khi được sử dụng để chuyển đổi chế độ
- Chế độ mặc định khi khởi động là COMBO_KEY
- Việc chuyển đổi chế độ được thực hiện trong main loop để đảm bảo độ tin cậy

#HUST Embedded System Project
This is a project for the course of Embedded System Design at HUST.

## Project Overview
This project focus on the design of a analog macro pad using the Blackpill (STM32F401CCU6) microcontroller. 

## Project Structure

### Firmware

### Kicad

## References

- [Minipad](https://github.com/minipadKB/minipad-firmware)
- [Riskeyboard](https://github.com/riskable/void_switch_65_pct)
- [Macrolev](https://github.com/heiso/macrolev/tree/main)
- [Bluepill keyboard](https://github.com/TechTalkies/YouTube/tree/main/38%20STM32%20Keyboard)
- 



