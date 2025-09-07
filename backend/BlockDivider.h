#pragma once
#include <iostream>
#include <variant>
#include <vector>
#include <set>
#include <unordered_map>
#include "SemanticAnalyzer.h"


class SymbolInfo {
private:
	int next_use;						// ������һ��ʹ�õ�λ�ã�-1��ʾδʹ��
	bool is_active;						// �����Ƿ��Ծ
public:
	SymbolInfo(int use = -1, bool active = false);
	
	// getter/setter
	int getNextUse()const;				// ��ȡ������һ��ʹ�õ�λ��
	bool getIsActive()const;			// ��ȡ������ǰ�Ƿ��Ծ״̬
	void setNextUse(int use);			// ���ñ�����һ��ʹ�õ�λ��
	void setIsActive(bool active);		// ���ñ�����ǰ�Ƿ��Ծ״̬

	//DEBUG
	std::string toString() const;		// ���Դ�ӡ
};

struct QuadrupleInfo {
	SymbolInfo arg1_info;				// ��Ԫʽ��Դ������arg1�ı�����Ծ��Ϣ
	SymbolInfo arg2_info;				// ��Ԫʽ��Դ������arg2�ı�����Ծ��Ϣ
	SymbolInfo result_info;				// ��Ԫʽ��Ŀ�Ĳ�����result�ı�����Ծ��Ϣ

	QuadrupleInfo() = default;
};


class Block {
private:
	std::string name;					// ���������ƣ���block1,block2,...
	int start_addr;						// �������е�һ����Ԫʽ����ʼ��ַ
	std::vector<Quadruple> codes;		// ���ڸû��������Ԫʽ����
	Block* next1;						// ����������1����֧�ṹ��
	Block* next2;						// ����������2����֧�ṹ��

	// ��������Ϣ
	std::set<std::string> use_set;		// �����״γ���Ϊ���ñ����ļ���
	std::set<std::string> def_set;		// �����״γ���Ϊ��ֵ�����ļ���
	std::set<std::string> in_set;		// ��ڻ�Ծ��������
	std::set<std::string> out_set;		// ���ڻ�Ծ��������

	// ������Ծ��������Ϣ
	std::unordered_map<std::string, SymbolInfo> symbolInfoTable;				// �������ڱ����Ļ�Ծ/������Ϣ��	
	std::vector<QuadrupleInfo> codesInfo;										// ��Ӧÿ����Ԫʽ�ı�����Ծ/������Ϣ
	
	// ��������
	bool is_variable(const std::string name) const;									// �ж��Ƿ�Ϊ����
public:
	Block();

	// getter����
	std::string getName() const;										// ��ȡ����������
	int getStartAddr() const;											// ��ȡ��������ʼ��ַ
	const std::vector<Quadruple>& getCodes() const;						// ��ȡ���ڵ�ǰ���������Ԫʽ����
	const std::vector<QuadrupleInfo>& getCodesInfo() const;				// ��ȡ���ڵ�ǰ���������Ԫʽ�еı�����Ծ��Ϣ
	const Quadruple& getLastCode() const;								// ��ȡ���ڵ�ǰ����������һ����Ԫʽ
	const Block* getNext(int n);										// ��ȡ���������飨n=1,2,...��
	const std::set<std::string>& getOutSet() const;						// ��ȡ������ĳ��ڻ�Ծ������OUT����
	const std::set<std::string>& getInSet() const;						// ��ȡ���������ڻ�Ծ������IN����

	// setter����
	void setName(const std::string& n);									// ���û���������
	void setStartAddr(int addr);										// ���û�������ʼ��ַ
	void setNext(Block* next1, Block* next2);							// ���ú���������

	// ���㷽��
	void addCode(const Quadruple& code);								// ��ǰ���������Ԫʽ����������µ���Ԫʽ
	void computeUseDefSets();											// Ϊ��ǰ������������ã�USE�����ϺͶ�ֵ��DEF������
	bool computeInOutSet();												// Ϊ��ǰ�����������ڣ�IN���ͳ��ڣ�OUT����Ծ�������ϣ�bool�ͷ���ֵ�����ж�״̬�Ƿ����
	void computeSymbolInfo(std::unordered_map<std::string, SymbolInfo>& initSymbolInfoTable);	// Ϊ��ǰ�������������Ļ�Ծ������Ϣ

	// DEBUG
	void printBlockInfo();
};


class BlockDivider {
private:
	std::vector<Quadruple> qList;										// ��Ԫʽ����
	int start_address;													// ��ʼ��ַ
	int block_cnt;														// �Ѿ��������ɵĻ��������
	std::unordered_map<std::string, std::vector<Block*>> func_blocks;	// �����������֮���ӳ���ϵ��ÿ����������������Щ�����飩
	std::vector<Block*> blocks;										// ָ�����л�����ָ��

	// ��������
	std::string getBlockName();											// Ϊ�´����Ļ�������������
	void divideBlocks(const ProcedureTable& procTable);					// ���ݺ��������Ԫʽ���ֻ�����
	void computeBlocks();												// ������������USE/DEF����IN/OUT���ͱ�����Ծ��Ϣ
public:
	BlockDivider(const std::vector<Quadruple>& qList);					// ���캯���������﷨����������Ԫʽ���
	~BlockDivider();													// ��������	
	
	void BlockDivision(const ProcedureTable& procTable);				// �����黮�ֺ��Ĺ���

	// getter
	std::unordered_map<std::string, std::vector<Block*>>& getFuncBlocks();	// ��ú����������֮���ӳ���ϵ
	
	// DEBUG
	void printBlocks();
};