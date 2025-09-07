#pragma once
#include"SemanticAnalyzer.h"
#include"BlockDivider.h"
#include<string>
#include<vector>
#include<set>
#include<unordered_map>

class RegManager {
private:
	// 核心数据结构
	std::unordered_map<std::string, std::set<std::string>> RValue;     // 寄存器中存储了哪些变量
	std::unordered_map<std::string, std::set<std::string>> AValue;     // 变量位于哪些寄存器

	int regs_num;								// 可用寄存器数量
	int frame_size;								// 当前栈帧大小
	std::vector<std::string> free_regs;			// 当前空闲寄存器列表
	std::unordered_map<std::string, int>memory;	// 变量内存地址映射
	std::set<std::string> func_vars;			// 局部变量集合
	std::set<std::string> data_vars;			// 全局变量集合

public:
	RegManager(int regs_num = 32);				 			// 构造函数。默认32个寄存器

	// 辅助方法
	bool is_variable(const std::string& name) const;

	// getter/setter
	void setFuncVars(const std::set<std::string>& vars);	// 设置局部变量
	void setDataVars(const std::set<std::string>& vars);	// 设置全局变量
	void setFrameSize(const int& frame_size);				// 设置栈帧大小
	int getFrameSize()const;								// 获取栈帧大小
	std::set<std::string>& getAValue(const std::string& var);// 获取某变量的AValue集合
	int getMemory(const std::string& var);					// 获取某变量在内存的地址空间
	
	// 存储空间管理
	void freeVarRegs(const std::string& var);				// 释放某个变量占用的所有寄存器
	void freeAllRegs(const std::set<std::string>in_set);	// 释放所有寄存器（基本块开始时调用）
	void freeReg(const std::string& reg);					// 释放某个寄存器
	void storeVarToReg(										// 变量存储到寄存器
		const std::string& var,
		const std::string& reg);
	void storeVarToMem(										// 变量存储到内存
		const std::string& var,
		const std::string& reg,
		std::vector<std::string>& codes);				
	void storeOutSetToMem(									// 存储出口活跃变量到内存
		const std::set<std::string>& out_set,
		std::vector<std::string>& codes);
	void resetMemory();										// 重置存储器状态

	// 寄存器分配
	std::string allocFreeReg(								// 分配一个空闲寄存器			
		const std::vector<Quadruple>& qList,
		int cur_q_idx,
		const std::set<std::string>& out_set,
		std::vector<std::string>& codes);
	std::string getArgReg(									// 源操作数arg获取寄存器
		const std::string& arg,
		const std::vector<Quadruple>& qList,
		int cur_q_idx,
		const std::set<std::string>& out_set,
		std::vector<std::string>& codes);
	std::string getResultReg(								// 目的操作数result获取寄存器
		const std::string& result,
		const std::vector<Quadruple>& qList,
		const std::vector<QuadrupleInfo>& qInfoList,
		int cur_q_idx,
		const std::set<std::string>& out_set,
		std::vector<std::string>& codes);

	// 函数栈帧空间分配
	void allocStackFrame(const std::string& func_name, const std::set<std::string>& vars);
};

// 函数参数列表中的参数活跃信息
struct ParamInfo {
	std::string name;		// 参数名称
	bool is_active;			// 参数是否活跃

	ParamInfo(const std::string& n = "", bool a = false) 
		:name(n), is_active(a) {}
};

class CodeGenerator {
private:
	RegManager regMgr;													// 寄存器管理
	std::vector<std::string> codes;										// 目标代码

	std::unordered_map<std::string, std::vector<Block*>> func_blocks;	// 函数和基本快之间的映射关系表

	// 变量管理
	std::unordered_map<std::string, std::set<std::string>> func_vars;	// 函数局部变量表
	std::set<std::string> data_vars;									// 全局变量集合
	std::vector<ParamInfo> param_list;									// 函数调用参数列表

	// 错误处理
	std::vector<std::string> genErrors;									// 代码生成阶段的错误

	// 辅助代码生成方法
	void genFuncObjCode(const std::string& func_name,					// 生成函数级目标代码
		const std::vector<Block*>& blocks);
	void genBlkObjCode(Block* block,									// 生成基本块级目标代码
		const std::string& func_name);
	void genQuarObjCode(const Quadruple& q,								// 生成四元式级目标代码
		const QuadrupleInfo& qInfo,
		int q_idx,
		Block* block,
		const std::string& func_name);
public:
	CodeGenerator(const std::unordered_map<std::string, std::vector<Block*>>& func_blocks, const ProcedureTable& procTable);								
																		// 构造函数
	std::vector<std::string> genObjCode();								// 生成目标代码的核心过程函数
	const std::vector<std::string>& GetGenErrors() const;				// 获取目标代码生成阶段的错误
};


