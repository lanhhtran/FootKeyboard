#include "keymatrix.h"
#include "BleKeyboardBuilder.h"
#include "ssd1306.h"
#include "configuration.h"
#include <Arduino.h>
// GPIO pins cho ma trận
const uint8_t row_pins[MATRIX_ROWS] = {ROW_0_PIN, ROW_1_PIN, ROW_2_PIN, ROW_3_PIN};
const uint8_t col_pins[MATRIX_COLS] = {COL_0_PIN, COL_1_PIN, COL_2_PIN, COL_3_PIN};
// Ma trận 16 phím (4x4)
MatrixKey matrix_keys[16] = {
    // Hàng 0
    {0, 0, KEY_RELEASED, 0, true, "A"},  // Key 0 PIN
    {0, 1, KEY_RELEASED, 0, false, "3"}, // Key 1
    {0, 2, KEY_RELEASED, 0, false, "2"}, // Key 2
    {0, 3, KEY_RELEASED, 0, false, "1"}, // Key 3

    // Hàng 1
    {1, 0, KEY_RELEASED, 0, true, "B"},  // Key 4 PIN
    {1, 1, KEY_RELEASED, 0, false, "6"}, // Key 5
    {1, 2, KEY_RELEASED, 0, false, "5"}, // Key 6
    {1, 3, KEY_RELEASED, 0, false, "4"}, // Key 7

    // Hàng 2
    {2, 0, KEY_RELEASED, 0, true, "C"},  // Key 8 PIN
    {2, 1, KEY_RELEASED, 0, false, "9"}, // Key 9
    {2, 2, KEY_RELEASED, 0, false, "8"}, // Key 10
    {2, 3, KEY_RELEASED, 0, false, "7"}, // Key 11

    // Hàng 3
    {3, 0, KEY_RELEASED, 0, true, "D"},  // Key 12 PIN
    {3, 1, KEY_RELEASED, 0, false, "#"},  // Key 13
    {3, 2, KEY_RELEASED, 0, false, "0"}, // Key 14
    {3, 3, KEY_RELEASED, 0, false, "*"}   // Key 15 
};

// Biến trạng thái
bool matrix_previous_state[MATRIX_ROWS][MATRIX_COLS];
bool matrix_current_state[MATRIX_ROWS][MATRIX_COLS];

// Biến theo dõi thời gian hoạt động cuối cùng của matrix
unsigned long last_matrix_activity_time = 0;

// Biến cho chế độ T9 (Mode C)
bool mode_t9_active = false;
uint8_t t9_current_key = 255;          // Phím hiện tại đang được nhấn (255 = không có phím nào)
uint8_t t9_press_count = 0;            // Số lần nhấn phím hiện tại
unsigned long t9_last_press_time = 0;  // Thời gian lần nhấn cuối cùng
const unsigned long T9_TIMEOUT = 1500; // Timeout 1.5 giây giữa các lần nhấn

// Bảng T9 mapping chuẩn
const char *t9_mapping[] = {
    " ",        // 0 - Dấu cách
    ".,?!@-/:", // 1 - Dấu câu & đặc biệt
    "abc",      // 2
    "def",      // 3
    "ghi",      // 4
    "jkl",      // 5
    "mno",      // 6
    "pqrs",     // 7
    "tuv",      // 8
    "wxyz",     // 9
    "w*",       // 10 - Phím * (có thể thêm ký tự khác)
    "sfrxj#"    // 11 - Phím # (có thể thêm ký tự khác)
};

// Biến theo dõi trạng thái của các phím T9
struct T9KeyStatus
{
    bool active;                 // Phím có đang được nhấn giữ không
    unsigned long lastActivated; // Thời gian phím được nhấn lần cuối
};

T9KeyStatus t9_key_status[10]; // Trạng thái cho từng phím từ 0 đến 9

extern BleKeyboardBuilder bleKeyboardBuilder;

/**
 * @brief Khởi tạo ma trận bàn phím
 * Rows (hàng): Làm OUTPUT, mặc định HIGH, từng lúc sẽ kéo từng hàng xuống LOW khi quét.
 * Cols (cột): Làm INPUT_PULLUP, bình thường ở HIGH, nhấn phím sẽ bị kéo xuống LOW nếu hàng tương ứng đang ở LOW.
 */
void setup_keymatrix()
{
    Serial.begin(115200);
    // Cấu hình các chân hàng (Row) làm OUTPUT
    for (int i = 0; i < MATRIX_ROWS; i++)
    {
        pinMode(row_pins[i], OUTPUT);
        digitalWrite(row_pins[i], HIGH); // Mặc định HIGH
    }

    // Cấu hình các chân cột (Column) làm INPUT_PULLUP
    for (int i = 0; i < MATRIX_COLS; i++)
    {
        pinMode(col_pins[i], INPUT_PULLUP);
    }

    // Khởi tạo trạng thái ma trận
    for (int row = 0; row < MATRIX_ROWS; row++)
    {
        for (int col = 0; col < MATRIX_COLS; col++)
        {
            matrix_previous_state[row][col] = HIGH;
            matrix_current_state[row][col] = HIGH;
        }
    }
    // Khởi tạo trạng thái cho các phím trong ma trận
    for (int i = 0; i < 16; i++)
    {
        if (matrix_keys[i].isPinnable)
        {
            matrix_keys[i].state = KEY_RELEASED;
        }
    }
    // Khởi tạo trạng thái cho các phím T9
    for (int i = 0; i < 10; i++)
    {
        t9_key_status[i].active = false;
        t9_key_status[i].lastActivated = 0;
    }

    Serial.println("Keymatrix 4x4 initialized");
}

/**
 * @brief Quét ma trận phím trong vòng lặp chính
 */
void loop_keymatrix()
{
    static unsigned long lastScanTime = 0;

    // Quét mỗi 10ms để tránh debouncing
    if (millis() - lastScanTime < 10)
    {
        return;
    }
    lastScanTime = millis();

    // Quét toàn bộ ma trận
    for (int row = 0; row < MATRIX_ROWS; row++)
    {
        // Kích hoạt hàng hiện tại (LOW)
        digitalWrite(row_pins[row], LOW);
        delayMicroseconds(10); // Chờ ổn định

        // Đọc tất cả các cột
        for (int col = 0; col < MATRIX_COLS; col++)
        {
            matrix_previous_state[row][col] = matrix_current_state[row][col];
            matrix_current_state[row][col] = digitalRead(col_pins[col]);

            // Phát hiện sườn xuống (phím được bấm)
            if (matrix_previous_state[row][col] == HIGH &&
                matrix_current_state[row][col] == LOW)
            {

                uint8_t keyIndex = row * MATRIX_COLS + col;
                // Phím A pin thì vô hiệu hóa tất cả trừ A, B, C, D
                if (matrix_keys[0].state == KEY_PINNED)
                {
                    if (keyIndex != 0 && keyIndex != 4 && keyIndex != 8 && keyIndex != 12)
                    {
                        break;
                    }
                    else
                        handle_key_press(keyIndex);
                }
                else // Nếu phím A không pin thì vô hiệu hóa các phím B, C, D
                {
                    if (keyIndex == 4 || keyIndex == 8 || keyIndex == 12)
                    {
                        if (matrix_keys[keyIndex].state != KEY_PINNED)
                        {
                            Serial.println("Phím B/C/D không thể bấm khi A không được PINNED!");
                            break; // Không xử lý phím B/C/D nếu A không pin
                        }
                        else
                        {
                            // xử lý khi B/C/D đang pin -> Bấm A thì bỏ pin BCD
                            if (keyIndex == 0)
                            {
                                matrix_keys[4].state = KEY_RELEASED;
                                matrix_keys[8].state = KEY_RELEASED;
                                matrix_keys[12].state = KEY_RELEASED;
                            }
                        }
                    }
                    handle_key_press(keyIndex);
                }
            }
        }

        // Tắt hàng hiện tại (HIGH)
        digitalWrite(row_pins[row], HIGH);
    }
}

/**
 * @brief Xử lý khi có phím được bấm
 */
void handle_key_press(uint8_t keyIndex)
{
    if (keyIndex >= 16)
        return;

    MatrixKey *key = &matrix_keys[keyIndex];
    unsigned long currentTime = millis();

    // Cập nhật thời gian hoạt động cuối cùng của matrix
    last_matrix_activity_time = currentTime;

    // Debouncing: Bỏ qua nếu bấm quá nhanh
    if (currentTime - key->lastPressTime < 50)
    {
        return;
    }
    key->lastPressTime = currentTime;
    Serial.print("Key pressed: ");
    Serial.print(key->label);
    Serial.print(" (");
    Serial.print(keyIndex);
    Serial.println(")");

    // Xử lý phím có thể cố định
    if (key->isPinnable)
    {
        toggle_pin_key(keyIndex);
    }
    else
    {
        // Phím thường: gửi ngay
        print_key_to_computer(keyIndex);
    }

    // Hiển thị trên OLED
    show_matrix_key(keyIndex, key->state);
}

/**
 * @brief Toggle trạng thái cố định của phím
 */
void toggle_pin_key(uint8_t keyIndex)
{
    MatrixKey *key = &matrix_keys[keyIndex];

    if (key->state == KEY_RELEASED)
    {
        if ((keyIndex == 4 || keyIndex == 8 || keyIndex == 12) && matrix_keys[0].state != KEY_PINNED)
        {
            Serial.println("Bạn phải PIN phím A trước khi PIN được B, C hoặc D!");
            return; // Không cho phép PIN B/C/D khi A chưa PINNED
        }
        // Từ RELEASED về PINNED
        key->state = KEY_PINNED;
        Serial.print("Key ");
        Serial.print(key->label);
        Serial.println(" PINNED");

        if (keyIndex == 0)
        {
            // Phím A được PIN - cho phép sử dụng B,C,D và đồng thường tự động unpin B,C,D
            Serial.println("Key A PINNED - B,C,D are now enabled");
            matrix_keys[4].state = KEY_RELEASED;  // B
            matrix_keys[8].state = KEY_RELEASED;  // C
            matrix_keys[12].state = KEY_RELEASED; // D
        }
        else if (keyIndex == 4 || keyIndex == 8 || keyIndex == 12)
        {
            // Phím B, C hoặc D được PIN - tự động unpin A
            if (matrix_keys[0].state == KEY_PINNED)
            {
                matrix_keys[0].state = KEY_RELEASED;
                Serial.println("Key A auto-UNPINNED because B/C/D was pinned");
                // show_matrix_key(0, KEY_RELEASED); // Cập nhật hiển thị cho A
            }

            // Xử lý chức năng riêng của từng phím
            if (keyIndex == 4)
            {
                Serial.println("Key B PINNED - Special function B activated");
                // SerialConfiguration("00=YES");
                // SerialConfiguration("01=NO");
                // SerialConfiguration("02={ENTER}");
                // SerialConfiguration("03={BACKSPACE}");
            }
            else if (keyIndex == 8)
            {
                // Phím C - kích hoạt T9
                mode_t9_active = true;
                Serial.println("Key C PINNED - T9 Mode activated!");
                // process_t9(keyIndex); // Kích hoạt chế độ T9
            }
            else if (keyIndex == 12)
            {
                Serial.println("Key D PINNED - Special function D activated");
                // Thêm chức năng cho phím D ở đây
            }
        }
    }
    else
    {
        // Từ PINNED về RELEASED
        key->state = KEY_RELEASED;
        Serial.print("Key ");
        Serial.print(key->label);
        Serial.println(" UNPINNED");

        // Kích hoạt mode tương ứng
        if (keyIndex == 8)
        {
            // Kích hoạt T9 mode cho phím C
            mode_t9_active = false;
            Serial.println("T9 Mode disabled!");
        }
        if (keyIndex == 4)
        {
            // SerialConfiguration("00={LEFT}");
            // SerialConfiguration("01={RIGHT}");
            // SerialConfiguration("02={UP}");
            // SerialConfiguration("03={DOWN}");
            Serial.println("Factory settings restored!");
        }

        // Gửi key press và giữ
        print_key_to_computer(keyIndex);
    }

    show_matrix_key(keyIndex, key->state);
}

/**
 * @brief Gửi ký tự về máy tính
 */
void print_key_to_computer(uint8_t keyIndex)
{
    if (keyIndex >= 16 || keyIndex == 0 || keyIndex == 4 || keyIndex == 8 || keyIndex == 12)
        return;

    MatrixKey *key = &matrix_keys[keyIndex];
    if (matrix_keys[8].state == KEY_PINNED)
    {
        process_t9(keyIndex); // Nếu đang ở chế độ T9, xử lý nhập liệu T9
    }
    else
    {
        if (bleKeyboardBuilder.isConnected())
        {
            bleKeyboardBuilder.print(key->label);
            bleKeyboardBuilder.SendKeys(key->label);
            Serial.print("Sent to computer: ");
            Serial.println(key->label);
        }
        else
        {
            Serial.println("BLE not connected");
        }
    }
}

/**
 * @brief Kiểm tra xem có phím matrix nào đang được bấm không
 * @return true nếu có phím đang được bấm, false nếu không
 */
bool is_matrix_active()
{
    for (int row = 0; row < MATRIX_ROWS; row++)
    {
        for (int col = 0; col < MATRIX_COLS; col++)
        {
            if (matrix_current_state[row][col] == LOW) // LOW có nghĩa là phím đang được bấm
            {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Xử lý toàn bộ logic T9
 */
/**
 * @brief Xử lý T9 Realtime - gửi ký tự ngay khi nhấn (preview trực tiếp)
 *
 * Logic mới:
 * - Lần 1: Gửi ký tự đầu tiên (vd: 'a')
 * - Lần 2: Backspace + gửi ký tự thứ 2 (xóa 'a', gửi 'ă')
 * - Lần 3: Backspace + gửi ký tự thứ 3 (xóa 'ă', gửi 'â')
 * - Và cứ thế tiếp tục cycle...
 *
 * @param keyIndex Index của phím được nhấn (255 = không có phím)
 */
void process_t9(uint8_t keyIndex)
{
    Serial.println("Processing T9...");
    if (!mode_t9_active)
        return;
    Serial.println("Processing T9...2");
    unsigned long currentTime = millis();

    // Nếu có keyIndex, nghĩa là có phím được nhấn
    if (keyIndex != 255)
    {
        // Chuyển đổi keyIndex thành digit (0-11)
        uint8_t digit = 255;

        switch (keyIndex)
        {
        case 14:
            digit = 0;
            break; // Key "0"
        case 3:
            digit = 1;
            break; // Key "1"
        case 2:
            digit = 2;
            break; // Key "2"
        case 1:
            digit = 3;
            break; // Key "3"
        case 7:
            digit = 4;
            break; // Key "4"
        case 6:
            digit = 5;
            break; // Key "5"
        case 5:
            digit = 6;
            break; // Key "6"
        case 11:
            digit = 7;
            break; // Key "7"
        case 10:
            digit = 8;
            break; // Key "8"
        case 9:
            digit = 9;
            break; // Key "9"
        case 15:
            digit = 10;
            break; // Key "*"
        case 13:
            digit = 11;
            break; // Key "#"
        default:
            return;
        }

        // Kiểm tra timeout hoặc phím khác
        if ((currentTime - t9_last_press_time > T9_TIMEOUT) || (t9_current_key != digit))
        {
            // Phím mới hoặc timeout - không cần xóa ký tự cũ
            t9_current_key = digit;
            Serial.println("nhan phim" + String(digit));
            t9_press_count = 1;
        }
        else
        { // Cùng phím - XÓA ký tự cũ trước khi gửi ký tự mới
            if (bleKeyboardBuilder.isConnected() && t9_press_count > 0)
            {
                // Gửi backspace để xóa ký tự cũ
                char backspace_cmd[] = {0x08, 0}; // ASCII backspace
                bleKeyboardBuilder.SendKeys(backspace_cmd);
                delay(50); // Đợi xóa hoàn tất
                Serial.println("T9: Backspace sent (delete previous char)");
            }

            // Tăng số lần nhấn
            t9_press_count++;
        }

        // GỬI ký tự mới NGAY LẬP TỨC
        if (digit < (sizeof(t9_mapping) / sizeof(t9_mapping[0])))
        {
            const char *chars = t9_mapping[digit];
            uint8_t char_count = strlen(chars);

            if (char_count > 0)
            {
                // Lấy ký tự theo số lần nhấn (cycle nếu vượt quá)
                uint8_t char_index = (t9_press_count - 1) % char_count;
                char selected_char = chars[char_index];

                if (bleKeyboardBuilder.isConnected())
                {
                    char char_str[2] = {selected_char, '\0'};
                    bleKeyboardBuilder.SendKeys(char_str);
                    Serial.print("T9 realtime sent: ");
                    Serial.print(selected_char);
                    Serial.print(" (press ");
                    Serial.print(t9_press_count);
                    Serial.println(")");
                }
            }
        }

        t9_last_press_time = currentTime;

        Serial.print("T9: Key ");
        Serial.print(digit);
        Serial.print(" pressed ");
        Serial.print(t9_press_count);
        Serial.println(" times");
    }
    else
    {
        // Kiểm tra timeout - Reset state chứ không gửi ký tự
        if (t9_current_key != 255 && (currentTime - t9_last_press_time > T9_TIMEOUT))
        {
            Serial.println("T9 timeout - reset state (no char sent)");

            // Reset sau timeout - không gửi ký tự vì đã gửi realtime rồi
            t9_current_key = 255;
            t9_press_count = 0;
        }
    }
}
