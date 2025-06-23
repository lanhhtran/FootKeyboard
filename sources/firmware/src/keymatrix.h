#pragma once
#include <Arduino.h>

#define MATRIX_ROWS 4
#define MATRIX_COLS 4

// GPIO cho các hàng (Row)
#define ROW_0_PIN 32
#define ROW_1_PIN 33
#define ROW_2_PIN 25
#define ROW_3_PIN 26

// GPIO cho các cột (Column)
#define COL_0_PIN 13
#define COL_1_PIN 12
#define COL_2_PIN 14
#define COL_3_PIN 27

// Trạng thái phím
enum KEY_STATE
{
    KEY_RELEASED = 0, // Trạng thái nhả ( không bấm)
    KEY_PRESSED = 1,  // Trạng thái vừa đc bấm
    KEY_PINNED = 2    // Trạng thái cố định
};

// Cấu trúc thông tin phím
struct MatrixKey
{
    uint8_t row;
    uint8_t col;
    KEY_STATE state;
    unsigned long lastPressTime; // Thời điểm gần nhất phím được nhấn (millis)
    bool isPinnable;             // Phím này có cho phép ghim (pin) không? true/false
    char label[16];              // Nhãn hiển thị trên OLED
};

// EXTERN: Khai báo biến toàn cục
extern MatrixKey matrix_keys[16];
extern unsigned long last_matrix_activity_time; // Thời gian hoạt động cuối cùng của matrix

// Khai báo các hàm
void setup_keymatrix();
void loop_keymatrix();
uint8_t scan_matrix();
void handle_key_press(uint8_t keyIndex);
void toggle_pin_key(uint8_t keyIndex);
void print_key_to_computer(uint8_t keyIndex);
void process_t9(uint8_t keyIndex);
bool is_matrix_active(); // Kiểm tra xem có phím matrix nào đang được bấm không