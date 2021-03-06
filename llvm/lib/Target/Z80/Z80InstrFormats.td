
def Z80BranchCCAsmOperand: AsmOperandClass {
  let Name = "BranchCC";
  let ParserMethod = "parseBranchCC";
  let PredicateMethod = "isBranchCC";
  let RenderMethod = "addBranchCCOperand";
}

// Operand Types
def branch_target : Operand<OtherVT> {
  let EncoderMethod = "getJumpTargetOpValue";
  // let ParserMatchClass = Z80JumpTargetAsmOperand;
}

def branch_cc : Operand<OtherVT> {
    let EncoderMethod = "getBranchConditionCodeValue";
    let ParserMatchClass = Z80BranchCCAsmOperand;
}

def imm8: Operand<i8>, ImmLeaf<i8, [{return isInt<8>(Imm);}]> {
  let EncoderMethod = "getImmOpValue";
  let MCOperandPredicate = [{
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isInt<8>(Imm);
    return MCOp.isBareSymbolRef();
  }];
}

def imm16: Operand<i16>, ImmLeaf<i16, [{return isInt<16>(Imm);}]> {
  let EncoderMethod = "getImmOpValue";
  let MCOperandPredicate = [{
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isInt<16>(Imm);
    return MCOp.isBareSymbolRef();
  }];
}

def addr16: Operand<i16> {
  let EncoderMethod = "getImmOpValue";
  let OperandType = "OPERAND_MEMORY";
  let MCOperandPredicate = [{
    int64_t Imm;
    if (MCOp.evaluateAsConstantImm(Imm))
      return isInt<16>(Imm);
    return MCOp.isBareSymbolRef();
  }];
}

class InstZ80<dag outs, dag ins,
string opcodestr, string argstr, list<dag> pattern> : Instruction
{
  field bits<8> Inst;

  let Namespace = "Z80";
  dag OutOperandList = outs;
  dag InOperandList = ins;
  let AsmString = opcodestr # "\t" # argstr;
  let Pattern = pattern;

  // SoftFail is a field the disassembler can use to provide a way for
  // instructions to not match without killing the whole decode process. It is
  // mainly used for ARM, but Tablegen expects this field to exist or it fails
  // to build the decode table.
  field bits<8> SoftFail = 0;
}

class PseudoZ80<dag outs, dag ins, list<dag> pattern> : InstZ80<outs, ins, "", "", pattern> {
    let isCodeGenOnly = 1;
    let isPseudo = 1;
}

multiclass Z80ALUInst<string opcodestr, SDNode OpNode> {
    def "_A_r" : InstZ80<(outs AccReg:$dr), (ins AccReg:$sr, GR8:$r), opcodestr, "$dr, $r", [(set i8:$dr, (OpNode i8:$sr, i8:$r))]>;
    def "_A_i" : InstZ80<(outs AccReg:$dr), (ins AccReg:$sr, imm8:$imm), opcodestr, "$dr, $imm", [(set i8:$dr, (OpNode i8:$sr, i8:$imm))]>;
    let mayLoad = 1 in
    def "_A_HL" : InstZ80<(outs AccReg:$dr), (ins AccReg:$sr, HLReg:$hl), opcodestr, "$dr, (${hl})", [(set i8:$dr, (OpNode i8:$sr, (i8 (load i16:$hl))))]>;
}
