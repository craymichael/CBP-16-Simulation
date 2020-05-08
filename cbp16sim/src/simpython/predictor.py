from abc import ABC, abstractmethod

# Typing defines, strict here to be closer to CPP code
UINT64 = int


# OpType = int  # TODO


# typedef enum {
#     OPTYPE_OP = 2,
#
#     OPTYPE_RET_UNCOND,  // 3
#     OPTYPE_JMP_DIRECT_UNCOND,
#     OPTYPE_JMP_INDIRECT_UNCOND,
#     OPTYPE_CALL_DIRECT_UNCOND,
#     OPTYPE_CALL_INDIRECT_UNCOND,
#
#     OPTYPE_RET_COND,  // 8
#     OPTYPE_JMP_DIRECT_COND,
#     OPTYPE_JMP_INDIRECT_COND,
#     OPTYPE_CALL_DIRECT_COND,
#     OPTYPE_CALL_INDIRECT_COND,
#
#     OPTYPE_ERROR,  // 13
#
#     OPTYPE_MAX  // 14
# } OpType;
class OpType:
    OPTYPE_OP = 2
    OPTYPE_RET_UNCOND = 3
    OPTYPE_JMP_DIRECT_UNCOND = 4
    OPTYPE_JMP_INDIRECT_UNCOND = 5
    OPTYPE_CALL_DIRECT_UNCOND = 6
    OPTYPE_CALL_INDIRECT_UNCOND = 7

    OPTYPE_RET_COND = 8
    OPTYPE_JMP_DIRECT_COND = 9
    OPTYPE_JMP_INDIRECT_COND = 10
    OPTYPE_CALL_DIRECT_COND = 11
    OPTYPE_CALL_INDIRECT_COND = 12

    OPTYPE_ERROR = 13

    OPTYPE_MAX = 14


# noinspection PyPep8Naming
class BASEPREDICTOR(ABC):
    """A dummy abstract predictor for testing with the CBP-16 simulator."""
    __slots__ = ()

    # PREDICTOR(void)
    # def __init__(self):
    #     pass

    # bool GetPrediction(UINT64 PC)
    @abstractmethod
    def GetPrediction(self,
                      PC: UINT64) -> bool:
        """Subclasses must implement this at minimum"""
        raise NotImplementedError

    # void UpdatePredictor(UINT64 PC, OpType opType, bool resolveDir,
    #                      bool predDir, UINT64 branchTarget)
    def UpdatePredictor(self,
                        PC: UINT64,
                        opType: OpType,
                        resolveDir: bool,
                        predDir: bool,
                        branchTarget: UINT64):
        """Subclasses should implement this if the method is online"""
        pass

    # void TrackOtherInst(UINT64 PC, OpType opType, bool taken,
    #                     UINT64 branchTarget)
    def TrackOtherInst(self,
                       PC: UINT64,
                       opType: OpType,
                       taken: bool,
                       branchTarget: UINT64):
        """Subclasses can implement this to track unconditional branches"""
        pass
