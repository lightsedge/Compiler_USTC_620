////////////////////////////////////////////////////////////
/// COPYRIGHT NOTICE \n
/// Copyright (c) 2013, 中国科学技术大学(合肥) \n
/// All rights reserved. \n
///
/// @file MultiFileCallGraph.cpp
///
/// @version 0.1
/// @author xtxwy
///
/// 版本修订: \n
///     无
//////////////////////////////////////////////////////////////
#include "FrontendInfoDB.h"
#include "MultiFileCallGraph.h"
#include "FPAnalysis.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/DebugLoc.h"
#include "llvm/Linker.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/PassManager.h"
#include "llvm/IR/DataLayout.h"
#include <cxxabi.h>

int DebugOutput_MultiFileCallGraph = 0;
int ErrorOutput_MultiFileCallGraph = 0;

char MultiFileCallGraph::ID = 0;
bool MultiFileCallGraph::runOnModule(Module &M) {
    CallGraphUtil CGU;
    CGU.collectCallSiteInfo(&M);
    return true;
}

const char* MultiFileCallGraph::getPassName() const {
    return PassName.c_str();
}

void MultiFileCallGraph::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
}

bool CallGraphUtil::collectCallSiteInfo(Module *M) {
    if(NULL == M) return false;
    for(Module::iterator sM = M->begin(), eM = M->end(); sM != eM; ++sM) {
        Function *F = sM;
        if(DebugOutput_MultiFileCallGraph) llvm::outs() << "函数:" << F->getName() << "\n";
        if(Intrinsic::ID ID = (Intrinsic::ID)F->getIntrinsicID()) {
            if(ID == Intrinsic::dbg_declare) continue;
        }

        bool result = BIDB.insertFunctionToList(M, F);
        if(false == result && ErrorOutput_MultiFileCallGraph) llvm::errs() << "写入后端信息数据库失败.\n";

        bool callIsExist = false;
        bool hasInstruction = false;
        for(inst_iterator sI = inst_begin(*F), eI = inst_end(*F); sI != eI; ++sI) {
            hasInstruction = true; //如果这个为false，表示是一个声明或者外部函数
            Instruction *I = &*sI;
            if(CallInst *CI = dyn_cast<CallInst>(I)) {
                //TODO：单纯的过滤掉IntrinsicInst正确吗？
                if(dyn_cast<IntrinsicInst>(CI) || dyn_cast<DbgDeclareInst>(CI)) continue;
                callIsExist = true;
                if(DebugOutput_MultiFileCallGraph) llvm::outs() << "Call指令:" << *CI << "\n";
                bool result1 = BIDB.insertCallInstToList(F, CI);
                if(false == result1 && ErrorOutput_MultiFileCallGraph) llvm::errs() << "写入后端信息数据库失败.\n";
            }
        }//end of for

        if(false == callIsExist && true == hasInstruction) {
            bool result1 = BIDB.insertCallInstToList(F, NULL);
            if(false == result1 && ErrorOutput_MultiFileCallGraph) llvm::errs() << "写入后端信息数据库失败.\n";
        }
    }//end of for

    return true;
}

bool CallGraphUtil::getCallRef(CallInstList *CIL) {
    if(NULL == CIL) return false;

    std::map<std::string, bool> tempVec;
    for(CallInstList::iterator sCIL = CIL->begin(), eCIL = CIL->end(); sCIL != eCIL; ++sCIL) {
        Function *F = sCIL->first;
        int CallInstCount = 0;
        tempVec.clear();
        for(SetVector<CallInst*>::iterator sSV = sCIL->second.begin(), eSV = sCIL->second.end(); sSV != eSV; ++sSV) {
            CallInst* A = *sSV;
            if(NULL == A) break;
            //这个值已经处理过了，就直接跳过
            std::map<std::string, bool>::iterator sI = tempVec.find(getCallInstName(A));
            if(sI != tempVec.end()) continue;

            tempVec[getCallInstName(A)] = true;
            for(SetVector<CallInst*>::iterator ssSV = sCIL->second.begin(), eeSV = sCIL->second.end(); ssSV != eeSV; ++ssSV) {
                CallInst *B = *ssSV;
                if(!getCallInstName(A).compare(getCallInstName(B))) CallInstCount++;
            }

            BIDB.insertCallInstRef(F, getCallInstName(A), CallInstCount);
            CallInstCount = 0;
        }
    }

    return true;
}

std::string CallGraphUtil::getFunctionName(Function *F) {
    return F->getName();
}

std::string CallGraphUtil::getRealFunctionName(Function *F) {
    std::string FuncName = F->getName();
    if(FuncName.empty()) return "";

    const char *name;
    int status = -1;
    name = abi::__cxa_demangle(FuncName.c_str(), NULL, NULL, &status);
    if( name == NULL )    
        name = FuncName.c_str( );

    std::string result( name );
    return result;
}

std::string CallGraphUtil::getRealFunctionName(std::string FuncName)
{
    const char *name;
    int status = -1;
    name = abi::__cxa_demangle(FuncName.c_str(), NULL, NULL, &status);
    if( name == NULL )
        name = FuncName.c_str( );
    
    std::string result( name );
    return result;
}

std::string CallGraphUtil::getCallInstName(CallInst *I) {
    Function *F = I->getCalledFunction();
    if(NULL == F) return "";
    std::string result = getFunctionName(F);
    return result;
}

bool CallGraphUtil::generateDotGraph(Module *RootModule, std::string RootFunctionName) {
    FunctionList *FL = BIDB.getFunctionList();
    CallInstList *CIL = BIDB.getCallInstList();
    FunctionPointerCall *Fptr = BIDB.getFptr();
    if(FL == NULL || CIL == NULL || NULL == Fptr) {
        if(ErrorOutput_MultiFileCallGraph) llvm::errs() << "读取后端数据库信息失败.\n";
        return false;
    }

    CallInstRef *CIR = BIDB.getCallInstRef();
    if(CIR == NULL) return false;
    if(CIR->size() == 0) {
        if(false == getCallRef(CIL)) {
            if(ErrorOutput_MultiFileCallGraph) llvm::errs() << "写入后端数据库信息失败.\n";
            return false;
        }
    }

    if(DebugOutput_MultiFileCallGraph) {
        llvm::outs() << "FunctionList Size:" << FL->size() << "\n";
        llvm::outs() << "CallInstList Size:" << CIL->size() << "\n";
        llvm::outs() << "CallInstRef Size:" << CIR->size() << "\n";
        llvm::outs() << "FunctionPointerCall Size:" << Fptr->size() << "\n";
    }

    Function *rootFunction = NULL;
    for(Module::iterator sM = RootModule->begin(), eM = RootModule->end(); sM != eM; ++sM) {
        Function *F = &*sM;
        if(F->empty()) continue;
        if(!RootFunctionName.compare(getFunctionName(F))) {
            rootFunction = F;
            break;
        }
    }

    if(NULL == rootFunction) {
        if(ErrorOutput_MultiFileCallGraph) llvm::errs() << "传入的入口函数无效.\n";
        return false;
    }

    std::string realRootName = getRealFunctionName( RootFunctionName );
    std::string dotFileName = "";
    dotFileName.append(realRootName);
    dotFileName.append(".dot");
    std::string ErrorInfo;
    raw_fd_ostream File(dotFileName.c_str(), ErrorInfo);
    //1.write文件头
    if(DebugOutput_MultiFileCallGraph) llvm::outs() << "开始写dot文件body.\n";
    if(false == writeHead(File, RootFunctionName)) {
        if(ErrorOutput_MultiFileCallGraph) llvm::errs() << "写dot图头失败.\n";
        return false;
    }
    //2.从RootFunctionName出发，开始写Dot图的body
    BIDB.clearFunctionStack();
    writeNode(File, rootFunction, CIL, Fptr);
    BIDB.clearFunctionStack();
    //3.写文件尾
    if(false == writeTail(File)) {
        if(ErrorOutput_MultiFileCallGraph) llvm::errs() << "写dot图尾失败.\n";
        return false;
    }

    llvm::outs() << "函数调用图生成成功.\n";
    return true;
}

bool CallGraphUtil::writeHead(raw_ostream &O, std::string Root) {
    std::string realRootName = getRealFunctionName( Root );

    O << "digraph {\n";
    O << "\tlabel=\"" << realRootName << "的函数调用图\";\n";
    return true;
}

bool CallGraphUtil::writeTail(raw_ostream &O) {
    O << "}\n";
    return true;
}

Function* CallGraphUtil::getFunctionofName(std::string Name) {
    FunctionList *FL = BIDB.getFunctionList();
    for(FunctionList::iterator sFL = FL->begin(), eFL = FL->end(); sFL != eFL; ++sFL) {
        for(SetVector<Function*>::iterator sSV = sFL->second.begin(), eSV = sFL->second.end(); sSV != eSV; ++sSV) {
            Function *FF = *sSV;
            if(FF->empty()) continue;
            std::string TargetName = getFunctionName(FF);
            if(!Name.compare(TargetName)) return FF;
        }
    }

    return NULL;
}

Module* CallGraphUtil::getModuleofFunction(Function *F) {
    if(NULL == F) return NULL;
    FunctionList *FL = BIDB.getFunctionList();
    for(FunctionList::iterator sFL = FL->begin(), eFL = FL->end(); sFL != eFL; ++sFL) {
        Module *M = sFL->first;
        int Exsit = sFL->second.count(F);
        if(0 == Exsit) continue;
        else return M;
    }

    return NULL;
}

Instruction* CallGraphUtil::getFirstDbgInst(Function *F) {
    BasicBlock::iterator start = F->getEntryBlock().begin(), end = F->getEntryBlock().end();
    while(start != end && !isa<DbgInfoIntrinsic>(start)) ++start;
    while(start != end && (isa<DbgInfoIntrinsic>(start) || isa<StoreInst>(start))) ++start;
    if(start == end) return NULL;
    else return &*start;
}

bool CallGraphUtil::writeNode(raw_ostream &O, Function *F, CallInstList *CIL, FunctionPointerCall *Fptr) {
    if(NULL == F || NULL == CIL) return false; // =>1<=

    //判断函数是否已被处理过
    if(BIDB.isFunctionVisited(F)) return true;

    //设置函数正在被访问
    BIDB.setFunctionVisiting(F, true);
    //设置节点属性
    O << "\t" << getFunctionName(F) << "[label=\"" << getRealFunctionName(F);
    llvm::Module *M = getModuleofFunction(F);
    if(NULL == M) {
        M = F->getParent();
        std::string FileName = CIDB.getFileofModule(M);
        if(FileName.empty()) O << "\"];\n";
        else O << "\\n在文件" << FileName << "中调用,定义点未知\"];\n";
    } else {
        std::string FileName = CIDB.getFileofModule(M);
        if(FileName.empty()) O << "\"];\n";
        else {
            if(!F->empty()) {
                O << "\\n在文件" << FileName << "中";
                Instruction *I = getFirstDbgInst(F);
                if(NULL != I) {
                    DebugLoc Loc = I->getDebugLoc();
                    if(!Loc.isUnknown()) 
                        O << "第" << Loc.getLine() << "行左右定义\"];\n";
                    else
                        O << "定义\"];\n";
                } else O << "定义\"];\n";
            } else { //外部函数
                M = F->getParent();
                std::string FileName1 = CIDB.getFileofModule(M);
                if(FileName1.empty()) O << "\"];\n";
                else O << "\\n在文件" << FileName1 << "中调用,定义点未知\"];\n";
            }
        }
    }

    CallInstList::iterator sCIL = CIL->find(F);
    if(sCIL == CIL->end()) return false;

    //llvm::outs() << F->getName() << "\n";
    SetVector<CallInst*>::iterator sSV = sCIL->second.begin();
    if(*sSV != NULL) {
        std::map<std::string, bool> tempD;
        tempD.clear();
        for(SetVector<CallInst*>::iterator eSV = sCIL->second.end(); sSV != eSV; ++sSV) {
            CallInst *CI = *sSV;

            //判断这条指令是否已被访问过
            std::map<std::string, bool>::iterator sCI = tempD.find(getCallInstName(CI));
            if(sCI != tempD.end()) continue;
            else tempD[getCallInstName(CI)] = true;

            //递归生成节点
            bool recur = false;
            std::string RootFunctionName = getCallInstName(CI);
            Function *rootFunction = getFunctionofName(RootFunctionName);
            if(BIDB.isFunctionVisiting(rootFunction)) {
                if(DebugOutput_MultiFileCallGraph) llvm::outs() << "检测到递归.\n";
                recur = true;
            }
   
            if(DebugOutput_MultiFileCallGraph) llvm::outs() << "要访问的函数:" << RootFunctionName << "\n";

            if(NULL != rootFunction && recur == false) writeNode(O, rootFunction, CIL, Fptr);
            if(NULL == rootFunction && recur == false) {
                //TODO:外部函数
            }

            if(getCallInstName(CI).empty()) {
                if(DebugOutput_MultiFileCallGraph) {
                    CI->print(llvm::outs());
                    std::string instLL = "";
                    raw_string_ostream inst(instLL);
                    CI->print(inst);
                    size_t pos = instLL.rfind("!dbg");
                    if(pos != std::string::npos) instLL.erase(pos, instLL.size() - pos);
                    FunctionPointerCall::iterator sS = Fptr->find(instLL);
                    if(sS != Fptr->end()) llvm::outs() << "\n\t有戏...\n";
                    else llvm::outs() << "\n\t悲剧了...\n";
                }

                //开始函数指针处理
                std::vector<std::string> MayPoint;
                if(true == getFPAResult(CI, MayPoint, Fptr)) {
                    if(MayPoint.size() > 1) {
                        for(std::vector<std::string>::iterator sV = MayPoint.begin(), eV = MayPoint.end(); sV != eV; ++sV) {
                            //开始递归分析
                            rootFunction = getFunctionofName(*sV);
                            if(BIDB.isFunctionVisiting(rootFunction)) recur = true;
                            if(NULL != rootFunction && recur == false) writeNode(O, rootFunction, CIL, Fptr);

                            O << "\t" << *sV << "[color=red];\n";
                            O << "\t" << getFunctionName(F);
                            O << " -> ";
                            O << *sV << "[";
                            if(true == recur) O << "label=\"递归环\",";
                            O << "color=red";
                            if(true != recur) O << ",style=dashed];\n";
                            else {
                                O << "];\n";
                                recur = false;
                            }
                        }
                    } else {
                        //开始递归分析
                        rootFunction = getFunctionofName(*(MayPoint.begin()));
                        if(BIDB.isFunctionVisiting(rootFunction)) recur = true;
                        if(NULL != rootFunction && recur == false) writeNode(O, rootFunction, CIL, Fptr);

                        O << "\t" << *(MayPoint.begin()) << "[color=blue];\n";
                        O << "\t" << getFunctionName(F);
                        O << " -> ";
                        O << *(MayPoint.begin());
                        if(recur == true) {
                            O << "[label=\"递归环\",color=red];";
                            recur = false;
                        } else O << "[color=blue];\n";
                    }
                }  else { //如果函数指针分析失败,用一个统一的节点"FP"表示
                    O << "\tFP_unkown" << "[style=filled,color=red];\n";
                    O << "\t" << getFunctionName(F);
                    O << " -> ";
                    O << "FP_unkown [color=blue];\n";
                }
            } else { //处理正常的指令
                O << "\t" << getFunctionName(F);
                O << " -> ";
                O << getCallInstName(CI);
                int count = BIDB.getCountofCallInst(F, getCallInstName(CI));
                if(recur == true) O << "[label=\"递归环\",color=red];\n";
                else if(0 == count || 1 == count) O << ";" << "\n";
                else O << "[label=\"" << count << "\"];\n";
            }
        }//end of for
    }//end of if

    //设置函数已经被访问过了和访问完了
    BIDB.setFunctionVisited(F);
    BIDB.setFunctionVisiting(F, false);
    return true;
}

bool CallGraphUtil::getFPAResult(CallInst *CI, std::vector<std::string> &MayPoint, FunctionPointerCall *Fptr) {
    if(CI == NULL) return false;
    Value *V = CI->getCalledValue();
    if(NULL == V) return false;
    if(ConstantExpr *CE = dyn_cast<ConstantExpr>(V)) {
        std::string OpName = CE->getOpcodeName();
        if(OpName.compare("bitcast")) return false;
        for(User::const_op_iterator sU = CE->op_begin(), eU = CE->op_end(); sU != eU; ++sU) {
            Value *V = *sU;
            if(V->hasName()) MayPoint.push_back(V->getName());
        }
        return true;
    } else {
        std::string instLL = "";
        raw_string_ostream inst(instLL);
        CI->print(inst);
        size_t pos = instLL.rfind("!dbg");
        if(pos != std::string::npos) instLL.erase(pos, instLL.size() - pos);
        FunctionPointerCall::iterator sS = Fptr->find(instLL);
        if(sS != Fptr->end()) {
            std::vector<std::string> sSS = sS->second;
            MayPoint.insert(MayPoint.begin(), sSS.begin(), sSS.end());
            return true;
        } else return false;
    }

    return false;
}

bool CallGraphUtil::processFPtr(Module *RootModule, std::string RootFunctionName) {
    CallInstList *CIL = BIDB.getCallInstList();
    Function *rootFunction = NULL;
    for(Module::iterator sM = RootModule->begin(), eM = RootModule->end(); sM!= eM; ++sM) {
        Function *F = &*sM;
        if(F->empty()) continue;
        if(!RootFunctionName.compare(getFunctionName(F))) {
            rootFunction = F;
            break;
        }
    }

    if(!CIL || !rootFunction) return false;
    BIDB.clearFunctionStack();
    if(false == getLinkModule(rootFunction, CIL)) {
        llvm::outs() << "模块链接分析失败.\n";
        return false;
    }
    BIDB.clearFunctionStack();
    if(false == linkAllModule()) {
        llvm::errs() << "模块链接失败.\n";
        return false;
    }

    if(false == analyzeSigleModule()) {
        llvm::errs() << "函数指针分析失败.\n";
        return false;
    }
  
    return true;  
}

bool CallGraphUtil::getLinkModule(Function *F, CallInstList *CIL) {
    if(NULL == F || NULL == CIL) return false;
    if(BIDB.isFunctionVisited(F)) return true;
    BIDB.setFunctionVisiting(F, true);
    llvm::Module *M = getModuleofFunction(F);
    if(NULL != M) {
        std::string file = M->getModuleIdentifier();
        if(0 == RML.count(file)) {
            RML.insert(file);
        }
    }
    CallInstList::iterator sCIL = CIL->find(F);
    if(sCIL == CIL->end()) return false;
    SetVector<CallInst*>::iterator sSV = sCIL->second.begin();
    if(*sSV != NULL) {
        std::map<std::string, bool> tempD;
        tempD.clear();
        for(SetVector<CallInst*>::iterator eSV = sCIL->second.end(); sSV != eSV; ++sSV) {
            CallInst *CI = *sSV;
            std::map<std::string, bool>::iterator sCI = tempD.find(getCallInstName(CI));
            if(sCI != tempD.end()) continue;
            else tempD[getCallInstName(CI)] = true;
            bool recur = false;
            std::string RootFunctionName = getCallInstName(CI);
            Function *rootFunction = getFunctionofName(RootFunctionName);
            if(BIDB.isFunctionVisiting(rootFunction)) recur = true;
            if(NULL != rootFunction && recur == false) getLinkModule(rootFunction, CIL);
        }
    }
    BIDB.setFunctionVisited(F);
    BIDB.setFunctionVisiting(F, false);
  
    return true;
}

static std::string getLLFileName(std::string FileName) {
    std::string llName = "";
    llName += "/tmp/";
    size_t pos = FileName.rfind("/");
    if(pos != std::string::npos) llName.append(FileName, pos + 1, FileName.size() - pos);
    else llName += FileName;
    llName += ".out";
    return llName;
}

bool CallGraphUtil::linkAllModule() {
    LLVMContext &Context = getGlobalContext();
    RelatedModuleList::iterator sRML = RML.begin();
    std::string startFile = getLLFileName(*sRML); ++sRML;
    SMDiagnostic Errs;
    LinkedModule = ParseIRFile(startFile, Errs, Context);
    if(NULL == LinkedModule) return false;
    Linker L(LinkedModule);

    for(RelatedModuleList::iterator eRML = RML.end(); sRML != eRML; ++sRML) {
        startFile.clear();
        startFile = getLLFileName(*sRML);
        Module *M = ParseIRFile(startFile, Errs, Context);
        if(NULL == M) return false;
        std::string Error = "";
        if(L.linkInModule(M, &Error)) {
        llvm::errs() << "模块链接失败:" << Error << "\n";
            return false;
        }
    }
  
    return true;
}

bool CallGraphUtil::analyzeSigleModule() {
    const std::string &ModuleDataLayout = LinkedModule->getDataLayout();
    DataLayout *TD = new DataLayout(ModuleDataLayout);
    PassManager pm; if (TD) pm.add(TD);
    pm.add(new FPAnalysis());
    pm.run(*LinkedModule);
	return true;
}
