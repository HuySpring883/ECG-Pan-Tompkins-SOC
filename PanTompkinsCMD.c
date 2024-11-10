#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "PanTompkins.h"

int main(int argc, char* argv[]) {

    // --------------Input Arguments ------------------ //
    if (argc == 1 || argc > 3)
    {
        printf("\nProvide an input ECG filename!\n");
        printf("=================================\n");
        printf("Usage: PanTompkinsCMD FILENAME VERBOSITY\n\n");
        printf("Example: PanTompkinsCMD ecg.txt 1\n");
        printf("Reads ecg.txt and prints the results to both console and output file.\n\n");
        printf("Example: PanTompkinsCMD ecg.txt \n");
        printf("Reads ecg.txt but does not print to console and only prints to the file.\n");
        printf("Program prints the results in output.csv\n");
        exit(1);
    }

    FILE *fptr, *fptr_out;
    errno_t err, err1;
    int line_count = 0;
    char line[1024];  // Buffer for reading lines from the file
    float duration_minutes = 0.0;  // Duration will be stored as a float

    // Open the ECG file for reading
    err = fopen_s(&fptr, argv[1], "r");
    err1 = fopen_s(&fptr_out, "output.csv", "w");
    if (err == 0 || err1 == 0) {
        printf("The file %s was opened\n", argv[1]);
        fprintf_s(fptr_out, "Input,LPFilter,HPFilter,DerivativeF,SQRFilter,MVAFilter,RBeat,RunningThI1,SignalLevel,NoiseLevel,RunningThF\n");
    } else {
        printf("The file %s was not opened\n", argv[1]);
        exit(1);
    }

    int verbosity = atoi(argv[2]);

    PT_init();  // Always Initialize the Algorithm before use ---> This prepares all filters and parameters

    // ----------- Count the total lines and retrieve the last line (duration) -------------- //
    while (fgets(line, sizeof(line), fptr)) {
        line_count++;
    }

    // Go back to the last line to get the duration value
    fseek(fptr, 0, SEEK_SET);
    for (int i = 0; i < line_count - 1; i++) {
        fgets(line, sizeof(line), fptr);  // Skip all lines except the last
    }

    // Read the duration (last line should contain the duration as a float)
    if (fgets(line, sizeof(line), fptr)) {
        if (sscanf(line, "%f", &duration_minutes) != 1) {
            printf("Invalid or missing duration value.\n");
            fclose(fptr);
            fclose(fptr_out);
            exit(1);
        }
    }

    // Print the duration for confirmation
    printf("Duration (in minutes): %.2f\n", duration_minutes);

    int16_t delay, Rcount, s1, s2, s3, s4, s5, ThF1;
    uint16_t ThI1, SPKI, NPKI;
    int32_t RLoc, c, SampleCount;
    SampleCount = 0;

    Rcount = 0;

    // --------- Reset file pointer and start processing the ECG samples --------- //
    fseek(fptr, 0, SEEK_SET);
    while (fscanf_s(fptr, "%ld", &c) != EOF) {
        if (SampleCount == line_count - 1) {
            break; // Stop processing when reaching the line before the duration line
        }

        ++SampleCount;

        delay = PT_StateMachine((int16_t)c);  // This is the main function of the algorithm

        // ------- A positive delay to current sample is returned in case of beat detection ----------- //
        if (delay != 0) {
            RLoc = SampleCount - (int32_t)delay;
            ++Rcount;
        } else {
            RLoc = 0;
        }

        // -------- Toolbox comes with many helper functions for debugging, see PanTompkins.c for more details ---------- //
        s1 = PT_get_LPFilter_output();
        s2 = PT_get_HPFilter_output();
        s3 = PT_get_DRFilter_output();
        s4 = PT_get_SQRFilter_output();
        s5 = PT_get_MVFilter_output();

        ThI1 = PT_get_ThI1_output();
        SPKI = PT_get_SKPI_output();
        NPKI = PT_get_NPKI_output();
        ThF1 = PT_get_ThF1_output();

        if (verbosity)
            printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", c, s1, s2, s3, s4, s5, RLoc, ThI1, SPKI, NPKI, ThF1);

        fprintf_s(fptr_out, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", c, s1, s2, s3, s4, s5, RLoc, ThI1, SPKI, NPKI, ThF1);
    }

    // Calculate the BPM (beats per minute)
    float bpm = (Rcount / duration_minutes);

    // Print the results
    printf("%d beats detected\n", Rcount);
    printf("BPM: %.2f\n", bpm);

    fclose(fptr);
    fclose(fptr_out);
    return 0;
}
