#include <stdio.h>

#include "pico/stdlib.h"

// For scb_hw so we can enable deep sleep
#include "hardware/structs/scb.h"

// For __wfi() function
#include "hardware/sync.h"

#include "PWM.h"
#include "Stepper.h"
#include "RTC.h"
#include "WS2812.h"

/// @brief Handles the usb power detection gpio pin going high.
/// @param gpio The gpio pin that caused the interrupt.
/// @param event_mask The type of interrupt that occured
void usbPowerDetectionHandler(uint gpio, uint32_t event_mask)
{
    if (gpio == 24 && event_mask == GPIO_IRQ_EDGE_RISE)
    {
        // Show that we are awake
        gpio_put(25, true);

        // Disable IRQ for rising edge
        gpio_set_irq_enabled(24, GPIO_IRQ_EDGE_RISE, false);

        // Disable sleep on exit of IRQ hanlder
        scb_hw->scr &= ~M0PLUS_SCR_SLEEPONEXIT_BITS;
    }
}

/// @brief Puts the current core to sleep until gpio pin 24 (usb power detection) goes high.
void goToSleep()
{
    // Enable IRQ for rising edge of usb power
    gpio_set_irq_enabled_with_callback(24, GPIO_IRQ_EDGE_RISE, true, usbPowerDetectionHandler);

    // Turn off all clocks when in sleep mode except for RTC
    // TODO: Check which other clocks need to keep running
    // clocks_hw->sleep_en0 = CLOCKS_SLEEP_EN0_CLK_RTC_RTC_BITS;
    // clocks_hw->sleep_en1 = 0x0;

    // Show that we went to sleep
    gpio_put(25, false);

    // Enable sleep and automatic sleep on exit from irq handler
    scb_hw->scr |= (M0PLUS_SCR_SLEEPDEEP_BITS | M0PLUS_SCR_SLEEPONEXIT_BITS);

    // Go to sleep
    __wfi();
}

/// @brief Reads a line from stdin. Aborts when the virtual serial console gets disconnected.
/// @param buffer The buffer to write the data into.
/// @param sizeOfBuffer The size of the buffer.
/// @return Number of chars read or zero when serial console got disconnected.
uint32_t readLine(char *buffer, uint32_t sizeOfBuffer)
{
    uint32_t index = 0;

    bool powerConnected = false;
    while (powerConnected = gpio_get(24) && index < sizeOfBuffer)
    {
        int c = getchar_timeout_us(1000000); // 1s

        if (c == PICO_ERROR_TIMEOUT)
            continue;

        if (c == '\n' || c == '\r')
        {
            putchar(c);
            break;
        }

        // Only printable ascii
        if (c < ' ' || c > '~')
            continue;

        putchar_raw(c);
        buffer[index++] = c;
    }

    // If power got disconnected before receiving a complete line return zero
    if (!powerConnected)
        return 0;

    return index;
}

/// @brief Converts chars 0 to 9 to an integer.
/// @param c The char to convert.
/// @param number The result of the converion.
/// @return true when the character was between 0 to 9, otherwise false.
bool convertCharToNumber(char c, uint32_t *number)
{
    if (c >= '0' && c <= '9')
    {
        *number = c - '0';
        return true;
    }

    return false;
}

/// @brief Parses a datetime with the given format dd.mm.yy hh:mm
/// @param buffer The buffer containing the string to parse.
/// @param sizeofBuffer The size of the buffer.
/// @param datetime The parsed date time.
/// @return true when the date time was parsed successfully, otherwise false
bool parseDateTime(char *buffer, uint32_t sizeofBuffer, datetime_t *datetime)
{
    if (sizeofBuffer != 14)
    {
        printf("Invalid length: %d\n", sizeofBuffer);
        return false;
    }

    uint32_t digit = 0;
    // Day
    if (!convertCharToNumber(buffer[0], &digit))
    {
        printf("Invalid char (day digit 1): %c\n", buffer[0]);
        return false;
    }
    datetime->day = digit;

    if (!convertCharToNumber(buffer[1], &digit))
    {
        printf("Invalid char (day digit 2): %c\n", buffer[1]);
        return false;
    }
    datetime->day = ((datetime->day * 10) + digit);

    if (buffer[2] != '.')
    {
        printf("Invalid char (day month seperator): %c\n", buffer[2]);
        return false;
    }

    // Month
    if (!convertCharToNumber(buffer[3], &digit))
    {
        printf("Invalid char (month digit 1): %c\n", buffer[3]);
        return false;
    }
    datetime->month = digit;

    if (!convertCharToNumber(buffer[4], &digit))
    {
        printf("Invalid char (month digit 2): %c\n", buffer[4]);
        return false;
    }
    datetime->month = ((datetime->month * 10) + digit);

    if (buffer[5] != '.')
    {
        printf("Invalid char (month year seperator): %c\n", buffer[5]);
        return false;
    }

    // Year
    if (!convertCharToNumber(buffer[6], &digit))
    {
        printf("Invalid char (year digit 1): %c\n", buffer[6]);
        return false;
    }
    datetime->year = digit;

    if (!convertCharToNumber(buffer[7], &digit))
    {
        printf("Invalid char (year digit 2): %c\n", buffer[7]);
        return false;
    }
    datetime->year = (2000 + ((datetime->year * 10) + digit));

    if (buffer[8] != ' ')
    {
        printf("Invalid char (year hour seperator): %c\n", buffer[8]);
        return false;
    }

    // Hour
    if (!convertCharToNumber(buffer[9], &digit))
    {
        printf("Invalid char (hour digit 1): %c\n", buffer[9]);
        return false;
    }
    datetime->hour = digit;

    if (!convertCharToNumber(buffer[10], &digit))
    {
        printf("Invalid char (hour digit 2): %c\n", buffer[10]);
        return false;
    }
    datetime->hour = ((datetime->hour * 10) + digit);

    if (datetime->hour > 23)
    {
        printf("Hour out of range (0-23): %02d", datetime->hour);
        return false;
    }

    if (buffer[11] != ':')
    {
        printf("Invalid char (hour minute seperator): %c\n", buffer[11]);
        return false;
    }

    // Minute
    if (!convertCharToNumber(buffer[12], &digit))
    {
        printf("Invalid char (minute digit 1): %c\n", buffer[12]);
        return false;
    }
    datetime->min = digit;

    if (!convertCharToNumber(buffer[13], &digit))
    {
        printf("Invalid char (minute digit 2): %c\n", buffer[12]);
        return false;
    }
    datetime->min = ((datetime->min * 10) + digit);

    if (datetime->min > 59)
    {
        printf("Minute out of range (0-59): %02d", datetime->min);
        return false;
    }

    return true;
}

/// @brief Parses a string containing a 2 digit hour (00-23)
/// @param buffer The buffer containing the string to parse.
/// @param sizeofBuffer The size of the buffer.
/// @param hour The parsed hour.
/// @return true when the hour was parsed successfully, otherwise false
bool parseHour(char *buffer, uint32_t sizeofBuffer, uint8_t *hour)
{
    if (sizeofBuffer != 2)
    {
        printf("Invalid length: %d\n", sizeofBuffer);
        return false;
    }

    uint32_t digit = 0;
    if (!convertCharToNumber(buffer[0], &digit))
    {
        printf("Invalid char (hour digit 1): %c\n", buffer[0]);
        return false;
    }
    *hour = digit;

    if (!convertCharToNumber(buffer[1], &digit))
    {
        printf("Invalid char (hour digit 2): %c\n", buffer[1]);
        return false;
    }

    *hour = ((*hour * 10) + digit);

    if (*hour > 23)
    {
        printf("Hour out of range (0-23): %i", *hour);
        return false;
    }

    return true;
}

/// @brief Parses a string containing a boolean (yYnN)
/// @param buffer The buffer containing the string to parse.
/// @param sizeofBuffer The size of the buffer.
/// @param value The parsed boolean.
/// @return true when the boolean was parsed successfully, otherwise false
bool parseBool(char *buffer, uint32_t sizeofBuffer, bool *value)
{
    if (sizeofBuffer != 1)
    {
        printf("Invalid length: %d\n", sizeofBuffer);
        return false;
    }

    if (buffer[0] == 'Y' || buffer[0] == 'y')
    {
        *value = true;
        return true;
    }

    if (buffer[0] == 'N' || buffer[0] == 'n')
    {
        *value = false;
        return true;
    }

    return false;
}

/// @brief Manually home the given stepper motor by typing + or - on the virtual serial console.
/// @param stepper The stepper motor to home.
/// @return true when the motor was homed or false when the process was aborted.
bool manualHomeStepper(struct stepper *stepper)
{
    // Move motor a step and break when enter is pressed
    bool powerConnected;
    while (powerConnected = gpio_get(24))
    {
        int c = getchar_timeout_us(1000000); // 1s

        if (c == PICO_ERROR_TIMEOUT)
            continue;

        if (c == '\n' || c == '\r')
            break;

        if (c == '+')
            stepperStep(stepper, false);
        else if (c == '-')
            stepperStep(stepper, true);
    }

    return powerConnected;
}

int main()
{
    // Init driver enable pin
    gpio_init(8);
    gpio_set_dir(8, true);
    gpio_put(8, false); // Disable driver

    // Init step generator
    initStepper(&hourStepper);
    initStepper(&minuteStepper);
    gpio_put(8, true); // Enable stepper drivers

    // USB power detection gpio
    gpio_init(24);
    gpio_set_dir(24, false);

    // Onboard led
    gpio_init(25);
    gpio_set_dir(25, true);
    gpio_put(25, true);

    ws2812_init();

    // If usb power isnt connected to to sleep
    if (!gpio_get(24))
        goToSleep();

    // Either we awoke from sleep or power was connected already
    while (true)
    {
        // Show that we are awake
        gpio_put(25, true);

        // Wait 500ms before attempting to do anything
        sleep_ms(500);

        // While power is connected to the usb port try to init stdio through usb
        bool powerConnected = false;
        while (powerConnected = gpio_get(24))
        {
            if (stdio_usb_init())
                break;

            sleep_ms(1000);
        }
        if (!powerConnected)
            goto endOfLoop;

        // Wait until something has connected to the virtual serial port
        while (powerConnected = gpio_get(24))
        {
            if (stdio_usb_connected())
                break;

            sleep_ms(1000);
        }
        if (!powerConnected)
            goto endOfLoop;

        // Display start message
        puts("TinyStepperClock V1.0 Press enter to continue.");

        // Wait until enter is pressed
        while (powerConnected = gpio_get(24))
        {
            int c = getchar_timeout_us(1000000); // 1s

            if (c == PICO_ERROR_TIMEOUT)
                continue;

            if (c == '\n' || c == '\r')
                break;
        }
        if (!powerConnected)
            goto endOfLoop;

        char buffer[128];

        puts("Enable hourly animations (y,n):");
        while (powerConnected = gpio_get(24))
        {
            uint32_t numberOfCharsRead = readLine(buffer, count_of(buffer));

            // If nothing has been read either the power has been disconnected or enter was pressed immediately
            if (numberOfCharsRead == 0)
                continue;

            bool success = parseBool(buffer, numberOfCharsRead, &enableHourlyAnimation);
            if (success)
            {
                printf("Animations enabled: %s\n", enableHourlyAnimation ? "Y" : "N");
                break;
            }

            puts("Invalid input!");
        }
        if (!powerConnected)
            goto endOfLoop;

        if (enableHourlyAnimation)
        {
            puts("Enter animation start hour (00-23):");
            while (powerConnected = gpio_get(24))
            {
                uint32_t numberOfCharsRead = readLine(buffer, count_of(buffer));

                // If nothing has been read either the power has been disconnected or enter was pressed immediately
                if (numberOfCharsRead == 0)
                    continue;

                bool success = parseHour(buffer, numberOfCharsRead, &animationStartHour);
                if (success)
                {
                    printf("Got hour: %02d\n", animationStartHour);
                    break;
                }

                puts("Invalid input!");
            }
            if (!powerConnected)
                goto endOfLoop;

            puts("Enter animation end hour (00-23):");
            while (powerConnected = gpio_get(24))
            {
                uint32_t numberOfCharsRead = readLine(buffer, count_of(buffer));

                // If nothing has been read either the power has been disconnected or enter was pressed immediately
                if (numberOfCharsRead == 0)
                    continue;

                bool success = parseHour(buffer, numberOfCharsRead, &animationEndHour);
                if (success)
                {
                    printf("Got hour: %02d\n", animationEndHour);
                    break;
                }

                puts("Invalid input!");
            }
            if (!powerConnected)
                goto endOfLoop;
        }

        // Disable rtc alarm after this point so the clock hands dont move while we are trying to set the clock
        disableRtcAlarm();

        puts("Move hour hand to 12 o'clock position and press enter. + = CW - = CCW");
        powerConnected = manualHomeStepper(&hourStepper);
        if (!powerConnected)
            goto endOfLoop;

        puts("Move minute hand to 12 o'clock position and press enter. + = CW - = CCW");
        powerConnected = manualHomeStepper(&minuteStepper);
        if (!powerConnected)
            goto endOfLoop;

        datetime_t dateAndTime = {};

        // Display prompt for setting the date and time
        puts("Type date and time (dd.mm.yy hh:mm) and press enter.");
        while (powerConnected = gpio_get(24))
        {
            uint32_t numberOfCharsRead = readLine(buffer, count_of(buffer));

            // If nothing has been read either the power has been disconnected or enter was pressed immediately
            if (numberOfCharsRead == 0)
                continue;

            bool success = parseDateTime(buffer, numberOfCharsRead, &dateAndTime);
            if (success)
            {
                printf("Got date and time: %02d.%02d.%d %02d:%02d\n",
                       dateAndTime.day,
                       dateAndTime.month,
                       dateAndTime.year,
                       dateAndTime.hour,
                       dateAndTime.min);
                break;
            }

            puts("Invalid input!");
        }
        if (!powerConnected)
            goto endOfLoop;

        seekClockHands(&dateAndTime);
        rtcInit(&dateAndTime);

        // Wait until power is disconnected before going to sleep to prevent the usb device from disconnecting improperly.
        while (gpio_get(24))
            sleep_ms(500);

    endOfLoop:
        goToSleep();
    }

    return 0;
}