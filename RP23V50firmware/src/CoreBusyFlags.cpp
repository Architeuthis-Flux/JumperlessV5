#include "CoreBusyFlags.h"

// Single point of definition (ODR).
volatile bool core1busy = false;
volatile bool core2busy = false;

volatile bool pauseCore2 = false;