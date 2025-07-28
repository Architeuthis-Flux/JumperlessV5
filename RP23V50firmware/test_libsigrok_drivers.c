#include <stdio.h>
#include <stdlib.h>
#include <libsigrok/libsigrok.h>

int main() {
    struct sr_context *ctx;
    struct sr_dev_driver **drivers;
    int ret, i;
    
    printf("=== Testing libsigrok driver enumeration ===\n");
    
    // Initialize libsigrok
    ret = sr_init(&ctx);
    if (ret != SR_OK) {
        printf("ERROR: Failed to initialize libsigrok: %d\n", ret);
        return 1;
    }
    
    printf("✅ libsigrok initialized successfully\n");
    
    // Get driver list
    drivers = sr_driver_list(ctx);
    if (!drivers) {
        printf("ERROR: No drivers found\n");
        sr_exit(ctx);
        return 1;
    }
    
    printf("\n=== Available drivers ===\n");
    
    int driver_count = 0;
    int jumperless_found = 0;
    
    for (i = 0; drivers[i]; i++) {
        driver_count++;
        printf("%3d: %-30s - %s\n", 
               i + 1, 
               drivers[i]->name ? drivers[i]->name : "NULL",
               drivers[i]->longname ? drivers[i]->longname : "NULL");
        
        // Check for our driver
        if (drivers[i]->name && 
            strcmp(drivers[i]->name, "jumperless-mixed-signal") == 0) {
            jumperless_found = 1;
            printf("     ★ FOUND JUMPERLESS DRIVER! ★\n");
            
            // Try to initialize the driver
            ret = sr_driver_init(ctx, drivers[i]);
            if (ret == SR_OK) {
                printf("     ✅ Driver initialization: SUCCESS\n");
            } else {
                printf("     ❌ Driver initialization: FAILED (%d)\n", ret);
            }
        }
    }
    
    printf("\n=== Summary ===\n");
    printf("Total drivers found: %d\n", driver_count);
    if (jumperless_found) {
        printf("✅ SUCCESS: Jumperless driver found and registered!\n");
        printf("The driver should appear in PulseView.\n");
        printf("If it doesn't, this might be a PulseView-specific issue.\n");
    } else {
        printf("❌ ERROR: Jumperless driver NOT found in driver list!\n");
        printf("This indicates a driver registration problem.\n");
    }
    
    // Cleanup
    sr_exit(ctx);
    return jumperless_found ? 0 : 1;
} 