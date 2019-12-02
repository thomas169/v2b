/**************************************************************************
* Program:      Vector to Bus
*
* File:         v2b_sf.c
*
* Description:  Converts a vector signal into a bus. Uses only 1 input port
*               and 1 output port. The input type must (obviously) be of 
*               all the same type. These inputs are stored in a buffer then
*               cast to the appropriate element type of the output bus.
*
* Notes:        Works with nested buses. Compile mex (on linux) with:
*                   mex('CFLAGS="$CFLAGS -std=c99 -Wall"','v2b_sf.c')
*               Input may be of type double only.
*
* Revsions:     1.00 05/11/19 (tf) First release.
*
* See also:     v2b_sf.tlc
**************************************************************************/

// Preamble: defines & includes
#define S_FUNCTION_NAME v2b_sf
#define S_FUNCTION_LEVEL 2

// Always needed modules and simstruc
#include "simstruc.h"
#include <stdint.h>
#include <stdbool.h>

#ifndef SS_STDIO_AVAILABLE
    #define SS_STDIO_AVAILABLE
#endif

#define MDL_START 
#undef MDL_UPDATE 
#undef MDL_DERIVATIVES
#ifdef MATLAB_MEX_FILE
    #define MDL_CHECK_PARAMETERS
    #define MDL_SET_INPUT_PORT_WIDTH
    #define MDL_SET_INPUT_PORT_DATA_TYPE
    #define MDL_SET_OUTPUT_PORT_WIDTH
    #define MDL_SET_OUTPUT_PORT_DATA_TYPE
    #define MDL_SET_DEFAULT_PORT_DATA_TYPES
    #define MDL_SET_DEFAULT_PORT_DIMENSION_INFO
    #define MDL_RTW
#endif

#define N_PARAMS        1
#define P_OP_BUS_NAME   0

#define N_DWORK         1
#define DW_BUF_SIZE     0

#define PARAM_VAL(PIDX) ssGetSFcnParam(S, (PIDX))
#define MAX(a,b) ((a) > (b)) ? (a) : (b)

#define DEBUG 1
#define WARN_STRLEN 64
char infoStr[WARN_STRLEN];
#define INFO(fmt,...) snprintf(infoStr,WARN_STRLEN,(fmt),##__VA_ARGS__)


// Function: mdlCheckParameters ===========================================
// Abstract: Verifies the numer of and contents of block parammeters if 
// changed during a simulation.
#ifdef MDL_CHECK_PARAMETERS
static void mdlCheckParameters(SimStruct *S) {
}
#endif

// Function: mdlInitializeSizes ===========================================
// Abstract: Setup size, type, and  options of block parameters, ports,
// work vectors and sample times. Also sets many sfun block options.
static void mdlInitializeSizes(SimStruct *S) {
    
    // Number of expected parameters and check paramemters
    ssSetNumSFcnParams(S, N_PARAMS);

    // Set the parameters to non-tuneable
    for (int iParam = 0; iParam < N_PARAMS; iParam++) {
        ssSetSFcnParamTunable(S, iParam, SS_PRM_NOT_TUNABLE);
    }

    // Inputs - number of and settings
    ssSetNumInputPorts(S, 1);
    ssSetInputPortDirectFeedThrough(S, 0,1);
    ssSetInputPortRequiredContiguous(S, 0, 1);
    ssSetInputPortWidth(S, 0, DYNAMICALLY_SIZED);
    ssSetInputPortDataType(S, 0, SS_DOUBLE);

    // Outputs - number of and settings and bus registartion
    char *name;
    ssGetSFcnParamName(S,P_OP_BUS_NAME,&name);
    
    DTypeId opDataTypeId;
    ssRegisterTypeFromNamedObject(S,name,&opDataTypeId);
    
    ssSetBusOutputObjectName(S, 0, name);
    ssSetBusOutputAsStruct(S, 0, 1);
    if (opDataTypeId == INVALID_DTYPE_ID){
        ssSetErrorStatus(S,"Invalid type");
        return;
    }
    ssSetNumOutputPorts(S, 1);
    ssSetOutputPortWidth(S, 0, 1);
    ssSetOutputPortDataType(S, 0, opDataTypeId);

    // dwork stuff
    ssSetNumDWork(S, N_DWORK);
    ssSetDWorkDataType(S, DW_BUF_SIZE, SS_INT32);
    ssSetDWorkWidth(S, DW_BUF_SIZE, 3);
    ssSetDWorkComplexSignal(S, DW_BUF_SIZE, COMPLEX_NO);
    ssSetDWorkName(S, DW_BUF_SIZE, "BUF_SIZE");
    
    // Sample times
    ssSetNumSampleTimes(S, 1);

    // Specify the sim state compliance to be same as a built-in block
    ssSetOptions(S, SS_OPTION_USE_TLC_WITH_ACCELERATOR | 
            SS_OPTION_WORKS_WITH_CODE_REUSE);
    
    ssPrintf("V2B: Callback Fired\n");
    
}
// Function: mdlSetInputPortWidth =========================================
// Abstract: ...
#ifdef MDL_SET_INPUT_PORT_WIDTH
static void mdlSetInputPortWidth(SimStruct *S, int_T port, int_T width){
    ssSetInputPortWidth(S, port, width);
}
#endif

// Function: mdlSetInputPortDataType ======================================
// Abstract: ...
#ifdef MDL_SET_INPUT_PORT_DATA_TYPE
void mdlSetInputPortDataType(SimStruct *S, int_T port, DTypeId id){
    ssSetInputPortDataType(S,port,id);
}
#endif

// Function: mdlSetOutputPortWidth ========================================
// Abstract: ...
#ifdef MDL_SET_OUTPUT_PORT_WIDTH
static void mdlSetOutputPortWidth(SimStruct *S, int_T port, int_T width){
    ssSetOutputPortWidth(S, port, width);
}
#endif

// Function: mdlSetOutputPortDataType =====================================
// Abstract: ...
#ifdef MDL_SET_OUTPUT_PORT_DATA_TYPE
void mdlSetOutputPortDataType(SimStruct *S, int_T port, DTypeId id){
    ssSetOutputPortDataType(S,port,id);
}
#endif

// Function: mdlSetDefaultPortDimensionInfo ===============================
// Abstract: Set the dimensions of dynamically sized ports
#ifdef MDL_SET_DEFAULT_PORT_DIMENSION_INFO
static void mdlSetDefaultPortDimensionInfo(SimStruct *S){

    if (ssGetInputPortWidth(S, 0) == DYNAMICALLY_SIZED){
        ssSetInputPortWidth(S, 0, 1);
        ssWarning(S,"Input port using default port width of 1");
    }
    
    if (ssGetOutputPortWidth(S, 0) == DYNAMICALLY_SIZED){
        ssSetOutputPortWidth(S, 0, 1);
        ssWarning(S,"Output port using default port width of 1");
    }
}
#endif

// Function: mdlSetDefaultPortDataTypes ===================================
// Abstract: Set data type info of dynamically typed ports
#ifdef MDL_SET_DEFAULT_PORT_DATA_TYPES
void mdlSetDefaultPortDataTypes(SimStruct *S){
    if (ssGetInputPortDataType(S, 0) == DYNAMICALLY_TYPED){
        ssSetInputPortDataType(S, 0, SS_DOUBLE);
        ssWarning(S,"Input port using default port type of double");
    }

    if (ssGetOutputPortDataType(S, 0) == DYNAMICALLY_TYPED){
        ssSetOutputPortDataType(S, 0, SS_DOUBLE);
        ssWarning(S,"Input port using default port type of double");
    }
}
#endif


// Function: mdlInitializeSampleTimes =====================================
// Abstract: Set the block or port based sample times.
static void mdlInitializeSampleTimes(SimStruct *S){
    ssSetSampleTime(S, 0, INHERITED_SAMPLE_TIME);
    ssSetOffsetTime(S, 0, 0.0);
    ssSetModelReferenceSampleTimeDefaultInheritance(S);
}

// Function: ssGetNumBusElementsNested ====================================
// Abstract: Getnumber of builtin type elements in a (nested) bus. This 
// function really should be built into simulink.
static int ssGetNumBusElementsNested(SimStruct *S, DTypeId type){
    
    DTypeId nestedType;
    const int* dims;
    int nDim;
    int elemCount = 0;
    int elemNumel = 0;;
    
    for (int elem = 0 ; elem < ssGetNumBusElements(S,type) ; elem++) {
        nestedType = ssGetBusElementDataType(S,type,elem);
        if (ssIsDataTypeABus(S,nestedType)) {
            elemCount += ssGetNumBusElementsNested(S, nestedType);
        } else {
            dims = ssGetBusElementDimensions(S,type,elem);
            nDim = ssGetBusElementNumDimensions(S,type,elem); 
            if (nDim == 1) // yeh i know...
                elemNumel = dims[0];
            else
                elemNumel = (dims[0] * dims[1]);
            elemCount += elemNumel;
            ssPrintf("Added %d\n",elemNumel);
        }
    }
    return elemCount;
}

// Function: mdlStart =====================================================
// Abstract: ...
#ifdef MDL_START
static void mdlStart(SimStruct *S) {
    
    int *bufSize = (int*) ssGetDWork(S, DW_BUF_SIZE);
    int ipNumel, opNumel;
    DTypeId dType;
        
    ipNumel = ssGetInputPortWidth(S, 0);
    dType = ssGetOutputPortDataType(S, 0);
    opNumel = ssGetNumBusElementsNested(S,dType);

    bufSize[0] = ipNumel * ssGetDataTypeSize(S,ssGetInputPortDataType(S,0));
    bufSize[1] = ssGetDataTypeSize(S,ssGetInputPortDataType(S,0)); // (8 as dbl)
    bufSize[2] = ssGetDataTypeSize(S,dType);

    ssPrintf("IP/OP size (%d[%d],%d)\n",bufSize[0],bufSize[1],bufSize[2]);
    
    if (ipNumel < opNumel) {
        snprintf(infoStr,WARN_STRLEN,"Input vector too small (%d < %d)",
            ipNumel,opNumel);
        ssWarning(S,infoStr);
    } else if (ipNumel > opNumel) {
        snprintf(infoStr,WARN_STRLEN,"Input vector too big (%d > %d)",
            ipNumel,opNumel);
        ssWarning(S,infoStr);
    }   
}
#endif

// Function: sugar ========================================================
// Abstract: Cast from doubles to any simulink builtin type. The input and 
// output buffers are byte arrays. The position within each of is tracked.
void sugar(char* ipBuf, char* opBuf, int opType, int numel){
    for (int i = 0 ; i < numel ; i++) {
        double dbl = *(double*) (&ipBuf[i*8]);
        switch (opType) {
            case SS_DOUBLE: {
                double value = (double) dbl;
                memcpy(&opBuf[i*8],&value,sizeof(double));
                } break;
            case SS_SINGLE: {
                float value = (float) dbl;
                memcpy(&opBuf[i*4],&value,sizeof(float));
                } break;
            case SS_INT8: {
                int8_t value = (int8_t) dbl;
                memcpy(&opBuf[i*1],&value,sizeof(int8_t));
                } break;
            case SS_UINT8: {
                uint8_t value = (uint8_t) dbl;
                memcpy(&opBuf[i*1],&value,sizeof(uint8_t));
                } break;
            case SS_INT16: {
                int16_t value = (int16_t) dbl;
                memcpy(&opBuf[i*2],&value,sizeof(int16_t));
                } break;
            case SS_UINT16: {
                uint16_t value = (uint16_t) dbl;
                memcpy(&opBuf[i*2],&value,sizeof(uint16_t));
                } break;
            case SS_UINT32: {
                uint32_t value = (uint32_t) dbl;
                memcpy(&opBuf[i*4],&value,sizeof(uint32_t));
                } break;
            case SS_INT32: {
                int32_t value = (int32_t) dbl;
                memcpy(&opBuf[i*4],&value,sizeof(int32_t));
                } break;
            case SS_BOOLEAN: {
                bool value = (bool) dbl;
                memcpy(&opBuf[i*1],&value,sizeof(bool));
                } break;
        }
    }
}

// Function: busExplore ===================================================
// Abstract: Recursively explore the bus signals in use until we reach a 
// builtin type. The code in this fuction is not suitable for any code 
// generation and TLC must be provided to use this SF with codegen tools.
int busExplore(SimStruct *S, int type, int pos, int idx, char* ipBuf, char* opBuf) {
    
    int nElem = ssGetNumBusElements(S,type);

    for (int iElem = 0 ; iElem < nElem; iElem++) {
        int nWidth = 0;
        int elemType = ssGetBusElementDataType(S,type,iElem);
        int elemPos = ssGetBusElementOffset(S,type,iElem);
        const int* dims = ssGetBusElementDimensions(S,type,iElem);
        int nDim = ssGetBusElementNumDimensions(S,type,iElem); 
        if (nDim == 1) 
            nWidth = dims[0];
        else
            nWidth = (dims[0] * dims[1]);
        
        if (ssIsDataTypeABus(S,elemType)) {
            idx = busExplore(S, elemType, pos+elemPos, idx, ipBuf, opBuf);
        } else {
            sugar(&ipBuf[idx * 8], &opBuf[pos + elemPos], elemType, nWidth);
            if (ssGetT(S) == 0) {
                #if DEBUG
                INFO("Elem: %-15.15s [%-7.7s], Pos: %d, Idx: %d, Width %d\n",
                    (char *) ssGetBusElementName(S,type,iElem),
                    (char *) ssGetDataTypeName(S,elemType),
                    pos+elemPos,idx,nWidth);
                ssPrintf("%s",infoStr);
                #endif
            }
            idx += nWidth;
        }
    }
    return idx;
}

// Function: mdlOutputs ===================================================
// Abstract: Read the model inputs and set to the outputs.
static void mdlOutputs(SimStruct *S, int_T tid){

    int *bufSize = (int*) ssGetDWork(S, DW_BUF_SIZE);
    char ipBuffer[bufSize[0]];
    memcpy(ipBuffer,ssGetInputPortSignal(S,0), bufSize[0]);
    void* sigRoot = ssGetOutputPortSignal(S,0);
    int busTypeId = ssGetOutputPortDataType(S,0);
    busExplore(S, busTypeId, 0, 0, ipBuffer, (char*) sigRoot);

}


// Function: mdlTerminate =================================================
// Abstract: Not used, but must be defined.
static void mdlTerminate(SimStruct *S){
    UNUSED_ARG(S);
}

// Function: mdlRTW =======================================================
// Abstract: mdlRTW implamented to enforce TLC requirment for codegen. Not
// used otherwise.
#ifdef MDL_RTW
void mdlRTW(SimStruct *S) {
    UNUSED_ARG(S);
}
#endif

// Required S-function trailer ============================================
#ifdef  MATLAB_MEX_FILE
    #include "simulink.c"
#else
    #include "cg_sfun.h"
#endif