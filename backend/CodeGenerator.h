#pragma once
#include"SemanticAnalyzer.h"
#include"BlockDivider.h"
#include<string>
#include<vector>
#include<set>
#include<unordered_map>

class RegManager {
private:
	// �������ݽṹ
	std::unordered_map<std::string, std::set<std::string>> RValue;     // �Ĵ����д洢����Щ����
	std::unordered_map<std::string, std::set<std::string>> AValue;     // ����λ����Щ�Ĵ���

	int regs_num;								// ���üĴ�������
	int frame_size;								// ��ǰջ֡��С
	std::vector<std::string> free_regs;			// ��ǰ���мĴ����б�
	std::unordered_map<std::string, int>memory;	// �����ڴ��ַӳ��
	std::set<std::string> func_vars;			// �ֲ���������
	std::set<std::string> data_vars;			// ȫ�ֱ�������

public:
	RegManager(int regs_num = 32);				 			// ���캯����Ĭ��32���Ĵ���

	// ��������
	bool is_variable(const std::string& name) const;

	// getter/setter
	void setFuncVars(const std::set<std::string>& vars);	// ���þֲ�����
	void setDataVars(const std::set<std::string>& vars);	// ����ȫ�ֱ���
	void setFrameSize(const int& frame_size);				// ����ջ֡��С
	int getFrameSize()const;								// ��ȡջ֡��С
	std::set<std::string>& getAValue(const std::string& var);// ��ȡĳ������AValue����
	int getMemory(const std::string& var);					// ��ȡĳ�������ڴ�ĵ�ַ�ռ�
	
	// �洢�ռ����
	void freeVarRegs(const std::string& var);				// �ͷ�ĳ������ռ�õ����мĴ���
	void freeAllRegs(const std::set<std::string>in_set);	// �ͷ����мĴ����������鿪ʼʱ���ã�
	void freeReg(const std::string& reg);					// �ͷ�ĳ���Ĵ���
	void storeVarToReg(										// �����洢���Ĵ���
		const std::string& var,
		const std::string& reg);
	void storeVarToMem(										// �����洢���ڴ�
		const std::string& var,
		const std::string& reg,
		std::vector<std::string>& codes);				
	void storeOutSetToMem(									// �洢���ڻ�Ծ�������ڴ�
		const std::set<std::string>& out_set,
		std::vector<std::string>& codes);
	void resetMemory();										// ���ô洢��״̬

	// �Ĵ�������
	std::string allocFreeReg(								// ����һ�����мĴ���			
		const std::vector<Quadruple>& qList,
		int cur_q_idx,
		const std::set<std::string>& out_set,
		std::vector<std::string>& codes);
	std::string getArgReg(									// Դ������arg��ȡ�Ĵ���
		const std::string& arg,
		const std::vector<Quadruple>& qList,
		int cur_q_idx,
		const std::set<std::string>& out_set,
		std::vector<std::string>& codes);
	std::string getResultReg(								// Ŀ�Ĳ�����result��ȡ�Ĵ���
		const std::string& result,
		const std::vector<Quadruple>& qList,
		const std::vector<QuadrupleInfo>& qInfoList,
		int cur_q_idx,
		const std::set<std::string>& out_set,
		std::vector<std::string>& codes);

	// ����ջ֡�ռ����
	void allocStackFrame(const std::string& func_name, const std::set<std::string>& vars);
};

// ���������б��еĲ�����Ծ��Ϣ
struct ParamInfo {
	std::string name;		// ��������
	bool is_active;			// �����Ƿ��Ծ

	ParamInfo(const std::string& n = "", bool a = false) 
		:name(n), is_active(a) {}
};

class CodeGenerator {
private:
	RegManager regMgr;													// �Ĵ�������
	std::vector<std::string> codes;										// Ŀ�����

	std::unordered_map<std::string, std::vector<Block*>> func_blocks;	// �����ͻ�����֮���ӳ���ϵ��

	// ��������
	std::unordered_map<std::string, std::set<std::string>> func_vars;	// �����ֲ�������
	std::set<std::string> data_vars;									// ȫ�ֱ�������
	std::vector<ParamInfo> param_list;									// �������ò����б�

	// ������
	std::vector<std::string> genErrors;									// �������ɽ׶εĴ���

	// �����������ɷ���
	void genFuncObjCode(const std::string& func_name,					// ���ɺ�����Ŀ�����
		const std::vector<Block*>& blocks);
	void genBlkObjCode(Block* block,									// ���ɻ����鼶Ŀ�����
		const std::string& func_name);
	void genQuarObjCode(const Quadruple& q,								// ������Ԫʽ��Ŀ�����
		const QuadrupleInfo& qInfo,
		int q_idx,
		Block* block,
		const std::string& func_name);
public:
	CodeGenerator(const std::unordered_map<std::string, std::vector<Block*>>& func_blocks, const ProcedureTable& procTable);								
																		// ���캯��
	std::vector<std::string> genObjCode();								// ����Ŀ�����ĺ��Ĺ��̺���
	const std::vector<std::string>& GetGenErrors() const;				// ��ȡĿ��������ɽ׶εĴ���
};


