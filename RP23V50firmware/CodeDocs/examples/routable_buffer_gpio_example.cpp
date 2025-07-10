#include "Probing.h"
#include "MatrixState.h"
#include <iostream>

/**
 * Example demonstrating the new GPIO functionality in routableBufferPower()
 * 
 * This example shows how to use the new useGPIO parameter to connect
 * the routable buffer to an unused GPIO instead of a DAC.
 */

void demonstrateGPIOFunctionality() {
    std::cout << "Demonstrating routableBufferPower with GPIO option..." << std::endl;
    std::cout << "==================================================" << std::endl;
    
    // Test 1: Power on using DAC (default behavior)
    std::cout << "\nTest 1: Power on using DAC (default behavior)" << std::endl;
    routableBufferPower(1, 0, 0, 0);  // offOn=1, flash=0, force=0, useGPIO=0
    std::cout << "✓ Routable buffer powered on using DAC" << std::endl;
    
    // Test 2: Power off using DAC
    std::cout << "\nTest 2: Power off using DAC" << std::endl;
    routableBufferPower(0, 0, 0, 0);  // offOn=0, flash=0, force=0, useGPIO=0
    std::cout << "✓ Routable buffer powered off using DAC" << std::endl;
    
    // Test 3: Power on using GPIO (new functionality)
    std::cout << "\nTest 3: Power on using GPIO (new functionality)" << std::endl;
    routableBufferPower(1, 0, 0, 1);  // offOn=1, flash=0, force=0, useGPIO=1
    std::cout << "✓ Routable buffer powered on using GPIO" << std::endl;
    
    // Test 4: Power off using GPIO
    std::cout << "\nTest 4: Power off using GPIO" << std::endl;
    routableBufferPower(0, 0, 0, 1);  // offOn=0, flash=0, force=0, useGPIO=1
    std::cout << "✓ Routable buffer powered off using GPIO" << std::endl;
    
    std::cout << "\n==================================================" << std::endl;
    std::cout << "All tests completed!" << std::endl;
    std::cout << "\nNote: If no unused GPIOs are available, the function will" << std::endl;
    std::cout << "fall back to using DAC behavior automatically." << std::endl;
}

void checkGPIOAvailability() {
    std::cout << "\nChecking GPIO availability..." << std::endl;
    std::cout << "=============================" << std::endl;
    
    // Check which GPIOs are currently connected
    std::vector<int> connected_gpios;
    std::vector<int> available_gpios;
    
    for (int i = 1; i <= 8; i++) {
        int gpio_node = RP_GPIO_1 + (i - 1);
        bool is_connected = false;
        
        // Check if this GPIO is connected to anything in any net
        for (int net_index = 0; net_index < numberOfNets; net_index++) {
            for (int node_index = 0; node_index < MAX_NODES; node_index++) {
                if (net[net_index].nodes[node_index] <= 0) {
                    break; // End of nodes array
                }
                if (net[net_index].nodes[node_index] == gpio_node) {
                    is_connected = true;
                    break;
                }
            }
            if (is_connected) break;
        }
        
        if (is_connected) {
            connected_gpios.push_back(i);
        } else {
            available_gpios.push_back(i);
        }
    }
    
    std::cout << "Connected GPIOs: ";
    for (int gpio : connected_gpios) {
        std::cout << "RP_GPIO_" << gpio << " ";
    }
    std::cout << std::endl;
    
    std::cout << "Available GPIOs: ";
    for (int gpio : available_gpios) {
        std::cout << "RP_GPIO_" << gpio << " ";
    }
    std::cout << std::endl;
    
    if (!available_gpios.empty()) {
        std::cout << "✓ Found " << available_gpios.size() << " available GPIO(s) for routable buffer" << std::endl;
    } else {
        std::cout << "⚠ No GPIOs available - will fall back to DAC behavior" << std::endl;
    }
}

int main() {
    // Initialize the system (this would normally be done in setup())
    // initNets();
    // initChipStatus();
    
    // Check GPIO availability
    checkGPIOAvailability();
    
    // Demonstrate the new functionality
    demonstrateGPIOFunctionality();
    
    return 0;
} 