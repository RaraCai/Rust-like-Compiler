#pragma once
#include <iostream>
#include <variant>
#include <vector>
#include <set>
#include <unordered_map>
#include "SemanticAnalyzer.h"


class SymbolInfo {
private:
	int next_use;						// 变量下一次使用的位置，-1表示未使用
	bool is_active;						// 变量是否活跃
public:
	SymbolInfo(int use = -1, bool active = false);
	
	// getter/setter
	int getNextUse()const;				// 获取变量下一次使用的位置
	bool getIsActive()const;			// 获取变量当前是否活跃状态
	void setNextUse(int use);			// 设置变量下一次使用的位置
	void setIsActive(bool active);		// 设置变量当前是否活跃状态

	//DEBUG
	std::string toString() const;		// 调试打印
};

struct QuadrupleInfo {
	SymbolInfo arg1_info;				// 四元式中源操作数arg1的变量活跃信息
	SymbolInfo arg2_info;				// 四元式中源操作数arg2的变量活跃信息
	SymbolInfo result_info;				// 四元式中目的操作数result的变量活跃信息

	QuadrupleInfo() = default;
};


class Block {
private:
	std::string name;					// 基本块名称，如block1,block2,...
	int start_addr;						// 基本块中第一个四元式的起始地址
	std::vector<Quadruple> codes;		// 属于该基本块的四元式集合
	Block* next1;						// 后续基本块1（分支结构）
	Block* next2;						// 后续基本块2（分支结构）

	// 数据流信息
	std::set<std::string> use_set;		// 块内首次出现为引用变量的集合
	std::set<std::string> def_set;		// 块内首次出现为定值变量的集合
	std::set<std::string> in_set;		// 入口活跃变量集合
	std::set<std::string> out_set;		// 出口活跃变量集合

	// 变量活跃、待用信息
	std::unordered_map<std::string, SymbolInfo> symbolInfoTable;				// 基本块内变量的活跃/待用信息表	
	std::vector<QuadrupleInfo> codesInfo;										// 对应每个四元式的变量活跃/待用信息
	
	// 辅助方法
	bool is_variable(const std::string name) const;									// 判断是否为变量
public:
	Block();

	// getter方法
	std::string getName() const;										// 获取基本块名称
	int getStartAddr() const;											// 获取基本块起始地址
	const std::vector<Quadruple>& getCodes() const;						// 获取属于当前基本块的四元式集合
	const std::vector<QuadrupleInfo>& getCodesInfo() const;				// 获取属于当前基本块的四元式中的变量活跃信息
	const Quadruple& getLastCode() const;								// 获取属于当前基本块的最后一个四元式
	const Block* getNext(int n);										// 获取后续基本块（n=1,2,...）
	const std::set<std::string>& getOutSet() const;						// 获取基本块的出口活跃变量（OUT集）
	const std::set<std::string>& getInSet() const;						// 获取基本块的入口活跃变量（IN集）

	// setter方法
	void setName(const std::string& n);									// 设置基本块名称
	void setStartAddr(int addr);										// 设置基本块起始地址
	void setNext(Block* next1, Block* next2);							// 设置后续基本块

	// 计算方法
	void addCode(const Quadruple& code);								// 向当前基本块的四元式集合中添加新的四元式
	void computeUseDefSets();											// 为当前基本块计算引用（USE）集合和定值（DEF）集合
	bool computeInOutSet();												// 为当前基本块计算入口（IN）和出口（OUT）活跃变量集合，bool型返回值用于判断状态是否更新
	void computeSymbolInfo(std::unordered_map<std::string, SymbolInfo>& initSymbolInfoTable);	// 为当前基本块计算变量的活跃待用信息

	// DEBUG
	void printBlockInfo();
};


class BlockDivider {
private:
	std::vector<Quadruple> qList;										// 四元式集合
	int start_address;													// 起始地址
	int block_cnt;														// 已经划分生成的基本块计数
	std::unordered_map<std::string, std::vector<Block*>> func_blocks;	// 函数与基本块之间的映射关系（每个函数都包含了哪些基本块）
	std::vector<Block*> blocks;										// 指向所有基本块指针

	// 辅助方法
	std::string getBlockName();											// 为新创建的基本块生成名称
	void divideBlocks(const ProcedureTable& procTable);					// 依据函数表和四元式划分基本块
	void computeBlocks();												// 计算各基本块的USE/DEF集、IN/OUT集和变量活跃信息
public:
	BlockDivider(const std::vector<Quadruple>& qList);					// 构造函数，接受语法分析器的四元式结果
	~BlockDivider();													// 析构函数	
	
	void BlockDivision(const ProcedureTable& procTable);				// 基本块划分核心过程

	// getter
	std::unordered_map<std::string, std::vector<Block*>>& getFuncBlocks();	// 获得函数与基本块之间的映射关系
	
	// DEBUG
	void printBlocks();
};