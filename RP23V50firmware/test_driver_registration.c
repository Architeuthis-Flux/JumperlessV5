#include <stdio.h>
#include <libsigrok/libsigrok.h>

int main()
{
    struct sr_context *ctx;
    struct sr_dev_driver **drivers;
    int i;
    
    printf("Testing libsigrok driver registration...\n\n");
    
    /* Initialize libsigrok */
    if (sr_init(&ctx) != SR_OK) {
        printf("Failed to initialize libsigrok\n");
        return 1;
    }
    
    /* Get driver list */
    drivers = sr_driver_list(ctx);
    if (!drivers) {
        printf("No drivers found!\n");
        sr_exit(ctx);
        return 1;
    }
    
    /* List all drivers */
    printf("Registered drivers:\n");
    for (i = 0; drivers[i]; i++) {
        printf("  %d: %s (%s)\n", i, drivers[i]->name, drivers[i]->longname);
        
        /* Check specifically for our drivers */
        if (strcmp(drivers[i]->name, "jumperless-mixed-signal") == 0) {
            printf("    *** FOUND JUMPERLESS DRIVER! ***\n");
        }
        if (strcmp(drivers[i]->name, "bp5-binmode-fala") == 0) {
            printf("    *** FOUND BP5 DRIVER! ***\n");
        }
    }
    
    printf("\nTotal drivers: %d\n", i);
    
    /* Cleanup */
    sr_exit(ctx);
    
    return 0;
} 