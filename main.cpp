#include <Arduino.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// Ініціалізація дисплея (Тут для SSD1306 OLED)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

// Налаштування до якого Wi-Fi підключатися
const char* ssid = "Ім'я Wi-Fi";
const char* password = "не пароль напевно";

// налаштування NTP
const long utcOffsetInSeconds = 2 * 3600; // Київський час: UTC+2
const char* ntpServer = "pool.ntp.org";

// Об'єкти NTP клієнта
WiFiUDP ntpUDP; // UDP клієнт для NTP
NTPClient timeClient(ntpUDP, ntpServer, utcOffsetInSeconds); // NTP клієнт

// Змінні для збереження дати та часу
unsigned long lastNTPUpdate = 0; // Останнє оновлення NTP
const unsigned long NTP_UPDATE_INTERVAL = 50000; // Оновлення NTP кожні 50 секунд
unsigned long initialMillis; // Початковий час в мілісекундах
unsigned long initialEpochTime; // Початковий час в секундах

// Оновлення часу
void updateTime() {
    timeClient.update();
    lastNTPUpdate = millis();
}
// Налаштування часу
void setupTime() {
    timeClient.begin();
    timeClient.setTimeOffset(utcOffsetInSeconds); // Встановлення зміщення часу для часового поясу (2 години = 7200 секунд)
    updateTime(); // Початкове оновлення часу
}
// Перевірка синхронізації часу
void checkTimeSync() {
    if (millis() - lastNTPUpdate >= NTP_UPDATE_INTERVAL) { // Якщо пройшло більше NTP_UPDATE_INTERVAL мілісекунд
        updateTime(); // Оновити час
    }
}

void setup() {
  Serial.begin(115200);
  
  // Підключення до Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Запуск NTP-клієнта і отриманя початкового часу
  timeClient.begin(); // Запуск NTP-клієнта
  timeClient.update(); // Оновлення часу
  initialEpochTime = timeClient.getEpochTime(); // Отримання початкового часу в секундах
  initialMillis = millis(); // Отримання початкового часу в мілісекундах
  u8g2.begin(); // Ініціалізація дисплея
  u8g2.enableUTF8Print(); // Включення підтримки UTF-8
  setupTime(); // Налаштування часу
}
// Малювання ялинки
void drawTree(uint8_t x, uint8_t y) {
  u8g2.drawTriangle(x, y-10, x-15, y, x+15, y);
  u8g2.drawTriangle(x, y-20, x-12, y-5, x+12, y-5);
  u8g2.drawTriangle(x, y-30, x-8, y-2, x+8, y-2);
  u8g2.drawBox(x-3, y, 6, 5);
}

// Малювання зірки
void drawStar(int centerX, int centerY, int outerRadius, int innerRadius) {
  const int points = 5;  // п'ятикутна зірка
  int x[10], y[10];     // масиви для зберігання координат точок
  // Розрахунок координат точок зірки
  for (int i = 0; i < points * 2; i++) {
    float angle = i * PI / points + PI / 2; // кут точки
    int radius = (i % 2 == 0) ? outerRadius : innerRadius; // радіус точки
    x[i] = centerX + radius * cos(angle); // координата x
    y[i] = centerY + radius * sin(angle); // координата y
  }
  // З'єднання точок лініями для створення зірки
  for (int i = 0; i < points * 2; i++) {
    int nextPoint = (i + 1) % (points * 2);                // наступна точка
    u8g2.drawLine(x[i], y[i], x[nextPoint], y[nextPoint]); // малювання лінії
  }
}
// Структура частинки
struct Particle {
  int16_t x, y;
  int16_t dx, dy;
  uint8_t life;
};
#define NUM_PARTICLES 2025/65
Particle particles[NUM_PARTICLES];
// Ініціалізація феєрверку
void initFirework(uint8_t x, uint8_t y) {
  for(int i = 0; i < NUM_PARTICLES; i++) {
    particles[i].x = x << 4; // Фіксована точка для більш плавного руху по осі x
    particles[i].y = y << 4; // Фіксована точка для більш плавного руху по осі y
    float angle = random(360) * PI / 180.0; // Випадковий кут
    uint8_t speed = random(2, 10); // Випадкова швидкість
    particles[i].dx = cos(angle) * speed * 16; // Швидкість по осі x
    particles[i].dy = sin(angle) * speed * 16; // Швидкість по осі y
    particles[i].life = random(20, 40); // Випадкова тривалість життя
  }
}
// Оновлення та малювання феєрверків
void updateFirework() {
  for(int i = 0; i < NUM_PARTICLES; i++) {
    if(particles[i].life > 0) { // Якщо частинка жива
      particles[i].x += particles[i].dx; // Рух по осі x
      particles[i].y += particles[i].dy; // Рух по осі y
      particles[i].dy += 4; // Гравітація
      particles[i].life--; // Зменшення тривалості життя
      u8g2.drawPixel(particles[i].x >> 4, particles[i].y >> 4); // Малювання частинки
    }
  }
}
bool isFirework = false; // Чи запускати феєрверк
void loop() {
  // Розрахувати поточний час на основі початкового часу та часу, що минув
  unsigned long currentMillis = millis(); // Поточний час в мілісекундах
  unsigned long elapsedMillis = currentMillis - initialMillis; // Час, що минув з початку програми
  unsigned long currentEpochTime = initialEpochTime + elapsedMillis / 1000; // Поточний час в секундах
  // Перетворення часу епохи (секунд) на форматований час
  time_t rawTime = currentEpochTime; // Час епохи
  struct tm * timeInfo = localtime(&rawTime); // Часова структура
  char formattedTime[9]; // Масив для зберігання форматованого часу
  strftime(formattedTime, 9, "%H:%M:%S", timeInfo); // Перетворення часу

  static uint32_t lastFirework = 0; // Час останнього феєрверку
  static bool fireActive = false; // Чи активний феєрверк
  
  u8g2.clearBuffer(); // Очистка буфера дисплея
  
  drawTree(64, 58); // Малювання ялинки
  
  drawStar(64, 25, 3, 8); // Малювання зірки на вершині ялинки

  // Перевірка часу для запуску феєрверку о 00:00 годині (новий рік)
  if (strncmp(formattedTime, "00:00", 5) == 0) {isFirework = true;}  

  // Малювання тексту на дисплеї
  u8g2.setFont(u8g2_font_cu12_t_cyrillic); // Встановлення шрифту
  if (isFirework) { // Якщо запущений феєрверк
    u8g2.setCursor(0, 64); // Встановлення позиції тексту на дисплеї (0, 64)
    u8g2.print("З Новим"); // Виведення тексту "З Новим"
    u8g2.setCursor(84, 64); // Встановлення позиції тексту на дисплеї (84, 64)
    u8g2.print("Роком!"); // Виведення тексту "Роком!"
  }
  if (!isFirework) { // Якщо феєрверк не запущений
    u8g2.setCursor(36, 12); // Встановлення позиції тексту на дисплеї (36, 12)
    u8g2.print(formattedTime); // Виведення форматованого часу
  }
  // Запуск феєрверку
  if (isFirework && !fireActive) {
    initFirework(random(30, 98), random(10, 30)); // Ініціалізація феєрверку
    fireActive = true; // Активувати феєрверк
    lastFirework = millis(); // Зберегти час останнього феєрверку
  }
  if(fireActive) { // Якщо феєрверк активний
    updateFirework(); // Оновлення феєрверку
    bool anyAlive = false; // Чи є живі частинки
    for(int i = 0; i < NUM_PARTICLES; i++) { // Перевірка частинок
      if(particles[i].life > 0) anyAlive = true; // Якщо частинка жива - встановити флаг
    }
    fireActive = anyAlive; // Встановити флаг активності феєрверку
  }
  u8g2.sendBuffer(); // Відправка буфера на дисплей
  delay(20); // Затримка для плавності анімації
  checkTimeSync(); // Перевірка синхронізації часу
}