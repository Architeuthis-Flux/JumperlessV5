// Board and hardware specific configuration
#define MICROPY_HW_BOARD_NAME                   "Jumperless v5"
#define MICROPY_HW_FLASH_STORAGE_BYTES          (PICO_FLASH_SIZE_BYTES - 1024 * 1024)

// USB VID/PID
#define MICROPY_HW_USB_VID (0x1D50)
#define MICROPY_HW_USB_PID (0xACAB)

// Enable filesystem support for runtime module imports
#define MICROPY_VFS                     (1)
#define MICROPY_VFS_FAT                 (1)
#define MICROPY_FATFS_ENABLE_LFN        (1)
#define MICROPY_FATFS_RSIZE             (512)
#define MICROPY_FATFS_WSIZE             (512)

// Enable machine module support
#define MICROPY_PY_MACHINE              (1)
#define MICROPY_PY_MACHINE_PULSE        (1)
#define MICROPY_PY_MACHINE_PWM          (1)
#define MICROPY_PY_MACHINE_PWM_INIT     (1)
#define MICROPY_PY_MACHINE_PWM_DUTY     (1)
#define MICROPY_PY_MACHINE_PWM_DUTY_U16 (1)
#define MICROPY_PY_MACHINE_PWM_INCLUDEFILE "ports/rp2/machine_pwm.c"
#define MICROPY_PY_MACHINE_I2C          (1)
#define MICROPY_PY_MACHINE_SPI          (1)
#define MICROPY_PY_MACHINE_UART         (1)

// Enable sys module for path manipulation
#define MICROPY_PY_SYS_PATH_DEFAULT     (1)

// UART0 - USB Serial (default)
#define MICROPY_HW_UART0_TX  (0)
#define MICROPY_HW_UART0_RX  (1)
#define MICROPY_HW_UART0_CTS (-1)
#define MICROPY_HW_UART0_RTS (-1)

// UART1 - Hardware serial
#define MICROPY_HW_UART1_TX  (24)
#define MICROPY_HW_UART1_RX  (25)
#define MICROPY_HW_UART1_CTS (-1)
#define MICROPY_HW_UART1_RTS (-1)

// I2C0 - Primary I2C bus
#define MICROPY_HW_I2C0_SCL  (5)
#define MICROPY_HW_I2C0_SDA  (4)

// I2C1 - Secondary I2C bus  
#define MICROPY_HW_I2C1_SCL  (23)
#define MICROPY_HW_I2C1_SDA  (22)

// SPI0 - Primary SPI bus
#define MICROPY_HW_SPI0_SCK  (22)
#define MICROPY_HW_SPI0_MOSI (23)
#define MICROPY_HW_SPI0_MISO (20)
#define MICROPY_HW_SPI0_NSS  (21)

// SPI1 - Secondary SPI bus
#define MICROPY_HW_SPI1_SCK  (26)
#define MICROPY_HW_SPI1_MOSI (27)
#define MICROPY_HW_SPI1_MISO (24)
#define MICROPY_HW_SPI1_NSS  (25)

// ADC configuration for Jumperless analog inputs
#define MICROPY_HW_ADC_COUNT            (4)

// ADC channel assignments (mapped to Jumperless nodes)
#define MICROPY_HW_ADC0_PIN             (26)  // NANO_A0/ADC0
#define MICROPY_HW_ADC1_PIN             (27)  // NANO_A1/ADC1
#define MICROPY_HW_ADC2_PIN             (28)  // NANO_A2/ADC2
#define MICROPY_HW_ADC3_PIN             (29)  // NANO_A3/ADC3

// PWM configuration for DAC simulation
#define MICROPY_HW_PWM_COUNT            (8)

// Enable additional modules for Jumperless integration
#define MICROPY_PY_USELECT              (1)
#define MICROPY_PY_UJSON                (1)
#define MICROPY_PY_URE                  (1)
#define MICROPY_PY_UHEAPQ               (1)
#define MICROPY_PY_UHASHLIB             (1)
#define MICROPY_PY_UBINASCII            (1)

// GPIO configuration for Jumperless matrix switching
#define JUMPERLESS_LED_DATA_PIN         (32)
#define JUMPERLESS_LED_CLK_PIN          (33)
#define JUMPERLESS_MATRIX_CS1_PIN       (34)
#define JUMPERLESS_MATRIX_CS2_PIN       (35)
#define JUMPERLESS_MATRIX_CS3_PIN       (36)

// Enable custom modules
#define MICROPY_PY_MACHINE_JUMPERLESS   (1)