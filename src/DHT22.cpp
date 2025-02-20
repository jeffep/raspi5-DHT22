#include "DHT22.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <time.h>
#include <array>

DHT22::DHT22(int dhtPin, bool debug) : debugMode(debug) {
     if (debugMode) {
        std::cout << "[INFO] Initializing DHT22 on GPIO " << dhtPin << std::endl;
     }

    chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip) {
        std::cerr << "[ERROR] Failed to open GPIO chip" << std::endl;
        return;
    }

    line = gpiod_chip_get_line(chip, dhtPin);
    if (!line) {
        std::cerr << "[ERROR] Failed to get GPIO line" << std::endl;
        gpiod_chip_close(chip);
        return;
    }

    if (debugMode) {
     std::cout << "[INFO] Successfully initialized GPIO " << dhtPin << std::endl;
    }
}

DHT22::~DHT22() {
    if (chip) {
        gpiod_chip_close(chip);
     if (debugMode) {
         std::cout << "[INFO] GPIO chip closed" << std::endl;
     }
    }
}

void preciseSleep(long nanoseconds) {
    struct timespec ts;
    ts.tv_sec = nanoseconds / 1000000000L; // Seconds
    ts.tv_nsec = nanoseconds % 1000000000L; // Nanoseconds

    if (nanosleep(&ts, nullptr) != 0) {
        std::cerr << "nanosleep failed" << std::endl;
    }
}

std::vector<int> DHT22::sendStartSignal() {
    std::vector<int> rawData(3000, 0);

     if (debugMode) {
        std::cout << "[INFO] Sending start signal to sensor..." << std::endl;
     }
    struct gpiod_line_request_config config = {};
    config.consumer = "DHT22";
    config.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;

    if (gpiod_line_request(line, &config, 0) < 0) {
        std::cerr << "[ERROR] Failed to request GPIO line as output" << std::endl;
        return rawData;
    }

    gpiod_line_set_value(line, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(2)); // Pull low for at least 1-2ms
    gpiod_line_set_value(line, 1);
    std::this_thread::sleep_for(std::chrono::microseconds(22)); // Keep high for 30µs

    gpiod_line_release(line);

     if (debugMode) {
        std::cout << "[INFO] Start signal sent, switching to input mode" << std::endl;
     }

    config.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
    if (gpiod_line_request(line, &config, 0) < 0) {
        std::cerr << "[ERROR] Failed to request GPIO line as input" << std::endl;
        return rawData;
    }

     if (debugMode) {
       std::cout << "[INFO] Sampling GPIO state..." << std::endl;
     }

    for (size_t i = 0; i < rawData.size(); ++i) {
        int val = gpiod_line_get_value(line);
        if (val < 0) {
            std::cerr << "[ERROR] Failed to read GPIO value at index " << i << std::endl;
            break;
        }
        rawData[i] = val;
        // std::this_thread::sleep_for(std::chrono::microseconds(10));
        //preciseSleep(50); // Sleep for 50 nanoseconds
    }

     if (debugMode) {
        std::cout << "[INFO] Finished sampling GPIO state" << std::endl;

        // Print the size of rawData
        std::cout << "[INFO] Size of rawData: " << rawData.size() << std::endl;

       // Print the contents of rawData
       std::cout << "[INFO] Contents of rawData: ";
       for (size_t i = 0; i < rawData.size(); ++i) {
           std::cout << rawData[i] << " ";
       }
       std::cout << std::endl;
    }
    return rawData;
}

void DHT22::decodeData(const std::vector<int>& rawData, float& humidity, float& temperature) {
    int goodBitCounter=0, switches=0;
    uint8_t myDHTResult[40] = {0};
 
    // Works best to parse backwards to find the start of the data
    switches=2999;
    goodBitCounter=39;

    if (debugMode) {
      std::cout << "Raw Data(" << rawData.size() << ")=";
      for (size_t i = 0; i < rawData.size(); ++i) {
        std::cout << rawData[i] << "";
      }
      std::cout << std::endl;
    }

    while (rawData[switches] == 1) switches--;
    // Now we've hit 0, so the last data bit

    // A zero bit is high (3.3v) for 28us then goes low.
    // We get about 21 reads at 3.3v for 28us.
    // After that it is consistently 0 for a zero bit, or 1 for a 1
    while (switches >= 22 && goodBitCounter >= 0) {
        while(rawData[switches] == 0) switches--;

        // Check if rawData[switches] and rawData[switches - 22] are both 1
        if (rawData[switches] == 1 && rawData[switches - 22] == 1) {
            myDHTResult[goodBitCounter] = 1; // Span of more than 21 1's
        } else {
            myDHTResult[goodBitCounter] = 0; // Span of exactly 21 1's
        }

        //skip to the next set of 0s
        while(rawData[switches]==1) switches--;
        if (debugMode) {
           std::cout << "myDHTResult[" << goodBitCounter << "]=" << static_cast<int>(myDHTResult[goodBitCounter]) << std::endl;
        }
        goodBitCounter--;
    }

    // std::cerr << "[ERROR] goodBitCounter= " << goodBitCounter << std::endl;
    if (goodBitCounter == 0) {
         std::cerr << "[ERROR] Bad Read" << std::endl;
    }

     if (debugMode) {
        std::cout << "Final myDHTResult: ";
        for (int i = 0; i < 40; ++i) {
            std::cout << static_cast<int>(myDHTResult[i]) << " ";
        }
        std::cout << std::endl;
     }

    int data[5] = {0};
    for (int i = 0; i < 40; ++i) {
        data[i / 8] <<= 1;
        if (myDHTResult[i]) {
            data[i / 8] |= 1;
        }
    }
    
    int checksum = data[0] + data[1] + data[2] + data[3];
    if ((checksum & 0xFF) != data[4]) {
        std::cerr << "[ERROR] Checksum mismatch:" << checksum << "!=" << data[4] << std::endl;
        std::cerr << "data =" << data[0] << "," << data[1] << "," << data[2] << "," << data[3] << std::endl;
        return;
    }
    
    humidity = (float)((data[0] << 8) + data[1]) / 10.0;
    temperature = (float)((data[2] << 8) + data[3]) / 10.0;

    // Convert temperature to Fahrenheit
    float temperatureFahrenheit = (temperature * 1.8f) + 32.0f;
    
    if (humidity > 100) {
        humidity = data[0];
        temperature = data[2];
    }
    
    // std::cout << "Humidity = " << humidity << " % Temperature = " << temperature << " C" << std::endl;
}

