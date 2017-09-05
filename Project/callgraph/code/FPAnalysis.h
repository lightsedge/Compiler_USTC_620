////////////////////////////////////////////////////////////
/// COPYRIGHT NOTICE \n
/// Copyright (c) 2013, 中国科学技术大学(合肥) \n
/// All rights reserved. \n
///
/// @file FPAnalysis.h
///
/// @version 0.1
/// @author xtxwy
///
/// 版本修订: \n
///     无
//////////////////////////////////////////////////////////////
#ifndef _FPANALYSIS_H_
#define _FPANALYSIS_H_

#include "BackendInfoDB.h"
#include <llvm/Pass.h>

using namespace llvm;

struct FPAnalysis : public ModulePass {
    static char ID;
    std::string PassName;
    FPAnalysis() : ModulePass(ID) { PassName = "Function Pointer Analysis"; }
    
    virtual bool runOnModule(Module &M);
    virtual const char *getPassName() const;
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
};

#endif
