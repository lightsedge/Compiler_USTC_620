////////////////////////////////////////////////////////////
/// COPYRIGHT NOTICE \n
/// Copyright (c) 2013, 中国科学技术大学(合肥) \n
/// All rights reserved. \n
///
/// @file BackendInfoDB.cpp
///
/// @version 0.1
/// @author xtxwy
///
/// 版本修订: \n
///     无
//////////////////////////////////////////////////////////////
#include "BackendInfoDB.h"

int DebugOutput_BackendInfoDB = 0;
int ErrorOutput_BackendInfoDB = 0;

bool BackendCollectedInfoDB::insertFunctionToList(Module *M, Function *F) {
    if(NULL == M || NULL == F) return false;
    FList[M].insert(F);
    return true;
}

bool BackendCollectedInfoDB::insertCallInstToList(Function *F, CallInst *CI) {
    if(F == NULL) return false;
    CIList[F].insert(CI);
    return true;
}

bool BackendCollectedInfoDB::insertCallInstRef(Function *F, std::string CallName, int count) {
    if(CallName.empty()) return false;
    CIR[F][CallName] = count;
    return true;
}

FunctionList* BackendCollectedInfoDB::getFunctionList() {
    return &FList;
}

CallInstList* BackendCollectedInfoDB::getCallInstList() {
    return &CIList;
}

CallInstRef* BackendCollectedInfoDB::getCallInstRef() {
    return &CIR;
}

SetVector<CallInst*> *BackendCollectedInfoDB::getCallInstofFunction(Function *F) {
    if(NULL == F) return NULL;
    CallInstList::iterator sCIL = CIList.find(F);
    if(sCIL == CIList.end()) return NULL;
    else return &(sCIL->second);
}

int BackendCollectedInfoDB::getCountofCallInst(Function *F, std::string callName) {
    if(F == NULL) return 0;
    CallInstRef::iterator sI = CIR.find(F);
    if(sI == CIR.end()) return 0;

    if(callName.empty()) return 0;
    std::map<std::string, int>::iterator sCI = sI->second.find(callName);
    if(sCI == sI->second.end()) return 0;
    else return sCI->second;
}

bool BackendCollectedInfoDB::insertFunctionToStack(Function *F) {
    if(NULL == F) return false;
    FunctionStack::iterator sFS = FS.find(F);
    if(sFS == FS.end()) {
        FS[F].first = false;
        FS[F].second = false;
    }

    return true;
}

bool BackendCollectedInfoDB::isFunctionVisiting(Function *F) {
    if(NULL == F) return false;
    FunctionStack::iterator sFS = FS.find(F);
    if(sFS == FS.end()) return false;
    else return sFS->second.first;
}

bool BackendCollectedInfoDB::isFunctionVisited(Function *F) {
    if(NULL == F) return false;
    FunctionStack::iterator sFS = FS.find(F);
    if(sFS == FS.end()) return false;
    else return sFS->second.second;
}

bool BackendCollectedInfoDB::setFunctionVisiting(Function *F, bool status) {
    if(NULL == F) return false;
    FunctionStack::iterator sFS = FS.find(F);
    if(sFS == FS.end()) {
        FS[F].first = true;
        FS[F].second = false;
    } else sFS->second.first = status;
    return true;
}

bool BackendCollectedInfoDB::setFunctionVisited(Function *F) {
    if(NULL == F) return false;
    FunctionStack::iterator sFS = FS.find(F);
    if(sFS == FS.end()) {
        FS[F].first = false;
        FS[F].second = true;
    } else sFS->second.second = true;
    return true;
}

bool BackendCollectedInfoDB::insertFptrToList(std::string instLL, std::string TargetFunction) {
    if(instLL.empty() || TargetFunction.empty()) return false;
    Fptr[instLL].push_back(TargetFunction);
    return true;
}

FunctionPointerCall* BackendCollectedInfoDB::getFptr() {
    return &Fptr;
}

bool BackendCollectedInfoDB::clearFunctionStack() {
    FS.clear();
    return true;
}

bool BackendCollectedInfoDB::insertllToLL(std::string Name) {
    if(Name.empty()) return false;
    llList.push_back(Name);
    return true;
}

LLList* BackendCollectedInfoDB::getLLList() {
    return &llList;
}
