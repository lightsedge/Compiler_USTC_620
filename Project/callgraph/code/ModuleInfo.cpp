////////////////////////////////////////////////////////////
/// COPYRIGHT NOTICE \n
/// Copyright (c) 2013, 中国科学技术大学(合肥) \n
/// All rights reserved. \n
///
/// @file ModuleInfo.cpp
/// @brief 本文件实现了FrontendInfoDB.h中声明的与Module相关的接口
///
/// @version 0.1
/// @author xtxwy
///
/// 版本修订: \n
///     无
//////////////////////////////////////////////////////////////
#include "FrontendInfoDB.h"
#include "llvm/Support/raw_ostream.h"

int DebugOutput_ModuleInfo = 0;
int ErrorOutput_ModuleInfo = 0;

bool FrontendCollectedInfoDB::insertModuleToList(std::string File, llvm::Module *M) {
	if(MList.end() == MList.find(File)) { //insert
		MList[File] = M;
	} else { //modify
		if(DebugOutput_ModuleInfo) {
			llvm::outs() << "File:" << File << "'s module has been changed.\n";
		}

		MList[File] = M;
	}

	return true;
}

ModuleList* FrontendCollectedInfoDB::getModuleList() {
	return &MList;
}

llvm::Module* FrontendCollectedInfoDB::getModuleofFile(std::string File) {
	if(MList.end() == MList.find(File)) {
		if(DebugOutput_ModuleInfo) llvm::outs() << "File is not found in map list.\n";
		return NULL;
	}

	return MList[File];
}

std::string FrontendCollectedInfoDB::getFileofModule(llvm::Module *M) {
	for(ModuleList::iterator s = MList.begin(), e = MList.end(); s != e; ++s) {
		if(!s->second->getModuleIdentifier().compare(M->getModuleIdentifier())) {
			return s->first;
		}
	}

	return NULL;
}
