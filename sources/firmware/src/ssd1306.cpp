/**************************************************************************
 This is an example for our Monochrome OLEDs based on SSD1306 drivers

 Pick one up today in the adafruit shop!
 ------> http://www.adafruit.com/category/63_98

 This example is for a 128x32 pixel display using I2C to communicate
 3 pins are required to interface (two I2C and one reset).

 Adafruit invests time and resources providing this open
 source code, please support Adafruit and open-source
 hardware by purchasing products from Adafruit!

 Written by Limor Fried/Ladyada for Adafruit Industries,
 with contributions from the open source community.
 BSD license, check license.txt for more information
 All text above, and the splash screen below must be
 included in any redistribution.
 **************************************************************************/

#include "ssd1306.h"
#include "keymatrix.h"

#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/**
 * @brief Hiển thị màn hình khi khởi động
 */
void welcome_screen(char SerialCommand[])
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0); // Start at top-left corner
  display.print(F("Tran Thi Lan Anh"));

  // Hiển thị tên mạng BLE mà thiết bị phát ra
  display.setCursor(0, 18);
  if (SerialCommand == nullptr || SerialCommand[0] == '\0')
  {
    display.print(F("Default_BLE"));
  }
  else
  {
    display.print(F("BLE :"));
    display.print(SerialCommand);
  }

  display.display();
  delay(2000); // Giữ màn hình trong 3 giây

  display.clearDisplay(); // Xoá màn hình sau 2 giây
  display.display();      // Cập nhật để áp dụng xoá
}

/**
 * @brief Hiển thị màn hình chào mừng sau khi thức dậy
 */
void welcome_screen_afterwakeup(void)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 2);
  display.print(F("Wake up from sleep"));

  display.setCursor(0, 10);
  display.print(F("Ready to use"));

  display.display();
  delay(2000); // Giữ màn hình trong 2 giây

  display.clearDisplay();
  display.display();
}

/**
 * @brief Hiển thị màn hình sleep mode
 */
void show_sleep_screen(void)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 2);
  display.print(F("SLEEP MODE IN :"));
  // Hướng dẫn với text trắng
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 12);
  display.print(F("Press pedal 00"));
  display.setCursor(0, 20);
  display.print(F("to wake up"));
  for (int i = 3; i >= 0; i--)
  {
    oled_show_number(i); // Hàm tự viết, hiện số lên màn hình
    delay(1000);         // Chờ 1 giây
  }
  display.display();
  delay(100);
  display.clearDisplay();
  display.display(); // Cập nhật để áp dụng xoá
}
void main_screen(void)
{
  display.clearDisplay();

  display.setTextSize(2);                             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.setCursor(0, 0);                            // Start at top-left corner
  display.print(F(" Foot "));

  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setTextSize(1);              // Normal 1:1 pixel scale
  display.println(F(" keyboard"));
  display.setCursor(75, 12); // Start at top-left corner
  display.println(F("//--\\\\"));

  display.setTextSize(1); // Draw 2X-scale text

  display.print(F("Lan Anh 20215180"));

  display.display();

  /*
  void show_title(const char * title) {
  //Hiển thị trên oled
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(title);  //10 kí tự
  display.setTextSize(2);
  display.setCursor(0, ROW_OF_INFOR);
  display.display();
  */
}

void setup_oled()
{
  Wire.begin(PIN_SDA, PIN_SCL);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();

  // Xoay 180*
  // display.setRotation(0);

  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();
  delayMicroseconds(200 * 1000);
  main_screen(); // Draw 'stylized' characters

  delayMicroseconds(200 * 1000);
}

void show_pedal(unsigned char no)

{
  display.fillRect(105, 0, 24, 24, SSD1306_BLACK);
  display.drawChar(105, 0, (no + 0x30), SSD1306_WHITE, SSD1306_BLACK, 3);
  display.display();
}

void oled_show_number(int n)
{
  display.fillRect(100, 2, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_BLACK);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(100, 2);
  display.print(n);
  display.display();
}

/**
 * @brief Hiển thị phím ma trận được bấm
 */
void show_matrix_key(uint8_t keyIndex, KEY_STATE state)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // 1. Ưu tiên: Nếu phím A đang PINNED, hiển thị menu đặc biệt
  if (matrix_keys[0].state == KEY_PINNED)
  {
    display.setCursor(0, 0);
    display.print(F("MENU: "));
    display.setCursor(0, 9);
    display.println(F("B: YESNO MODE"));
    display.println(F("C: TEXT ON 9KEYS MODE"));
    display.println(F("D: D MODE"));
    display.display();
    return;
  }

  // 2. Nếu đang ở Mode B/C/D (phím B/C/D đang PINNED), hiển thị tên mode ở đầu
  if (matrix_keys[4].state == KEY_PINNED)
  {
    display.setCursor(0, 0);
    display.print(F("Mode B"));
  }
  else if (matrix_keys[8].state == KEY_PINNED)
  {
    display.setCursor(0, 0);
    display.print(F("Mode C"));
  }
  else if (matrix_keys[12].state == KEY_PINNED)
  {
    display.setCursor(0, 0);
    display.print(F("Mode D"));
  }

  // 3. Thông tin phím được thao tác (hiển thị ở dòng tiếp theo)
  display.setTextSize(1);
  display.setCursor(0, 12);
  display.print(F("Key: "));
  display.print(matrix_keys[keyIndex].label);

  // 4. Trạng thái phím
  display.setCursor(48, 12);
  switch (state)
  {
  case KEY_PRESSED:
    display.print(F("PRESSED"));
    break;
  case KEY_PINNED:
    display.print(F("PINNED ON"));
    break;
  default:
    if (keyIndex == 0 || keyIndex == 4 || keyIndex == 8 || keyIndex == 12)
    {
      display.print(F("UNPINNED"));
    }
    else
    {
      display.print(F("RELEASED"));
    }
    break;
  }

  // 5. Nếu là các phím đặc biệt (A/B/C/D) và vừa bị UNPINNED thì báo reset phía dưới
  if (state != KEY_PINNED && (keyIndex == 0 || keyIndex == 4 || keyIndex == 8 || keyIndex == 12))
  {
    display.setCursor(0, 22);
    display.print(F("FootKeyboard Reset!"));
    display.display();
    delay(1000);
    display.clearDisplay();
  }

  display.display();
}

/**
 * @brief Hiển thị preview ký tự T9 đang nhập
 */
void show_t9_preview(uint8_t digit, uint8_t press_count)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Hiển thị tiêu đề
  display.setCursor(0, 0);
  display.print(F("T9 Mode - Key: "));
  display.print(digit);
  display.display();
}