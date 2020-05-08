///////////////////////////////////////////////////////////////////////
//  Copyright 2015 Samsung Austin Semiconductor, LLC.                //
//            2020 Zach Carmichael                                   //
///////////////////////////////////////////////////////////////////////

//Description : Main file for CBP2016

#include <iostream>
#include <fstream>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <map>
using namespace std;

#include "utils.h"
#include "bt9_reader.h"
#include "predictor.h"


#define COUNTER     unsigned long long

//#define SAVE_CSV
#define SAVE_BINARY


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


#ifdef SAVE_BINARY
struct BinDataPoint {
    bool branchTaken;
    bool predDir;
    bool conditional;
    OpType opType;
    UINT64 branchTarget;
    UINT64 PC;
};
#endif

// usage: predictor <trace>

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("usage: %s <trace>\n", argv[0]);
        exit(-1);
    }

    ///////////////////////////////////////////////
    // Init variables
    ///////////////////////////////////////////////

    ///////////////////////////////////////////////
    // read each trace recrod, simulate until done
    ///////////////////////////////////////////////

    std::string trace_path;
    trace_path = argv[1];

#ifdef SAVE_CSV
    std::ofstream csvFile;
    csvFile.open(trace_path + ".csv");

    csvFile << "PC,conditional,branchTaken,predDir,opType,branchTarget\n";
#endif

#ifdef SAVE_BINARY
    ofstream binFile(trace_path + ".dat", ios::out | ios::binary);
    if (!binFile) {
        std::cout << "Cannot open file!" << std::endl;
        return 1;
    }
    BinDataPoint dp;
    std::cout << "Size of BinDataPoint: " << sizeof(BinDataPoint) << std::endl;
    std::cout << "    branchTaken:  " << sizeof(dp.branchTaken) << '(' << &dp.branchTaken << ')' << std::endl;
    std::cout << "    predDir:      " << sizeof(dp.predDir) << '(' << &dp.predDir << ')' << std::endl;
    std::cout << "    conditional:  " << sizeof(dp.conditional) << '(' << &dp.conditional << ')' << std::endl;
    std::cout << "    opType:       " << sizeof(dp.opType) << '(' << &dp.opType << ')' << std::endl;
    std::cout << "    branchTarget: " << sizeof(dp.branchTarget) << '(' << &dp.branchTarget << ')' << std::endl;
    std::cout << "    PC:           " << sizeof(dp.PC) << '(' << &dp.PC << ')' << std::endl;
#endif

    PREDICTOR *brpred = new PREDICTOR();  // this instantiates the predictor code

    bt9::BT9Reader bt9_reader(trace_path);

    std::string key = "total_instruction_count:";
    std::string value;
    bt9_reader.header.getFieldValueStr(key, value);
    UINT64 total_instruction_counter = std::stoull(value, nullptr, 0);
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
#ifdef SAVE_CSV
            csvFile << std::to_string(PC) + ","; //PC
#endif

#ifdef SAVE_BINARY
            dp.PC = PC;
#endif

            if (opType == OPTYPE_ERROR) {
                if (it->getSrcNode()->brNodeIndex()) { //only fault if it isn't the first node in the graph (fake branch)
                    fprintf(stderr, "OPTYPE_ERROR\n");
                    printf("OPTYPE_ERROR\n");
                    exit(-1); //this should never happen, if it does please email CBP org chair.
                }
#ifdef SAVE_CSV
                csvFile << ",,,,\n"; //nothing else to see here
#endif
            } else if (br_class.conditionality ==
                       bt9::BrClass::Conditionality::CONDITIONAL) { //JD2_17_2016 call UpdatePredictor() for all branches that decode as conditional

                bool predDir = false;

                predDir = brpred->GetPrediction(PC);
                brpred->UpdatePredictor(PC, opType, branchTaken, predDir, branchTarget);

#ifdef SAVE_CSV
                //conditional,branchTaken,predDir,opType,branchTarget
                csvFile << "1," + std::to_string(branchTaken) + "," + std::to_string(predDir) +
                    "," + std::to_string(opType) + "," + std::to_string(branchTarget) + "\n";
#endif
#ifdef SAVE_BINARY
                dp.conditional = true;
                dp.branchTaken = branchTaken;
                dp.predDir = predDir;
                dp.opType = opType;
                dp.branchTarget = branchTarget;
                binFile.write((char *) &dp, sizeof(BinDataPoint));
#endif

                if (predDir != branchTaken) {
                    numMispred++; // update mispred stats
                }
                cond_branch_instruction_counter++;
            } else if (br_class.conditionality ==
                       bt9::BrClass::Conditionality::UNCONDITIONAL) { // for predictors that want to track unconditional branches
                uncond_branch_instruction_counter++;
                brpred->TrackOtherInst(PC, opType, branchTaken, branchTarget);
#ifdef SAVE_CSV
                //conditional,branchTaken,_,opType,branchTarget
                csvFile << "0," + std::to_string(branchTaken) + ",," +
                       std::to_string(opType) + "," + std::to_string(branchTarget) + "\n";
#endif
#ifdef SAVE_BINARY
                // NOTE: must ignore predDir here (unconditional branch...)
                dp.conditional = false;
                dp.branchTaken = branchTaken;
                dp.opType = opType;
                dp.branchTarget = branchTarget;
                binFile.write((char *) &dp, sizeof(BinDataPoint));
#endif
            } else {
                fprintf(stderr, "CONDITIONALITY ERROR\n");
                printf("CONDITIONALITY ERROR\n");
                exit(-1); //this should never happen, if it does please email CBP org chair.
            }

/************************************************************************************************************/
        }
        catch (const std::out_of_range &ex) {
            std::cout << ex.what() << '\n';
            break;
        }

    } //for (auto it = bt9_reader.begin(); it != bt9_reader.end(); ++it)

#ifdef SAVE_CSV
    csvFile.close();
#endif

#ifdef SAVE_BINARY
    binFile.close();
    if (!binFile.good()) {
        std::cout << "Error occurred at writing time!" << std::endl;
        return 1;
    }
#endif

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
}
