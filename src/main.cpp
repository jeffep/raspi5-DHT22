#include "DHT22.h"
#include <iostream>
#include <vector>
#include <cstring>

void printHelp() {
    std::cout << "DHT22 Sensor Reader\n"
              << "Reads temperature and humidity from a DHT22 sensor using GPIO.\n\n"
              << "Usage: ./dht22 [GPIO_PIN] [OPTIONS]\n\n"
              << "Options:\n"
              << "  -h        Show this help message\n"
              << "  -d        Enable debug mode (prints additional debug information)\n"
              << "  -c        Display temperature in Celsius (default)\n"
              << "  -f        Display temperature in Fahrenheit\n"
              << "  -cf       Display temperature in both Celsius and Fahrenheit\n\n"
              << "Example:\n"
              << "  ./dht22 4 -d -cf\n"
              << "  (Reads from GPIO 4, enables debug mode, shows temperature in C & F)\n";
}

int main(int argc, char* argv[]) {
    int gpioPin = 4; // Default GPIO pin
    bool debug = false;
    bool showCelsius = true;
    bool showFahrenheit = false;

    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0) {
            printHelp();
            return 0;
        } else if (strcmp(argv[i], "-d") == 0) {
            debug = true;
        } else if (strcmp(argv[i], "-c") == 0) {
            showCelsius = true;
            showFahrenheit = false;
        } else if (strcmp(argv[i], "-f") == 0) {
            showCelsius = false;
            showFahrenheit = true;
        } else if (strcmp(argv[i], "-cf") == 0) {
            showCelsius = true;
            showFahrenheit = true;
        } else {
            // Assume argument is a GPIO pin number
            try {
                gpioPin = std::stoi(argv[i]);
            } catch (...) {
                std::cerr << "[ERROR] Invalid GPIO pin number: " << argv[i] << "\n";
                return 1;
            }
        }
    }

    if (debug) {
        std::cout << "[DEBUG] Using GPIO pin: " << gpioPin << std::endl;
        std::cout << "[DEBUG] Temperature Unit: " 
                  << (showCelsius ? "Celsius " : "") 
                  << (showFahrenheit ? "Fahrenheit" : "") 
                  << std::endl;
        std::cout << "[INFO] Starting DHT22 test on GPIO " << gpioPin << std::endl;
    }

    DHT22 sensor(gpioPin, debug);
    std::vector<int> data = sensor.sendStartSignal();

    if (data.empty()) {
        std::cerr << "[ERROR] No data received from DHT22" << std::endl;
        return 1;
    }

    float humidity, temperature;
    sensor.decodeData(data, humidity, temperature);

    if (debug) {
        std::cout << "[DEBUG] Raw data collected: ";
        for (int bit : data) {
            std::cout << bit;
        }
        std::cout << std::endl;
    }

    std::cout << "Humidity: " << humidity << " % | Temperature: ";
    if (showCelsius) {
        std::cout << temperature << "°C";
    }
    if (showCelsius && showFahrenheit) {
        std::cout << " / ";
    }
    if (showFahrenheit) {
        float tempF = (temperature * 9.0 / 5.0) + 32;
        std::cout << tempF << "°F";
    }
    std::cout << std::endl;
    return 0;
}

