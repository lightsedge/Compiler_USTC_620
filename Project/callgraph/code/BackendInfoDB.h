////////////////////////////////////////////////////////////
/// COPYRIGHT NOTICE \n
/// Copyright (c) 2013, 中国科学技术大学(合肥) \n
/// All rights reserved. \n
///
/// @file BackendInfoDB.h
///
/// @version 0.1
/// @author xtxwy
///
/// 版本修订: \n
///     无
//////////////////////////////////////////////////////////////

#ifndef _BACKENDINFODB_H_
#define _BACKENDINFODB_H_

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

//为了实现跨函数的调用图分析，需要维护3个数据结构:
//1.文件<->Module关系(前端信息数据库已维护)
//2.Module<->Function关系
//3.Function<->CallSite关系
typedef DenseMap<Module*, SetVector<Function*> > FunctionList;
typedef DenseMap<Function*, SetVector<CallInst*> > CallInstList;
typedef std::map<Function*, std::map<std::string, int> > CallInstRef;
//第一个bool表示visiting:用于消除无限递归
//第二个表示visited:用于去除重复访问
typedef DenseMap<Function*, std::pair<bool, bool> > FunctionStack;
//保存函数指针
typedef std::map<std::string, std::vector<std::string> > FunctionPointerCall;
typedef std::vector<std::string> LLList;

class BackendCollectedInfoDB {
public:
    BackendCollectedInfoDB() { 
        FList.clear();
        CIList.clear();
        CIR.clear();
        FS.clear();
        llList.clear();
    }
    ~BackendCollectedInfoDB() {}
    bool insertFunctionToList(Module *M, Function *F);
    bool insertCallInstToList(Function *F, CallInst *CI);
    bool insertCallInstRef(Function *F, std::string callName, int count);
    bool insertFptrToList(std::string instLL, std::string TargetFunction);
    bool insertllToLL(std::string Name);
    FunctionList *getFunctionList();
    CallInstList *getCallInstList();
    CallInstRef *getCallInstRef();
    SetVector<CallInst*> *getCallInstofFunction(Function *F);
    FunctionPointerCall *getFptr();
    LLList *getLLList();
    int getCountofCallInst(Function *F, std::string callName);
    bool insertFunctionToStack(Function *F);
    bool isFunctionVisiting(Function *F);
    bool isFunctionVisited(Function *F);
    bool setFunctionVisiting(Function *F, bool status);
    bool setFunctionVisited(Function *F);
    bool clearFunctionStack();
private:
    FunctionList FList;
    CallInstList CIList;
    CallInstRef  CIR;
    FunctionStack FS;
    FunctionPointerCall Fptr;
    LLList llList;
};

//TODO:除了全局变量，还有更好的方法吗？
extern BackendCollectedInfoDB BIDB;

#endif
