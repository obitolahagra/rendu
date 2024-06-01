//=======================================================================================
// Title:       Analog Input Task for DAQ
// Purpose:     Structure similar to 101029965_DAQ.c
// Created by:  Your Name
// Created on:  Current Date
//=======================================================================================

// REGION START ------------------------------- Includes -------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <NIDAQmx.h>
#include <windows.h>
#include <time.h>
#include "h.h"
// REGION END

// REGION START ---------------------------- Definitions --------------------------------
#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else {}
// REGION END

// REGION START ------------------------ Static Global Variables ------------------------
static TaskHandle gAiTaskHandle = 0;
static float64* gAiAvgSamples = NULL;
static int gAiChanNb = 4; // Number of analog input channels used
char errBuff[2048] = {0};



// REGION END

// REGION START --------------------------- Static Functions ---------------------------
void CleanUp(void);
void Sum1D(const float64 *array, int size, double *sum);
int CheckPumpShot(float64 voltage, float64 threshold);
int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void* callbackData);
int AITask_Create(const char* channels, double sampleRate, int32 numSamples);
int AITask_Start(void);
void AITask_Clear(void);
// REGION END

// REGION START --------------------------- Global Functions ---------------------------

///======================================================================================
// NAME			:	CheckPumpShot															
// DESCRIPTION	:	Verifies that the voltage detected on the AI for pump and purge shots is indeed superior to the threshold
// CREATION		:	13/05/2024																
// AUTHOR		:	Y.Mahroug																
// PARAMETERS	:												
// RETURN VAL  	:	0
///======================================================================================
int CheckPumpShot(float64 voltage, float64 threshold) {
    return voltage > threshold;
}
//=======================================================================================
// NAME			:	AITask_Create															
// DESCRIPTION	:	Creates all the analog input tasks for measurements
// CREATION		:	19/04/2024																
// AUTHOR		:	Y.Mahroug																	
// PARAMETERS	:												
// RETURN VAL 	 :	0
//=======================================================================================
int AITask_Create(const char* channels, double sampleRate, int32 numSamples) {
    int error = 0;

    // Clearing any existing task if it exists
    if (gAiTaskHandle != 0) {
        DAQmxClearTask(gAiTaskHandle);
        gAiTaskHandle = 0;
    }

    // Create the task
    error = DAQmxCreateTask("", &gAiTaskHandle);
    if (error) {
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        fprintf(stderr, "Task Creation Failed: %s\n", errBuff);
        return error;  // If failed, return error code
    }

    // Create analog input voltage channels
    error = DAQmxCreateAIVoltageChan(gAiTaskHandle, channels, "", DAQmx_Val_Cfg_Default, -10.0, 10.0, DAQmx_Val_Volts, NULL);
    if (error) {
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        fprintf(stderr, "Channel Creation Failed: %s\n", errBuff);
        DAQmxClearTask(gAiTaskHandle);
        gAiTaskHandle = 0;
        return error;  // If failed, clear task and return error code
    }

    // Configure the timing for the task
    error = DAQmxCfgSampClkTiming(gAiTaskHandle, "", sampleRate, DAQmx_Val_Rising, DAQmx_Val_ContSamps, numSamples);
    if (error) {
        DAQmxGetExtendedErrorInfo(errBuff, 2048);
        fprintf(stderr, "Timing Configuration Failed: %s\n", errBuff);
        DAQmxClearTask(gAiTaskHandle);
        gAiTaskHandle = 0;
        return error;  // If failed, clear task and return error code
    }

    return 0;  // No errors, task created successfully
}
//=======================================================================================
// NAME			:	AITask_Start														
// DESCRIPTION	:	Starts the AI tasks
// CREATION		:	19/04/2024																
// AUTHOR		:	Y.Mahroug																	
// PARAMETERS	:												
// RETURN VAL 	 :	0
//=======================================================================================
int AITask_Start(void) {
    int error = 0;
    DAQmxErrChk(DAQmxRegisterEveryNSamplesEvent(gAiTaskHandle, DAQmx_Val_Acquired_Into_Buffer, 200, 0, EveryNCallback, NULL));
    DAQmxErrChk(DAQmxStartTask(gAiTaskHandle));
    return 0;

Error:
    CleanUp();
    return error;
}
//=======================================================================================
// NAME			:	EveryNCallback															
// DESCRIPTION	:	Performs the reading and averaging of samples
// CREATION		:	19/04/2024																
// AUTHOR		:	Y.Mahroug																	
// PARAMETERS	:												
// RETURN VAL 	 :	0
//=======================================================================================



int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData) {
    int32 read = 0;
    int error = 0;
	static int gPumpShotDetected = 0;
	static int gPurgeShotDetected = 0;
    static float64 *data = NULL;  // Allocate once, reuse
    static int size = 0;
    
    // Allocate or reallocate based on the number of samples
    if (size < nSamples * gAiChanNb) {
        free(data);
        data = (float64*) malloc(nSamples * gAiChanNb * sizeof(float64));
        size = nSamples * gAiChanNb;
        if (!data) {
            fprintf(stderr, "Failed to allocate memory for data.\n");
            CleanUp();
            return -1;
        }
    }

    DAQmxErrChk(DAQmxReadAnalogF64(taskHandle, nSamples, 10.0, DAQmx_Val_GroupByScanNumber, data, nSamples * gAiChanNb, &read, NULL));

    for (int i = 0; i < gAiChanNb; i++) {
        double sum = 0.0;
        Sum1D(data + i * nSamples, nSamples, &sum);
        gAiAvgSamples[i] = sum / nSamples;
    }

    gPumpShotDetected = CheckPumpShot(gAiAvgSamples[2], 6.9);  
    gPurgeShotDetected = CheckPumpShot(gAiAvgSamples[3], 6.9); 

    return 0;

Error:
    free(data);
    fprintf(stderr, "DAQmx Error: %s\n", errBuff);
    CleanUp();
    return -1;
}

void Sum1D(const float64 *array, int size, double *sum) {
    *sum = 0.0;
    for (int i = 0; i < size; i++) {
        *sum += array[i];
    }
}
//=======================================================================================
// NAME			:	AITask_Clear														
// DESCRIPTION	:	Clears the AI tasks
// CREATION		:	19/04/2024																
// AUTHOR		:	Y.Mahroug																	
// PARAMETERS	:												
// RETURN VAL 	 :	void
//=======================================================================================
void AITask_Clear(void) {
    if (gAiTaskHandle != 0) {
        DAQmxStopTask(gAiTaskHandle);
        DAQmxClearTask(gAiTaskHandle);
        gAiTaskHandle = 0;
    }
}
//=======================================================================================
// NAME			:	Cleanup														
// DESCRIPTION	:	Function to cleanup tasks when debuging
// CREATION		:	19/04/2024																
// AUTHOR		:	Y.Mahroug																	
// PARAMETERS	:												
// RETURN VAL 	 :	void
//=======================================================================================
void CleanUp() {
    AITask_Clear();
    if (gAiAvgSamples != NULL) {
        free(gAiAvgSamples);
        gAiAvgSamples = NULL;
    }
}

//=======================================================================================
// NAME			:	Main
// DESCRIPTION	:	Print statements for debuging
// CREATION		:	19/04/2024																
// AUTHOR		:	Y.Mahroug																	
// PARAMETERS	:												
// RETURN VAL 	 :	0
//=======================================================================================
//int main() {
//    if (AITask_Create("cDAQ1Mod1/ai0:1, cDAQ1Mod1/ai2:3", 125, 200) != 0) {
//        fprintf(stderr, "Setup failed.\n");
//        return 1;
//    }

//    if (AITask_Start() != 0) {
//        fprintf(stderr, "Task start failed.\n");
//        return 1;
//    }

//    printf("Monitoring channels ai0, ai1 for measurements and ai2, ai3 for pump and purge shots. Press Enter to stop and exit.\n");
//    getchar();

//    CleanUp();
//    return 0;
//}
// REGION END