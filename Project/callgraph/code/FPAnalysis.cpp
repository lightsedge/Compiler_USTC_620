////////////////////////////////////////////////////////////
/// COPYRIGHT NOTICE \n
/// Copyright (c) 2013, 中国科学技术大学(合肥) \n
/// All rights reserved. \n
///
/// @file FPAnalysis.cpp
///
/// @version 0.1
/// @author xtxwy
///
/// 版本修订: \n
///     无
//////////////////////////////////////////////////////////////

#include "FPAnalysis.h"
#include "llvm/Support/raw_ostream.h"
#include "dsa/DataStructure.h"
#include "dsa/DSGraph.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/CallSite.h"

int DebugOutput_FPAnalysis = 0;
int ErrorOutput_FPAnalysis = 0;

char FPAnalysis::ID = 0;
bool FPAnalysis::runOnModule(Module &M) {
    const DataStructures &DSA = getAnalysis<EQTDDataStructures>();
    const DSCallGraph &CallGraph = DSA.getCallGraph();
    for(Module::iterator sM = M.begin(), eM = M.end(); sM != eM; ++sM) {
        Function *F = &*sM;
        if(F->empty()) continue;
        for(inst_iterator sI = inst_begin(*F), eI = inst_end(*F); sI != eI; ++sI) {
            Instruction *I = &*sI;
            if(CallInst *CI = dyn_cast<CallInst>(I)) {
                if(dyn_cast<IntrinsicInst>(CI) || dyn_cast<DbgDeclareInst>(CI)) continue;
                if(!CI->getCalledFunction()) {
                    Value *V = CI->getCalledValue();
                    if(NULL == V) continue;
                    if(dyn_cast<ConstantExpr>(V)) continue;

                    for (DSCallGraph::callee_iterator sCE = CallGraph.callee_begin(CI), \
                                   eCE = CallGraph.callee_end(CI); sCE != eCE; ++sCE) {
                        std::string name = (*sCE)->getName();
                        std::string instLL = "";
                        raw_string_ostream inst(instLL);
                        CI->print(inst);
                        size_t pos = instLL.rfind("!dbg");
                        if(pos != std::string::npos) instLL.erase(pos, instLL.size() - pos);
                        if(false == BIDB.insertFptrToList(instLL, name)) {
                            if(ErrorOutput_FPAnalysis) llvm::errs() << "后端数据库写入失败.\n";
                            continue;
                        }
                    }//end of for
                }
            }
        }//end of for
    }//end of for

    return true;
}

const char* FPAnalysis::getPassName() const {
    return PassName.c_str();
}

void FPAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<EQTDDataStructures>();
}
