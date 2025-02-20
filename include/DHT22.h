#ifndef DHT22_H
#define DHT22_H

#include <gpiod.h>
#include <vector>

class DHT22 {
public:
    explicit DHT22(int pin, bool debug = false);
    ~DHT22();
    
    bool readSensor();
    float getTemperature() const;
    float getHumidity() const;
    std::vector<int> sendStartSignal();
    void decodeData(const std::vector<int>& rawData, float& humidity, float& temperature);

private:
    int dhtPin;
    struct gpiod_chip *chip;
    struct gpiod_line *line;
    bool debugMode;  // This keeps track of whether debug mode is enabled 

    bool processData(const std::vector<int>& rawData);

    float temperature;
    float humidity;
};

#endif // DHT22_H

