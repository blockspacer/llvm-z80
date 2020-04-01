#include "Z80.h"
#include "Z80TargetMachine.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "Z80Subtarget.h"
//#include "Z80InstrInfo.h"

#define DEBUG_TYPE "Z80-isel"

using namespace llvm;

class Z80DAGToDAGISel : public SelectionDAGISel {
  const Z80Subtarget *Subtarget;
public:
  explicit Z80DAGToDAGISel(Z80TargetMachine &TM, CodeGenOpt::Level OptLevel)
          : SelectionDAGISel(TM, OptLevel) {}

  StringRef getPassName() const override {
    return "Z80 DAG->DAG Pattern Instruction Selection";
  }

  bool runOnMachineFunction(MachineFunction &MF) override {
    Subtarget = &MF.getSubtarget<Z80Subtarget>();
    return SelectionDAGISel::runOnMachineFunction(MF);
  }

  void Select(SDNode *N) override;

//	bool SelectAddr(SDValue Addr, SDValue &Base, SDValue &Offset);

  bool SelectAddrFI(SDValue Addr, SDValue &Base);

  // Include the pieces autogenerated from the target description.
#include "Z80GenDAGISel.inc"
};

//
static SDNode *selectImm(SelectionDAG *CurDAG, const SDLoc &DL, int64_t Imm,
                         EVT XLenVT) {

  if (XLenVT == MVT::i16 && isInt<16>(Imm)) {
    SDValue SDImm = CurDAG->getTargetConstant(Imm, DL, XLenVT);

    return CurDAG->getMachineNode(Z80::LD16ri, DL, XLenVT, SDImm);
  } else if (XLenVT == MVT::i8 && isInt<8>(Imm)) {
    SDValue SDImm = CurDAG->getTargetConstant(Imm, DL, XLenVT);
    return CurDAG->getMachineNode(Z80::LD8ri, DL, XLenVT, SDImm);
  }
//  else if (isInt<16>(Imm)) {
//    SDValue SDImm = CurDAG->getTargetConstant(Imm, DL, XLenVT);
//
//    return CurDAG->getMachineNode(Z80::, DL, XLenVT, ZeroReg, SDImm);
//  }

  llvm_unreachable("cannot select immediate that doesn't fit in imm16");
}

void Z80DAGToDAGISel::Select(SDNode *Node) {
// If we have a custom node, we have already selected.
  if (Node->isMachineOpcode()) {
//    LLVM_DEBUG(dbgs() << "== "; Node->dump(CurDAG); dbgs() << "\n");
    Node->setNodeId(-1);
    return;
  }

  // Instruction Selection not handled by the auto-generated tablegen selection
  // should be handled here.
  unsigned Opcode = Node->getOpcode();
  MVT XLenVT = MVT::i16;
  SDLoc DL(Node);
  EVT VT = Node->getValueType(0);

  switch (Opcode) {
    case ISD::Constant: {
      auto ConstNode = cast<ConstantSDNode>(Node);
      int64_t Imm = ConstNode->getSExtValue();
      if (VT == MVT::i16 || VT == MVT::i8) {
        ReplaceNode(Node, selectImm(CurDAG, SDLoc(Node), Imm, VT));
        return;
      }
      break;
    }
    case ISD::FrameIndex: {
//      SDValue Imm = CurDAG->getTargetConstant(0, DL, XLenVT);
      int FI = cast<FrameIndexSDNode>(Node)->getIndex();
      SDValue TFI = CurDAG->getTargetFrameIndex(FI, VT);
      assert(VT == MVT::i16 && "VT should be a pointer type size");
      ReplaceNode(Node, CurDAG->getMachineNode(Z80::LD16ri, DL, VT, TFI));
      return;
    }
  }

  // Select the default instruction.
  SelectCode(Node);
}

bool Z80DAGToDAGISel::SelectAddrFI(SDValue Addr, SDValue &Base) {
  if (auto FIN = dyn_cast<FrameIndexSDNode>(Addr)) {
    Base = CurDAG->getTargetFrameIndex(FIN->getIndex(), MVT::i32);
    return true;
  }
  return false;
}

FunctionPass *
llvm::createZ80ISelDag(Z80TargetMachine &TM, CodeGenOpt::Level OptLevel) {
  return new Z80DAGToDAGISel(TM, OptLevel);
}