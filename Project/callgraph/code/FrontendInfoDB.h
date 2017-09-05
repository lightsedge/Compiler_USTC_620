////////////////////////////////////////////////////////////
/// COPYRIGHT NOTICE \n
/// Copyright (c) 2013, 中国科学技术大学(合肥) \n
/// All rights reserved. \n
///
/// @file FrontendInfoDB.h
/// @brief 本文件维护了编译过程前端收集到的信息
///
/// 本文间的当前实现只维护了每个文件和对应的Module信息
///
/// @version 0.1
/// @author xtxwy, Peter Chang
///
/// 版本修订: \n
///     无
////////////////////////////////////////////////////////////
#ifndef _FRONTENDINFODB_H_
#define _FRONTENDINFODB_H_

#include "llvm/IR/Module.h"
#include <map>

/// 存放与Module相关的信息
typedef std::map<std::string, llvm::Module*> ModuleList;

/// @brief 前端信息数据库,当前只保存了模块信息
///
/// 由于此分析的主要对象是LLVM IR，所以前端需要收集一些信息来 \n
/// 方便IR分析时的语义解析。
class FrontendCollectedInfoDB {
public:
	FrontendCollectedInfoDB() { MList.clear(); }
	~FrontendCollectedInfoDB() {};

	/// 保存源码文件名字和生成的Module
	bool insertModuleToList(std::string File, llvm::Module *M);
	/// 得到维护的所有源码文件和对应的Module信息
	ModuleList *getModuleList();
	/// 根据源码文件名称得到对应的Module
	llvm::Module *getModuleofFile(std::string File);
	/// 根据Module得到其对应的源码文件
	std::string getFileofModule(llvm::Module *M);
private:
	ModuleList MList;
};

extern FrontendCollectedInfoDB CIDB;
#endif
