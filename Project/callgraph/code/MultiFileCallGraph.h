////////////////////////////////////////////////////////////
/// COPYRIGHT NOTICE \n
/// Copyright (c) 2013, 中国科学技术大学(合肥) \n
/// All rights reserved. \n
///
/// @file MultiFileCallGraph.h
///
/// @version 0.1
/// @author xtxwy
///
/// 版本修订: \n
///     无
//////////////////////////////////////////////////////////////
#ifndef _MULTIFILECALLGRAPH_H_
#define _MULTIFILECALLGRAPH_H_

#include "BackendInfoDB.h"
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>

using namespace llvm;

struct MultiFileCallGraph : public ModulePass {
    static char ID;
    std::string PassName;
    MultiFileCallGraph() : ModulePass(ID) {
        PassName = "Collect Call Site P_IA_CALLSITE";
    }

    virtual bool runOnModule(Module &M);
    virtual const char *getPassName() const;
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
};

typedef std::set<std::string> RelatedModuleList;
class CallGraphUtil {
public:
    CallGraphUtil() { RML.clear(); }
    ~CallGraphUtil() {}
    //函数信息收集
    bool collectCallSiteInfo(Module *M);
    bool getCallRef(CallInstList *CIL); //得到引用计数
    std::string getFunctionName(Function *F);
    std::string getRealFunctionName(Function *F);
    std::string getRealFunctionName(std::string FuncName);
    std::string getCallInstName(CallInst *I);
    Function *getFunctionofName(std::string Name);
    Module *getModuleofFunction(Function *F);
    Instruction *getFirstDbgInst(Function *F);
    //DOT接口
    bool generateDotGraph(Module *RootModule, std::string RootFunctionName);
    bool writeHead(raw_ostream &O, std::string Root);
    bool writeTail(raw_ostream &O);
    //TODO:为节点加上属性
    bool writeNode(raw_ostream &O, Function *F, CallInstList *CIL, FunctionPointerCall *Fptr);
    //TODO:为边加上属性
    bool writeEdgeAttribute(raw_ostream &O);
    //处理函数指针
    bool processFPtr(Module *RootModule, std::string RootFunctionName);
    bool getLinkModule(Function *F, CallInstList *CI);
    bool linkAllModule();
    bool analyzeSigleModule();
    Module* getLinkedModule();
    bool getFPAResult(CallInst *CI, std::vector<std::string> &MayPoint, FunctionPointerCall *Fptr);
private:
    Module *LinkedModule;
    RelatedModuleList RML;
};

#endif
