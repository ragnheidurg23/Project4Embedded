#include <linux/init.h> 
#include <linux/module.h> 
#include <linux/kernel.h> 
#include <linux/gpio.h>                       // for the GPIO functions 
#include <linux/interrupt.h>                  // for the IRQ code

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("Group 2 & 6"); 
MODULE_DESCRIPTION("A Encoder/LED test driver for the RPi"); 
MODULE_VERSION("0.1");

static unsigned int gpioLED = 17;             // pin 11 (GPIO17) 
static unsigned int gpioEncoder = 27;          // pin 13 (GPIO27) 
static unsigned int irqNumber;                // share IRQ num within file 
static unsigned int num_pulses = 0;        // store number of presses 
static bool         ledOn = 0;                // used to invert state of LED

// prototype for the custom IRQ handler function, function below 
static irq_handler_t  erpi_gpio_irq_handler(unsigned int irq, 
                                            void *dev_id, struct pt_regs *regs);

static int __init erpi_gpio_init(void) 
{
    int result = 0;
    printk(KERN_INFO "ENCODER: Initializing the ENCODER LKM\n");

    if (!gpio_is_valid(gpioLED)) 
    {
        printk(KERN_INFO "ENCODER: invalid LED GPIO\n");
        return -ENODEV;
    }   

    ledOn = true;

    gpio_request(gpioLED, "sysfs");          // request LED GPIO
    gpio_direction_output(gpioLED, ledOn);   // set in output mode and on 
    // gpio_set_value(gpioLED, ledOn);       // not reqd - see line above
    gpio_export(gpioLED, false);             // appears in /sys/class/gpio
                                             // false prevents in/out change   
    gpio_request(gpioEncoder, "sysfs");       // set up gpioEncoder   
    gpio_direction_input(gpioEncoder);        // set up as input   
    gpio_set_debounce(gpioEncoder, 200);      // debounce delay of 200ms
    gpio_export(gpioEncoder, false);          // appears in /sys/class/gpio

    printk(KERN_INFO "ENCODER: encoder value is currently: %d\n", 
           gpio_get_value(gpioEncoder));

    irqNumber = gpio_to_irq(gpioEncoder);     // map GPIO to IRQ number
    printk(KERN_INFO "ENCODER: encoder mapped to IRQ: %d\n", irqNumber);

    // This next call requests an interrupt line    
    result = request_irq(irqNumber,                     // interrupt number requested            
        (irq_handler_t) erpi_gpio_irq_handler,          // handler function            
        IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,     // on rising edge and falling edge            
        "erpi_gpio_handler",                            // used in /proc/interrupts
        NULL);                                          // *dev_id for shared interrupt lines
    printk(KERN_INFO "ENCODER: IRQ request result is: %d\n", result);
    return result;

}

static void __exit erpi_gpio_exit(void) 
{   
    printk(KERN_INFO "ENCODER: encoder value is currently: %d\n", 
           gpio_get_value(gpioEncoder));

    printk(KERN_INFO "ENCODER: total pulses: %d\n", num_pulses);
    gpio_set_value(gpioLED, 0);              // turn the LED off
    gpio_unexport(gpioLED);                  // unexport the LED GPIO   
    free_irq(irqNumber, NULL);               // free the IRQ number, no *dev_id   
    gpio_unexport(gpioEncoder);               // unexport the Button GPIO   
    gpio_free(gpioLED);                      // free the LED GPIO
    gpio_free(gpioEncoder);                   // free the Button GPIO
    printk(KERN_INFO "ENCODER: Goodbye from the LKM!\n"); 
}

static irq_handler_t erpi_gpio_irq_handler(unsigned int irq, 
                                           void *dev_id, struct pt_regs *regs) 
{   
    ledOn = !ledOn;                          // toggle the LED state   
    gpio_set_value(gpioLED, ledOn);          // set LED accordingly  
    // printk(KERN_INFO "ENCODER: Interrupt! (pulse is %d)\n", gpio_get_value(gpioEncoder));
    num_pulses++;                         // global counter
    return (irq_handler_t) IRQ_HANDLED;      // announce IRQ handled 
}

module_init(erpi_gpio_init);
module_exit(erpi_gpio_exit);