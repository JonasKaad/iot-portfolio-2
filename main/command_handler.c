#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include "app_main.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_task_wdt.h>

// From: https://stackoverflow.com/a/51336144
uint32_t currentTimeMillis()
{
    struct timeval time;
    gettimeofday(&time, NULL);
    uint32_t s1 = (uint32_t)(time.tv_sec) * 1000;
    uint32_t s2 = (time.tv_usec / 1000);
    return s1 + s2;
}

void handle_command(char *data, int datalen)
{
    char *input = (char *)malloc((datalen * sizeof(char)));
    strncpy(input, data, datalen);

    uint32_t numberOfMeasurements = 0;
    uint32_t delayBetweenMeasurements = 0;

    int chars_read = -1;
    if (
        sscanf(input, "measure:%lu,%lu%n", &numberOfMeasurements, &delayBetweenMeasurements, &chars_read) != 2 ||
        chars_read == -1)
    {
        free(input);
        printf("Wrong format!\n");
        return;
    }
    free(input);
    printf("We should take %lu measurements with a delay of %lu ms.\n", numberOfMeasurements, delayBetweenMeasurements);

    uint32_t numberOfCommandsLeft = numberOfMeasurements;
    float temperature = 0.0;
    uint32_t response_time = 0;

    int lengthOfTotalString = (10 + 1 + 20 + 1 + 10); // Potential max length of string
    char *response = (char *)malloc((lengthOfTotalString * sizeof(char)));

    uint32_t count = 0;
    uint32_t start = currentTimeMillis();

    while (count < numberOfMeasurements)
    {
        uint32_t currentTime = currentTimeMillis();
        if (start + (count * delayBetweenMeasurements) <= currentTime)
        {
            numberOfCommandsLeft--;
            temperature = measure_temp();
            response_time = currentTimeMillis();

            sprintf(response, "%lu,%.1f,%lu", numberOfCommandsLeft, temperature, response_time);
            printf("I should now send:\n%s\n", response);
            send_response(response, strlen(response));
            count++;
        }
        if (currentTime % 100 == 0)
        {
            // To not trigger the Watchdog
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    printf("I am done! Releasing memory!\n");
    free(response);
}
