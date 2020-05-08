from predictor import BASEPREDICTOR, UINT64, OpType

DEBUG = True


# noinspection PyPep8Naming
class PREDICTOR(BASEPREDICTOR):
    """A dummy abstract predictor for testing with the CBP-16 simulator."""
    __slots__ = ()

    # bool GetPrediction(UINT64 PC)
    def GetPrediction(self,
                      PC: UINT64) -> bool:
        if DEBUG:
            print('GetPrediction | PC =', PC)
        # Always predict taken
        return True

    # void UpdatePredictor(UINT64 PC, OpType opType, bool resolveDir,
    #                      bool predDir, UINT64 branchTarget)
    def UpdatePredictor(self,
                        PC: UINT64,
                        opType: OpType,
                        resolveDir: bool,
                        predDir: bool,
                        branchTarget: UINT64):
        if DEBUG:
            print(
                'UpdatePredictor | PC = {} OpType = {} resolveDir = {} '
                'predDir = {} branchTarget = {}'.format(
                    PC, opType, resolveDir, predDir, branchTarget)
            )

    # void TrackOtherInst(UINT64 PC, OpType opType, bool taken,
    #                     UINT64 branchTarget)
    def TrackOtherInst(self,
                       PC: UINT64,
                       opType: OpType,
                       taken: bool,
                       branchTarget: UINT64):
        if DEBUG:
            print(
                'UpdatePredictor | PC = {} OpType = {} '
                'taken = {} branchTarget = {}'.format(
                    PC, opType, taken, branchTarget)
            )
