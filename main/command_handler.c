#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_main.h"

void handle_command(char *data, int datalen)
{
    uint32_t numberOfMeasurements;
    uint32_t delayBetweenMeasurements;
    sscanf(data, "measure:%lu,%lu", &numberOfMeasurements, &delayBetweenMeasurements);

    printf("We should take %lu measurements with a delay of %lu ms.\n", numberOfMeasurements, delayBetweenMeasurements);

    uint32_t numberOfCommandsLeft = 0;
    float temperature = 1.2;
    uint32_t time = 69;

    // INSERT LOGIC TO MEASURE HERE AND SET THE VARIABLES ABOVE

    int lengthOfTotalString = (10 + 1 + 20 + 1 + 10); // Potential max length of string
    char *response = (char *)malloc((lengthOfTotalString * sizeof(char)));
    sprintf(response, "%lu,%.1f,%lu", numberOfCommandsLeft, temperature, time);

    printf("I should now send:\n%s\n", response);

    send_response(response, strlen(response));
    free(response);
}