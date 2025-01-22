#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#include <esp32_smartdisplay.h>
#include <ui/ui.h>
#include <ui/screens.h>
#include <ui/actions.h>

const char *ssid = "****";
const char *password = "****";

// MQTT настройки
const char *mqtt_server = "192.168.1.50"; // IP-адрес или домен вашего MQTT-брокера
const char *mqtt_topic = "ioc/tele/BatteryControl/SENSOR";

// MQTT клиент
WiFiClient espClient;
PubSubClient client(espClient);



void sendMQTTMessage(const char *topic, const char *message)
{
    if (client.connected())
    {
        client.publish(topic, message);
        Serial.print("Сообщение отправлено в топик: ");
        Serial.println(topic);
    }
    else
    {
        Serial.println("MQTT клиент не подключен. Попробуйте подключиться заново.");
    }
}

void action_on_top_light(lv_event_t *e)
{
    sendMQTTMessage("ioc/cmnd/CarLight/POWER1", "TOGGLE");
}

void action_on_boot_light(lv_event_t *e)
{
    sendMQTTMessage("ioc/cmnd/CarLight/POWER2", "TOGGLE");
}

void action_on_side_light(lv_event_t *e)
{
    sendMQTTMessage("ioc/cmnd/CarLight/POWER3", "TOGGLE");
}

void callback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Получено сообщение на топик: ");
    Serial.println(topic);

    // Преобразуем payload в строку
    String message;
    for (unsigned int i = 0; i < length; i++)
    {
        message += (char)payload[i];
    }
    Serial.print("Сообщение: ");
    Serial.println(message);

    // Разбираем JSON
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error)
    {
        Serial.print("Ошибка парсинга JSON: ");
        Serial.println(error.c_str());
        return;
    }

    float voltage_gen = doc["INA226-1"]["Voltage"];
    float amper_charge = doc["INA226-1"]["Current"];

    float voltage_bat = doc["INA226-3"]["Voltage"];
    float amper_consume = doc["INA226-3"]["Current"];

    lv_label_set_text_fmt(objects.voltage_gen, "%.3f", voltage_gen);
    lv_label_set_text_fmt(objects.amper_charge, "%.3f", amper_charge);
    lv_label_set_text_fmt(objects.voltage_bat, "%.3f", voltage_bat);
    lv_label_set_text_fmt(objects.amper_consume, "%.3f", amper_consume);

    // print same to console
    Serial.println(voltage_gen);
    Serial.println(amper_charge);
    Serial.println(voltage_bat);
    Serial.println(amper_consume);
}

// Функция подключения к MQTT-брокеру
void reconnect()
{
    while (!client.connected())
    {
        Serial.print("Подключение к MQTT...");
        if (client.connect("ESP32Client", "mqtt", "mqtt5696"))
        {
            Serial.println("Успешно!");
            client.subscribe("ioc/tele/BatteryControl/SENSOR"); // Подписываемся на топик
            client.subscribe("ioc/stat/BatteryControl/STATUS10");
            client.publish("ioc/tele/monitor/online", "online");
            Serial.println("Сообщение 'online' отправлено в топик 'ioc/tele/monitor/online'.");
        }
        else
        {
            Serial.print("Ошибка подключения. Код: ");
            Serial.print(client.state());
            Serial.println(". Повтор через 5 секунд.");
            delay(5000);
        }
    }
}

void setup()
{
#ifdef ARDUINO_USB_CDC_ON_BOOT
    delay(5000);
#endif
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    log_i("Board: %s", BOARD_NAME);
    log_i("CPU: %s rev%d, CPU Freq: %d Mhz, %d core(s)", ESP.getChipModel(), ESP.getChipRevision(), getCpuFrequencyMhz(), ESP.getChipCores());
    log_i("Free heap: %d bytes", ESP.getFreeHeap());
    log_i("Free PSRAM: %d bytes", ESP.getPsramSize());
    log_i("SDK version: %s", ESP.getSdkVersion());

    smartdisplay_init();

    __attribute__((unused)) auto disp = lv_disp_get_default();

    ui_init();

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(1000);
    }

    smartdisplay_lcd_set_brightness_cb(smartdisplay_lcd_adaptive_brightness_cds, 100);
    lv_label_set_text(objects.wifi_ip, WiFi.localIP().toString().c_str());

    // Настройка MQTT
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    // Подключение к MQTT
    reconnect();

    // connectWifi(); // connect to wifi and start OTA
}

ulong next_millis;
auto lv_last_tick = millis();

void loop()
{
    auto const now = millis();

    if (now > next_millis)
    {
        next_millis = now + 500;

        
    }
    // Update the ticker
    lv_tick_inc(now - lv_last_tick);
    lv_last_tick = now;
    // Update the UI

    client.loop();
    lv_timer_handler();
}
