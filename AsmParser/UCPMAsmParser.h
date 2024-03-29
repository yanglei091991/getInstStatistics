//===-- UCPMAsmParser.h - Parse UCPM asm to MCInst instructions -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#ifndef UCPMASMPARSER_H
#define UCPMASMPARSER_H

//#include "../UCPMInsnFlags.h"
#include "../MCTargetDesc/UCPMInsnLine.h"
#include "../MCTargetDesc/UCPMMCTargetDesc.h"

#include "llvm/MC/MCParser/AsmLexer.h"
#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCParser/MCAsmParserExtension.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"

#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDwarf.h"

#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"

//NUMSLOTS is the same as UCPMInsnLine.h
//ducx #define  NUMSLOTS  18
//ducx #include <map>

namespace llvm {
namespace UCPM {

//ducx //yangl, for the 17 NOPs
//ducx extern MCInst *NOPInst[NUMSLOTS];
//ducx extern int LineNumber;//yangl
//ducx extern std::map<int, int> PC_LineNo;//yangl
//ducx extern std::string InputSrcFilename;//yangl
//ducx extern std::string InputSrcContext;//yangl
//ducx extern bool slots_occupation_table[17 + 1];//yangl
//ducx   
//ducx //yangl: imac or ifmac
//ducx extern bool is_ifmac;  
//ducx extern bool is_ifalu;  //ducx
//ducx extern bool is_imac;  //ducx
//ducx extern bool is_ialu;  //ducx

class MCFunction;
enum AsmOperandKind {
  AsmHMacro, // slot is a special kind of helper register class
  AsmOpcode,
  AsmRegister,
  AsmImmediate,
  AsmExpression,
  AsmSlot
};

struct UCPMAsmOperand: public MCParsedAsmOperand {
  enum AsmOperandKind Kind;
  SMLoc StartLoc, EndLoc;

  union {
    std::string *Name; // the HMacro name
    unsigned Opc;  // the opcode.
    unsigned Reg;
    int64_t Imm;
    const MCExpr *Expr;
  };

public:
  UCPMAsmOperand(const UCPMAsmOperand &o)
    : MCParsedAsmOperand() {
    Kind = o.Kind;
    StartLoc = o.StartLoc;
    EndLoc = o.EndLoc;
    switch(Kind) {
    case AsmHMacro:
      Name = o.Name;
      break;
    case AsmRegister:
      Reg = o.Reg;
      break;
    case AsmImmediate:
      Imm = o.Imm;
      break;
    case AsmExpression:
      Expr = o.Expr;
      break;
    case AsmOpcode:
      Opc = o.Opc;
      break;
    case AsmSlot:
      Imm = o.Imm;
      break;
    }
  }

  UCPMAsmOperand(AsmOperandKind K)
        : MCParsedAsmOperand(), Kind(K) {}

  /// getStartLoc - Get the location of the first token of this operand.
  SMLoc getStartLoc() const {
    return StartLoc;
  }

  /// getEndLoc - Get the location of the last token of this operand.
  SMLoc getEndLoc() const {
    return EndLoc;
  }

  unsigned getKind() const {
    return Kind;
  }

  std::string *getHMacro() const {
    assert(Kind == AsmHMacro && "Invalid access!");
    return Name;
  }

  unsigned getReg() const {
    assert(Kind == AsmRegister && "Invalid access!");
    return Reg;
  }

  unsigned getOpc() const {
    assert(Kind == AsmOpcode && "Invalid access!");
    return Opc;
  }

  int64_t getImm() const {
    assert(Kind == AsmImmediate && "Invalid access!");
    return Imm;
  }

  int64_t getSlot() const {
    assert(Kind == AsmSlot && "Invalid access!");
    return Imm;
  }

  const MCExpr * getExpr() const {
    assert(Kind == AsmExpression && "Invalid access!");
    return Expr;
  }

  StringRef getSymName() const {
    assert(Kind == AsmExpression && "Invalid access!");
    if (!dyn_cast<MCSymbolRefExpr>(Expr)) return StringRef();
    else return dyn_cast<MCSymbolRefExpr>(Expr)->getSymbol().getName();
  }

  bool isHMacro() const {
    return Kind == AsmHMacro;
  }

  bool isToken() const {
    return Kind == AsmOpcode;
  }

  bool isImm() const {
    return Kind == AsmImmediate;
  }

  bool isSlot() const {
    return Kind == AsmSlot;
  }

  bool isReg() const {
    return Kind == AsmRegister;
  }

  bool isMem() const {
    return false;
  }

  bool isExpr() const {
    return Kind == AsmExpression;
  }

  /// print - Print a debug representation of the operand to the given stream.
  void print(raw_ostream &OS) const {
    OS << "<";
    switch(Kind) {
      case AsmImmediate:
        OS << "Imm:";
        break;
      case AsmOpcode:
        OS << "Inst:";
        break;
      case AsmRegister:
        OS << "Operand:";
        break;
      case AsmHMacro:
        OS << "HMacro:";
        break;
      case AsmExpression:
        OS << "Expr:";
        break;
      case AsmSlot:
        OS << "Slot:" << Imm << ">";
        return;
    }
    if (StartLoc.isValid() && EndLoc.isValid() &&
        StartLoc.getPointer() <= EndLoc.getPointer())
      OS << StringRef(StartLoc.getPointer(),
                      EndLoc.getPointer() - StartLoc.getPointer() + 1);
    else OS << "N/A";
    OS << ">";
  }

  static UCPMAsmOperand *createHMacro(std::string *Name,
                                          SMLoc S = SMLoc(), SMLoc E = SMLoc()) {
    UCPMAsmOperand *Op = new UCPMAsmOperand(AsmHMacro);
    Op->Name = Name;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

static UCPMAsmOperand *createOpc(unsigned opcode,
                                     SMLoc S = SMLoc(), SMLoc E = SMLoc()) {
    UCPMAsmOperand *Op = new UCPMAsmOperand(AsmOpcode);
    Op->Opc = opcode;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static UCPMAsmOperand *createReg(unsigned RegNum,
                                       SMLoc S = SMLoc(), SMLoc E = SMLoc()) {
    UCPMAsmOperand *Op = new UCPMAsmOperand(AsmRegister);
    Op->Reg = RegNum;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static UCPMAsmOperand *createImm(int64_t imm,
                                       SMLoc S = SMLoc(), SMLoc E = SMLoc()) {
    UCPMAsmOperand *Op = new UCPMAsmOperand(AsmImmediate);
    Op->Imm = imm;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static UCPMAsmOperand *createSlot(int64_t imm,
                                        SMLoc S = SMLoc(), SMLoc E = SMLoc()) {
    UCPMAsmOperand *Op = new UCPMAsmOperand(AsmSlot);
    Op->Imm = imm;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static UCPMAsmOperand *createExpr(const MCExpr *expr,
                                        SMLoc S = SMLoc(), SMLoc E = SMLoc()) {
    UCPMAsmOperand *Op = new UCPMAsmOperand(AsmExpression);
    Op->Expr = expr;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }
};
typedef std::shared_ptr<UCPM::UCPMAsmOperand> SharedMMPUOprd;
#define OPERAND(TY, VALUE, START, END) \
UCPM::UCPMAsmOperand::create##TY(VALUE, START, END)
#define SHARED_OPRD(TY, VALUE, START, END) \
SharedMMPUOprd(OPERAND(TY, VALUE, START, END))
#define ADDOPERAND(TY, VALUE, START, END) \
Operands.push_back(std::unique_ptr<UCPM::UCPMAsmOperand>(std::unique_ptr<UCPM::UCPMAsmOperand>(OPERAND(TY, VALUE, START, END))))

  
struct HMacro {
  StringRef Name;
  MCFunction *Body;
    
public:
  HMacro(StringRef N, MCFunction *B)
    : Name(N), Body(B) {}
};

/* yangl */
void printInst(MCInst& Inst);
void printMCOperand(MCOperand& mcoperand);
void printUCPMAsmOperand(UCPMAsmOperand& ucpmoperand);

/* search pattern */
int NaiveSearch(std::vector<std::pair<int,int>> &records, int recordlen,
                std::vector<std::pair<int,int>> &pattern, int patlen, int& matchpos);
std::pair<int, int> generateOperandPair(MCOperand &mcoperand);
int matchLoop(std::vector<std::pair<int,int>> &pattern, int patlen, int startpos);
int matchRepeatImm(std::vector<std::pair<int,int>> &pattern, int patlen, int startpos);
/* to delete \r \t \n " " ";" in the source code text*/
void getPureInstText(std::string &input);
//ducx std::string getStringbyLine(const std::string &InputStr, int LineNo);
std::string transHMName(const std::string & str);
bool isLetter(char l);
std::string getSubSrcString(const std::string& sourcestr, int id);//the the id'th sub source string in string like XXX || XXX || ...

//ducx
extern std::map<unsigned, SmallVector<SharedMMPUOprd, 8> > MIidToMMPU;
} // namespace UCPM
} // namespace llvm

#endif
