///////////////////////////////////////////////////////////////////////
//  Copyright 2015 Samsung Austin Semiconductor, LLC.                //
//            2020 Zach Carmichael                                   //
///////////////////////////////////////////////////////////////////////

//Description : Main file for CBP2016

// https://docs.python.org/3/extending/embedding.html
// https://stackoverflow.com/a/3310608/6557588
// https://docs.python.org/3/c-api/arg.html#c.Py_BuildValue
// https://docs.python.org/3/c-api/object.html
// https://docs.python.org/3/c-api/refcounting.html
#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <map>
using namespace std;

#include "utils.h"
#include "bt9_reader.h"


#define COUNTER     unsigned long long


void CheckHeartBeat(UINT64 numIter, UINT64 numMispred) {
    UINT64 d1K = 1000;
    UINT64 d10K = 10000;
    UINT64 d100K = 100000;
    UINT64 d1M = 1000000;
    UINT64 d10M = 10000000;
    UINT64 d30M = 30000000;
    UINT64 d60M = 60000000;
    UINT64 d100M = 100000000;
    UINT64 d300M = 300000000;
    UINT64 d600M = 600000000;
    UINT64 d1B = 1000000000;
    UINT64 d10B = 10000000000;

    if (numIter == d1K) { //prints MPKI after 100K branches
        printf("  MPKBr_1K         \t : %10.4f", 1000.0 * (double) (numMispred) / (double) (numIter));
        fflush(stdout);
    }

    if (numIter == d10K) { //prints MPKI after 100K branches
        printf("  MPKBr_10K         \t : %10.4f", 1000.0 * (double) (numMispred) / (double) (numIter));
        fflush(stdout);
    }

    if (numIter == d100K) { //prints MPKI after 100K branches
        printf("  MPKBr_100K         \t : %10.4f", 1000.0 * (double) (numMispred) / (double) (numIter));
        fflush(stdout);
    }
    if (numIter == d1M) {
        printf("  MPKBr_1M         \t : %10.4f", 1000.0 * (double) (numMispred) / (double) (numIter));
        fflush(stdout);
    }

    if (numIter == d10M) { //prints MPKI after 100K branches
        printf("  MPKBr_10M         \t : %10.4f", 1000.0 * (double) (numMispred) / (double) (numIter));
        fflush(stdout);
    }

    if (numIter == d30M) { //prints MPKI after 100K branches
        printf("  MPKBr_30M         \t : %10.4f", 1000.0 * (double) (numMispred) / (double) (numIter));
        fflush(stdout);
    }

    if (numIter == d60M) { //prints MPKI after 100K branches
        printf("  MPKBr_60M         \t : %10.4f", 1000.0 * (double) (numMispred) / (double) (numIter));
        fflush(stdout);
    }

    if (numIter == d100M) { //prints MPKI after 100K branches
        printf("  MPKBr_100M         \t : %10.4f", 1000.0 * (double) (numMispred) / (double) (numIter));
        fflush(stdout);
    }

    if (numIter == d300M) { //prints MPKI after 100K branches
        printf("  MPKBr_300M         \t : %10.4f", 1000.0 * (double) (numMispred) / (double) (numIter));
        fflush(stdout);
    }

    if (numIter == d600M) { //prints MPKI after 100K branches
        printf("  MPKBr_600M         \t : %10.4f", 1000.0 * (double) (numMispred) / (double) (numIter));
        fflush(stdout);
    }

    if (numIter == d1B) { //prints MPKI after 100K branches
        printf("  MPKBr_1B         \t : %10.4f", 1000.0 * (double) (numMispred) / (double) (numIter));
        fflush(stdout);
    }

    if (numIter == d10B) { //prints MPKI after 100K branches
        printf("  MPKBr_10B         \t : %10.4f", 1000.0 * (double) (numMispred) / (double) (numIter));
        fflush(stdout);
    }

}//void CheckHeartBeat


inline void pythonCleanup(wchar_t *program) {
    // Python cleanup
    if (Py_FinalizeEx() < 0) {
        exit(120);
    }
    PyMem_RawFree(program); // free up Python memory
}

int main(int argc, char *argv[]) {
    char *predictor_name = "dummy_predictor";
    if (argc == 3) {
        predictor_name = argv[2];
        // Check if .py, remove this to get module name
        int len = strlen(predictor_name);
        if (len > 3 && strcmp(&predictor_name[len - 3], ".py") == 0)
            predictor_name[len - 3] = '\0';
    } else if (argc != 2) {
        printf("usage: %s <trace> [<predictor_module>]\n", argv[0]);
        exit(-1);
    }

    PyObject *brpred;
    // PREDICTOR *brpred = new PREDICTOR();  // this instantiates the predictor code
    // Python init
    wchar_t *program = Py_DecodeLocale(argv[0], NULL);
    if (program == NULL) {
        fprintf(stderr, "Fatal error: cannot decode argv[0]\n");
        exit(1);
    }
    Py_SetProgramName(program);
    Py_Initialize();
    // setup module
    PyObject *module_name = PyUnicode_FromString(predictor_name);

    // Load the module object
    PyObject *module = PyImport_Import(module_name);
    Py_DECREF(module_name);
    if (module == NULL) {
        PyErr_Print();
        fprintf(stderr, "Fatal error: cannot import the module (is it in your PYTHONPATH?)\n");
        pythonCleanup(program);
        return 1;
    }

    // Builds the name of a callable class
    PyObject *python_class = PyObject_GetAttrString(module, "PREDICTOR");
    Py_DECREF(module);
    if (python_class == NULL) {
        PyErr_Print();
        fprintf(stderr, "Fatal error: cannot import the PREDICTOR class (is your predictor class \"PREDICTOR\"?)\n");
        pythonCleanup(program);
        return 1;
    }

    // Creates an instance of the class
    if (PyCallable_Check(python_class)) {
        brpred = PyObject_CallObject(python_class, NULL);
        Py_DECREF(python_class);
    } else {
        if (PyErr_Occurred())
            PyErr_Print();
        fprintf(stderr, "Fatal error: cannot instantiate the PREDICTOR class\n");
        Py_DECREF(python_class);
        pythonCleanup(program);
        return 1;
    }

    // Get relevant methods of PREDICTOR
    PyObject *brpredGetPrediction = PyObject_GetAttrString(brpred, "GetPrediction");
    if (brpredGetPrediction == NULL || !PyCallable_Check(brpredGetPrediction)) {
        if (PyErr_Occurred())
            PyErr_Print();
        fprintf(stderr, "Fatal error: cannot use the PREDICTOR GetPrediction method\n");
        Py_XDECREF(brpredGetPrediction);
        Py_DECREF(brpred);
        pythonCleanup(program);
        return 1;
    }

    PyObject *brpredUpdatePredictor = PyObject_GetAttrString(brpred, "UpdatePredictor");
    if (brpredUpdatePredictor == NULL || !PyCallable_Check(brpredUpdatePredictor)) {
        if (PyErr_Occurred())
            PyErr_Print();
        fprintf(stderr, "Fatal error: cannot use the PREDICTOR UpdatePredictor method\n");
        Py_XDECREF(brpredUpdatePredictor);
        Py_DECREF(brpred);
        pythonCleanup(program);
        return 1;
    }

    PyObject *brpredTrackOtherInst = PyObject_GetAttrString(brpred, "TrackOtherInst");
    if (brpredTrackOtherInst == NULL || !PyCallable_Check(brpredTrackOtherInst)) {
        if (PyErr_Occurred())
            PyErr_Print();
        fprintf(stderr, "Fatal error: cannot use the PREDICTOR TrackOtherInst method\n");
        Py_XDECREF(brpredTrackOtherInst);
        Py_DECREF(brpred);
        pythonCleanup(program);
        return 1;
    }

    Py_DECREF(brpred);
    // End Python init

    ///////////////////////////////////////////////
    // read each trace recrod, simulate until done
    ///////////////////////////////////////////////

    std::string trace_path;
    trace_path = argv[1];
    bt9::BT9Reader bt9_reader(trace_path);

    std::string key = "total_instruction_count:";
    std::string value;
    bt9_reader.header.getFieldValueStr(key, value);
    UINT64 total_instruction_counter = std::stoull(value, nullptr, 0);
//    UINT64 current_instruction_counter = 0;
    key = "branch_instruction_count:";
    bt9_reader.header.getFieldValueStr(key, value);
    UINT64 branch_instruction_counter = std::stoull(value, nullptr, 0);
    UINT64 numMispred = 0;

    UINT64 cond_branch_instruction_counter = 0;
    UINT64 uncond_branch_instruction_counter = 0;

    ///////////////////////////////////////////////
    // read each trace record, simulate until done
    ///////////////////////////////////////////////

    OpType opType;
    UINT64 PC;
    bool branchTaken;
    UINT64 branchTarget;
    UINT64 numIter = 0;

    PyObject *PyPredDir;
    PyObject *PyTemp;
    PyObject *PyTempValue;

    for (auto it = bt9_reader.begin(); it != bt9_reader.end(); ++it) {
        CheckHeartBeat(++numIter, numMispred); //Here numIter will be equal to number of branches read

        try {
            bt9::BrClass br_class = it->getSrcNode()->brClass();

            //JD2_2_2016 break down branch instructions into all possible types
            opType = OPTYPE_ERROR;

            if ((br_class.type == bt9::BrClass::Type::UNKNOWN) &&
                (it->getSrcNode()->brNodeIndex())) { //only fault if it isn't the first node in the graph (fake branch)
                opType = OPTYPE_ERROR; //sanity check
            } else if (br_class.type == bt9::BrClass::Type::RET) {
                if (br_class.conditionality == bt9::BrClass::Conditionality::CONDITIONAL)
                    opType = OPTYPE_RET_COND;
                else if (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL)
                    opType = OPTYPE_RET_UNCOND;
                else {
                    opType = OPTYPE_ERROR;
                }
            } else if (br_class.directness == bt9::BrClass::Directness::INDIRECT) {
                if (br_class.type == bt9::BrClass::Type::CALL) {
                    if (br_class.conditionality == bt9::BrClass::Conditionality::CONDITIONAL)
                        opType = OPTYPE_CALL_INDIRECT_COND;
                    else if (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL)
                        opType = OPTYPE_CALL_INDIRECT_UNCOND;
                    else {
                        opType = OPTYPE_ERROR;
                    }
                } else if (br_class.type == bt9::BrClass::Type::JMP) {
                    if (br_class.conditionality == bt9::BrClass::Conditionality::CONDITIONAL)
                        opType = OPTYPE_JMP_INDIRECT_COND;
                    else if (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL)
                        opType = OPTYPE_JMP_INDIRECT_UNCOND;
                    else {
                        opType = OPTYPE_ERROR;
                    }
                } else {
                    opType = OPTYPE_ERROR;
                }
            } else if (br_class.directness == bt9::BrClass::Directness::DIRECT) {
                if (br_class.type == bt9::BrClass::Type::CALL) {
                    if (br_class.conditionality == bt9::BrClass::Conditionality::CONDITIONAL) {
                        opType = OPTYPE_CALL_DIRECT_COND;
                    } else if (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL) {
                        opType = OPTYPE_CALL_DIRECT_UNCOND;
                    } else {
                        opType = OPTYPE_ERROR;
                    }
                } else if (br_class.type == bt9::BrClass::Type::JMP) {
                    if (br_class.conditionality == bt9::BrClass::Conditionality::CONDITIONAL) {
                        opType = OPTYPE_JMP_DIRECT_COND;
                    } else if (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL) {
                        opType = OPTYPE_JMP_DIRECT_UNCOND;
                    } else {
                        opType = OPTYPE_ERROR;
                    }
                } else {
                    opType = OPTYPE_ERROR;
                }
            } else {
                opType = OPTYPE_ERROR;
            }

            PC = it->getSrcNode()->brVirtualAddr();

            branchTaken = it->getEdge()->isTakenPath();
            branchTarget = it->getEdge()->brVirtualTarget();

/************************************************************************************************************/

            if (opType == OPTYPE_ERROR) {
                if (it->getSrcNode()->brNodeIndex()) { //only fault if it isn't the first node in the graph (fake branch)
                    fprintf(stderr, "OPTYPE_ERROR\n");
                    printf("OPTYPE_ERROR\n");
                    Py_DECREF(brpredGetPrediction);
                    Py_DECREF(brpredUpdatePredictor);
                    Py_DECREF(brpredTrackOtherInst);
                    pythonCleanup(program);
                    exit(-1); //this should never happen, if it does please email CBP org chair.
                }
            } else if (br_class.conditionality ==
                       bt9::BrClass::Conditionality::CONDITIONAL) { //JD2_17_2016 call UpdatePredictor() for all branches that decode as conditional

                bool predDir = false;

                // predDir = brpred->GetPrediction(PC);
                PyTempValue = Py_BuildValue("(I)", PC);
                PyPredDir = PyObject_CallObject(brpredGetPrediction, PyTempValue);
                if (PyPredDir == NULL) {
                    PyErr_Print();
                    fprintf(stderr, "Fatal error: did not call PREDICTOR GetPrediction successfully.");
                    Py_XDECREF(PyTempValue);
                    Py_DECREF(brpredGetPrediction);
                    Py_DECREF(brpredUpdatePredictor);
                    Py_DECREF(brpredTrackOtherInst);
                    pythonCleanup(program);
                    exit(1);
                }
                Py_DECREF(PyTempValue);
                predDir = (bool) PyObject_IsTrue(PyPredDir);
                Py_DECREF(PyPredDir);

//                brpred->UpdatePredictor(PC, opType, branchTaken, predDir, branchTarget);
                PyTempValue = Py_BuildValue("(I,l,i,i,I)", PC, opType, branchTaken, predDir, branchTarget);
                PyTemp = PyObject_CallObject(brpredUpdatePredictor, PyTempValue);
                if (PyTemp == NULL) {
                    PyErr_Print();
                    fprintf(stderr, "Fatal error: did not call PREDICTOR UpdatePredictor successfully.");
                    Py_XDECREF(PyTempValue);
                    Py_DECREF(brpredGetPrediction);
                    Py_DECREF(brpredUpdatePredictor);
                    Py_DECREF(brpredTrackOtherInst);
                    pythonCleanup(program);
                    exit(1);
                }
                Py_DECREF(PyTempValue);
                Py_DECREF(PyTemp);

                if (predDir != branchTaken) {
                    numMispred++; // update mispred stats
                }

                cond_branch_instruction_counter++;
            } else if (br_class.conditionality ==
                       bt9::BrClass::Conditionality::UNCONDITIONAL) { // for predictors that want to track unconditional branches
                uncond_branch_instruction_counter++;
                // brpred->TrackOtherInst(PC, opType, branchTaken, branchTarget);
                // TODO: add error checking here???
                PyTempValue = Py_BuildValue("(I,l,i,I)", PC, opType, branchTaken, branchTarget);
                PyTemp = PyObject_CallObject(brpredTrackOtherInst, PyTempValue);
                if (PyTemp == NULL) {
                    PyErr_Print();
                    fprintf(stderr, "Fatal error: did not call PREDICTOR TrackOtherInst successfully.");
                    Py_XDECREF(PyTempValue);
                    Py_DECREF(brpredGetPrediction);
                    Py_DECREF(brpredUpdatePredictor);
                    Py_DECREF(brpredTrackOtherInst);
                    pythonCleanup(program);
                    exit(1);
                }
                Py_DECREF(PyTempValue);
                Py_DECREF(PyTemp);
            } else {
                fprintf(stderr, "CONDITIONALITY ERROR\n");
                printf("CONDITIONALITY ERROR\n");
                pythonCleanup(program);
                exit(-1); //this should never happen, if it does please email CBP org chair.
            }

/************************************************************************************************************/
        }
        catch (const std::out_of_range &ex) {
            std::cout << ex.what() << '\n';
            break;
        }

    } //for (auto it = bt9_reader.begin(); it != bt9_reader.end(); ++it)

    ///////////////////////////////////////////
    //print_stats
    ///////////////////////////////////////////

    //NOTE: competitors are judged solely on MISPRED_PER_1K_INST. The additional stats are just for tuning your predictors.

    printf("  TRACE \t : %s", trace_path.c_str());
    printf("  NUM_INSTRUCTIONS            \t : %10llu", total_instruction_counter);
    printf("  NUM_BR                      \t : %10llu",
           branch_instruction_counter - 1); //JD2_2_2016 NOTE there is a dummy branch at the beginning of the trace...
    printf("  NUM_UNCOND_BR               \t : %10llu", uncond_branch_instruction_counter);
    printf("  NUM_CONDITIONAL_BR          \t : %10llu", cond_branch_instruction_counter);
    printf("  NUM_MISPREDICTIONS          \t : %10llu", numMispred);
    printf("  MISPRED_PER_1K_INST         \t : %10.4f",
           1000.0 * (double) (numMispred) / (double) (total_instruction_counter));
    printf("\n");

    Py_DECREF(brpredGetPrediction);
    Py_DECREF(brpredUpdatePredictor);
    Py_DECREF(brpredTrackOtherInst);
    pythonCleanup(program);
    return 0;
}
