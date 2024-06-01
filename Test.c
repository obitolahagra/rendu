#include <stdio.h>
#include <stdlib.h>
#include <NIDAQmx.h>

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else {}

static TaskHandle gAiTaskHandle = 0;
char errBuff[2048]={0};

int CreateAITask(double minVal, double maxVal, double rate, int sampsPerChan);
int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void* callbackData);
void CleanUp(void);
int CheckPumpShot(float64 voltage, float64 threshold);

int CreateAITask(double minVal, double maxVal, double rate, int sampsPerChan) {
    int error = 0;
    DAQmxErrChk(DAQmxCreateTask("", &gAiTaskHandle));
    // Include both ai2 and ai3 in the same task
    DAQmxErrChk(DAQmxCreateAIVoltageChan(gAiTaskHandle, "cDAQ1Mod1/ai2:3", "", DAQmx_Val_Cfg_Default, minVal, maxVal, DAQmx_Val_Volts, NULL));
    DAQmxErrChk(DAQmxCfgSampClkTiming(gAiTaskHandle, "", rate, DAQmx_Val_Rising, DAQmx_Val_ContSamps, sampsPerChan));
    DAQmxErrChk(DAQmxRegisterEveryNSamplesEvent(gAiTaskHandle, DAQmx_Val_Acquired_Into_Buffer, 250, 0, EveryNCallback, NULL));
    DAQmxErrChk(DAQmxStartTask(gAiTaskHandle));

    return 0;

Error:
    if (error) {
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        fprintf(stderr, "DAQmx Error: %s\n", errBuff);
        CleanUp();
    }
    return error;
}

int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData) {
    int32 read = 0;
    int error = 0;
    static int pumpShotDetected = 0;
    static int purgeShotDetected = 0;
    float64 threshold = 6.9;
    float64 *data = (float64*) malloc(nSamples * 2 * sizeof(float64));  // Adjusted for two channels

    if (!data) {
        fprintf(stderr, "Failed to allocate memory for data.\n");
        CleanUp();
        return -1;
    }

    DAQmxErrChk(DAQmxReadAnalogF64(taskHandle, nSamples, 10.0, DAQmx_Val_GroupByScanNumber, data, nSamples * 2, &read, NULL));

    for (int i = 0; i < read; i += 2) {
        if (CheckPumpShot(data[i], threshold)) {
            if (!pumpShotDetected) {
                printf("Pump shot detected at sample %d: Voltage = %.2f V\n", i/2, data[i]);
                pumpShotDetected = 1;
            }
        } else {
            pumpShotDetected = 0;
        }
        if (CheckPumpShot(data[i + 1], threshold)) {
            if (!purgeShotDetected) {
                printf("Purge shot detected at sample %d: Voltage = %.2f V\n", i/2, data[i + 1]);
                purgeShotDetected = 1;
            }
        } else {
            purgeShotDetected = 0;
        }
    }

    free(data);
    return 0;

Error:
    if (data) free(data);
    fprintf(stderr, "DAQmx Error: %s\n", errBuff);
    CleanUp();
    return -1;
}



int CheckPumpShot(float64 voltage, float64 threshold) {
    return voltage > threshold;
}

void CleanUp() {
    if (gAiTaskHandle != 0) {
        DAQmxStopTask(gAiTaskHandle);
        DAQmxClearTask(gAiTaskHandle);
        gAiTaskHandle = 0;
    }
}

int main() {
    if (CreateAITask(-10.0, 10.0, 250, 200) != 0) {
        fprintf(stderr, "Setup failed.\n");
        return 1;
    }

    printf("Monitoring pump shots. Press Enter to stop and exit.\n");
    getchar();

    CleanUp();
    return 0;
}
