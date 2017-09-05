////////////////////////////////////////////////////////////
/// COPYRIGHT NOTICE \n
/// Copyright (c) 2013, 中国科学技术大学(合肥) \n
/// All rights reserved. \n
///
/// @file driver.cpp
/// @brief 本项目的入口函数，同时也是驱动其他分析的入口点
///
/// 本文件实现了分析前端，可以同时处理多个文件
///
/// @version 0.1
/// @author xtxwy, Peter Chang
///
/// 版本修订: \n
///     无
//////////////////////////////////////////////////////////////
#include "FrontendInfoDB.h"
#include "BackendInfoDB.h"
#include "MultiFileCallGraph.h"
//clang
#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Driver/Compilation.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Tool.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
//llvm
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Linker.h"
//#include "llvm/LLVMContext.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
//other
#include <cstring>
#include <cxxabi.h>

using namespace clang;
using namespace clang::driver;

//这个最好以后参数化
int DebugOutput_main = 0;
int ErrorOutput_main = 0;

FrontendCollectedInfoDB CIDB;
BackendCollectedInfoDB  BIDB;

static bool printErrorFormat() {
    llvm::outs() << "\n======================\n";
    llvm::outs() << "指定文件名和函数名错误.";
    llvm::outs() << "\n======================\n";
    return true;
}

static bool printHelp() {
    llvm::outs() << "LLVM callgraph Tool by xtxwy\n";
	llvm::outs() << "使用方法:callgraph <源文件1 源文件2 ...> [-root-function=文件名:函数名]\n\n";
    llvm::outs() << "-root-function=文件名:函数名        指定分析的入口函数\n";
    llvm::outs() << "-h或者--help                        显示帮助菜单\n";
    return true;
}

static std::string getRealFunctionName(Function *F) {
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

Function *isLegal(llvm::Module *M, std::string FunctionName) {
    for(llvm::Module::iterator sM = M->begin(), eM = M->end(); sM != eM; ++sM) {
        Function *F = &(*sM);
        if(!FunctionName.compare(getRealFunctionName(F))) return F;
    }

    return NULL;
}

int main(int argc, char **argv) {
  	IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts = new DiagnosticOptions();
  	TextDiagnosticPrinter *DiagClient = new TextDiagnosticPrinter(llvm::errs(), &*DiagOpts);
  	IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());
  	DiagnosticsEngine Diags(DiagID, &*DiagOpts, DiagClient);
  	Driver TheDriver("/usr/local/bin/", llvm::sys::getProcessTriple(), "a.out", Diags);
  	TheDriver.setTitle("FrontendDriver by maxs");
  	SmallVector<const char *, 16> Args(argv, argv + argc);
  	Args.push_back("-fsyntax-only");
  	Args.push_back("-g");
  	Args.push_back("-O0");

    std::string RootFunction = "";
    std::string RootFile = "";
    llvm::Module *RootModule = NULL;

  	for(SmallVector<const char*, 16>::iterator s = Args.begin(), e = Args.end(); s != e; ++s) {
		if(0 == strncmp(*s, "-h", 2) || 0 == strncmp(*s, "--help", 6) || 0 == strncmp(*s, "--h", 3)) {
		    printHelp();
		    return 0;
        }

		if(0 == strncmp(*s, "-root-function=", 15)) {
		    char *pfileStart = strstr((char*)*s, "="); pfileStart++;
		    char *pfileEnd = strstr((char*)*s, ":");
		    if(NULL == pfileEnd) {
		        printErrorFormat();
		        printHelp();
		        return 0;
            }

            RootFile.append(pfileStart, pfileEnd - pfileStart);
		    RootFunction.append(++pfileEnd);
			Args.erase(s);
			break;
		}
  	}

    if(RootFunction.empty() || RootFile.empty()) {
        printErrorFormat();
        printHelp();
        return 0;
    }

  	OwningPtr<Compilation> C(TheDriver.BuildCompilation(Args));
  	if (!C) return 0;

  	const driver::JobList &Jobs = C->getJobs();
  	unsigned int ActNum;
  	CodeGenAction **Act;

  	for(JobList::const_iterator s = Jobs.begin(), e = Jobs.end(); s != e; ++s) {
  		const driver::Command *Cmd = cast<driver::Command>(*s);
  		if (llvm::StringRef(Cmd->getCreator().getName()) != "clang") {
    		Diags.Report(diag::err_fe_expected_clang_command);
    		return 1;
  		}

  		const driver::ArgStringList &CCArgs = Cmd->getArguments();
  		OwningPtr<CompilerInvocation> CI(new CompilerInvocation);
  		CompilerInvocation::CreateFromArgs(*CI, const_cast<const char **>(CCArgs.data()), \
  		                                    const_cast<const char **>(CCArgs.data()) + CCArgs.size(), Diags);

  		CompilerInstance Clang;
  		Clang.setInvocation(CI.take());

  		Clang.createDiagnostics();
  		if (!Clang.hasDiagnostics()) return -1;

		ActNum = Clang.getFrontendOpts().Inputs.size() + 1;
		Act = new CodeGenAction*[Clang.getFrontendOpts().Inputs.size() + 1];

  		Clang.setTarget(TargetInfo::CreateTargetInfo(Clang.getDiagnostics(), &(Clang.getTargetOpts())));
  		if(!Clang.hasTarget()) return 0;
  		Clang.getTarget().setForcedLangOptions(Clang.getLangOpts());

  		for(unsigned i = 0, e = Clang.getFrontendOpts().Inputs.size(); i != e; ++i) {
			Act[i]=new EmitLLVMOnlyAction();

			if(Clang.hasSourceManager()) Clang.getSourceManager().clearIDTables();
			if(Act[i]->BeginSourceFile(Clang, Clang.getFrontendOpts().Inputs[i])) {
				Act[i]->Execute();
				Act[i]->EndSourceFile();
			}

			std::string File = Clang.getFrontendOpts().Inputs[i].getFile();
			llvm::Module *M = Act[i]->takeModule();

			std::string Err;
			if(!M || verifyModule(*M, ReturnStatusAction, &Err)) {
				llvm::errs() << "输入文件" << Clang.getFrontendOpts().Inputs[i].getFile() << "分析失败.\n";
			}

            std::string FileOutput;
            FileOutput.assign("/tmp/");
            std::string name = "";
            size_t pos = File.rfind("/");
            if(pos == std::string::npos) name += File;
            else name.append(File, pos + 1, File.size() - pos);
            FileOutput += name;
            FileOutput.append(".out");
            llvm::raw_fd_ostream Out(FileOutput.c_str(), Err);
            if(!Err.empty()) {
                llvm::errs() << Err << "\n";
                return 0;
            }
            llvm::WriteBitcodeToFile(M, Out);
            Out.flush();
            Out.close();

            if(false == BIDB.insertllToLL(FileOutput)) return 0;
			CIDB.insertModuleToList(File, M);
            std::string InputF = Clang.getFrontendOpts().Inputs[i].getFile();
            if(InputF.size() < RootFile.size()) continue;
            if(!InputF.compare(InputF.size() - RootFile.size(), RootFile.size(), RootFile)) RootModule = M;
  		}
  	}

	Function *rootFunctionPointer;
	//测试Module有效性
  	ModuleList *ML = CIDB.getModuleList();
    if(!RootModule || ( rootFunctionPointer = isLegal(RootModule, RootFunction) ) == NULL ) {
        printErrorFormat();
        printHelp();
        return 0;
    }

	//遍初始化
  	PassRegistry &Registry = *PassRegistry::getPassRegistry();
  	initializeCore(Registry);
  	initializeScalarOpts(Registry);
  	initializeIPO(Registry);
  	initializeAnalysis(Registry);
  	initializeTransformUtils(Registry);
  	initializeTarget(Registry);

	//调用自家写的遍
  	for(ModuleList::iterator s = ML->begin(), e = ML->end(); s != e; ++s) {
  		const std::string &ModuleDataLayout = (s->second)->getDataLayout();
  		DataLayout *TD = new DataLayout(ModuleDataLayout);

		//遍管理器
  		PassManager pm;
  		if (TD) pm.add(TD);

  		pm.add(new MultiFileCallGraph());

		//run other pass
		pm.run(*(s->second));
  	}

    CallGraphUtil CGU;
    CGU.processFPtr(RootModule, rootFunctionPointer->getName());
    CGU.generateDotGraph(RootModule, rootFunctionPointer->getName());
	//清理
  	delete Act;
  	llvm::llvm_shutdown();

  	return 0;
}
