#include"CodeGenerator.h"
#include<cctype>
#include<map>

RegManager::RegManager(int regs_num): regs_num(regs_num), frame_size(0)
{
	this->free_regs.reserve(regs_num);
	for (int i = 0; i < regs_num; ++i) {
		free_regs.push_back("$s" + std::to_string(regs_num - i - 1));
	}
}

bool RegManager::is_variable(const std::string& name) const
{
	if (name.empty())
		return false;
	else if (std::isalpha(name[0]))
		return true;
	else if (name[0] == '-' && name.length() > 1)
		return true;
	return false;
}

void RegManager::setFuncVars(const std::set<std::string>& vars)
{
	this->func_vars = vars;
}

void RegManager::setDataVars(const std::set<std::string>& vars)
{
	this->data_vars = vars;
}

void RegManager::setFrameSize(const int& frame_size)
{
	this->frame_size = frame_size;
}

int RegManager::getFrameSize() const
{
	return this->frame_size;
}

std::set<std::string>& RegManager::getAValue(const std::string& var)
{
	return this->AValue[var];
}

int RegManager::getMemory(const std::string& var) 
{
	return this->memory[var];
}

void RegManager::freeVarRegs(const std::string& var)
{
	// 全局变量不清空
	if (this->data_vars.find(var) != this->data_vars.end() && 
		this->func_vars.find(var) == this->func_vars.end()) {
		return;
	}

	for (const auto& pos : this->AValue[var]) {
		if (pos != "Mem") {// 不在内存，在寄存器中
			this->RValue[pos].erase(var); // 寄存器中清空
			if (this->RValue[pos].empty()) {
				this->free_regs.push_back(pos);	// 空闲寄存器列表增加此寄存器
			}
		}
	}
	this->AValue[var].clear();
}

void RegManager::freeAllRegs(const std::set<std::string>in_set)
{
	this->RValue.clear();
	this->AValue.clear();

	// 入口活跃变量标记在内存
	for (const auto& var : in_set) {
		this->AValue[var].insert("Mem");
	}
	// 重置寄存器空闲列表
	this->free_regs.clear();
	for (int i = 0; i < this->regs_num; ++i) {
		this->free_regs.push_back("$s" + std::to_string(this->regs_num - i - 1));
	}
}

void RegManager::freeReg(const std::string& reg)
{
	this->RValue[reg].clear();
	this->free_regs.push_back(reg);
}

void RegManager::storeVarToReg(const std::string& var, const std::string& reg)
{
	this->RValue[reg].insert(var);
	this->AValue[var].insert(reg);
}

void RegManager::storeVarToMem(const std::string& var, const std::string& reg, std::vector<std::string>& codes) 
{
	// 分配内存空间
	if (this->memory.find(var) == this->memory.end()) {// 当前变量不在内存中，则分配新空间
		this->memory[var] = this->frame_size;
		this->frame_size += 4;
	}
	this->AValue[var].insert("Mem"); // 更新该变量的AValue

	// 生成存储命令
	if (this->func_vars.count(var) || var[0] == 'T') {
		std::string code = "sw " + reg + ", " + std::to_string(this->memory[var]) + "($sp)";
		codes.push_back(code);
	}
	else if (this->data_vars.count(var)) {
		std::string code = "sw " + reg + ", " + var;
		codes.push_back(code);
	}
		
}

void RegManager::storeOutSetToMem(const std::set<std::string>& out_set, std::vector<std::string>& codes)
{
	for (const auto& var : out_set) {
		std::string reg;
		bool in_mem = false;
		// 检查变量是否已经在内存中
		for (const auto& pos : this->AValue[var]) {
			if (pos == "Mem") {
				in_mem = true;
				break;
			}
			else {
				reg = pos;
			}
		}
		// 如果不在内存但在寄存器中，则存储到内存
		if (!in_mem && !reg.empty()) {
			this->storeVarToMem(var, reg, codes);
		}
	}
}

std::string RegManager::allocFreeReg(const std::vector<Quadruple>& qList, int cur_q_idx, const std::set<std::string>& out_set, std::vector<std::string>& codes)
{
	// 若有空闲寄存器，直接分配
	if (!this->free_regs.empty()) {
		std::string free_reg = this->free_regs.back();
		this->free_regs.pop_back();
		return free_reg;
	}
	// 无空闲寄存器，寻找最远才会被使用的变量所在寄存器
	double farest_usepos = std::numeric_limits<double>::lowest();
	std::string selected_reg;
	for (const auto& [reg, vars] : this->RValue) {
		double cur_usepos = std::numeric_limits<double>::infinity();

		for (const auto& var : vars) {
			if (this->AValue[var].size() > 1) {
				// 该变量存在多处副本，可直接跳过
				continue;
			}

			// 该变量仅有一个副本在当前寄存器，则寻找该变量在四元式中最近使用的位置
			for (size_t i = cur_q_idx; i < qList.size(); ++i) {
				const auto& q = qList[i];
				if (var == q.arg1 || var == q.arg2) {
					cur_usepos = std::min(cur_usepos, static_cast<double>(i - cur_q_idx));
					break;
				}
				if (var == q.result) {
					break;
				}
			}
		}
		//更新最远使用位置的寄存器
		if (cur_usepos == std::numeric_limits<double>::infinity()) {// 防止farest_usepos被赋值为+inf
			selected_reg = reg;
			break;
		}
		else if (cur_usepos > farest_usepos) {
			farest_usepos = cur_usepos;
			selected_reg = reg;
		}
	}

	// 处理原来存放在选中寄存器的变量
	for (const auto& var : this->RValue[selected_reg]) {
		this->AValue[var].erase(selected_reg);
		// 判断是否要保存到内存
		if (this->AValue[var].empty()) {// 已无其他副本
			bool need_store = false;
			// 检查后续四元式，看是否还会使用
			for (size_t i = cur_q_idx; i < qList.size(); ++i) {
				const auto& q = qList[i];
				if (var == q.arg1 || var == q.arg2) {
					need_store = true;
					break;
				}
				if (var == q.result) {
					break;
				}
			}
			// 若在当前基本块内后续未使用，但是属于基本块的出口活跃变量，也需要保存
			if (need_store == false) {
				if (out_set.find(var) != out_set.end()) {
					need_store = true;
				}
			}

			// 若需要保存，则存入
			if (need_store) {
				this->storeVarToMem(var, selected_reg, codes);
			}
		}
	}

	// 释放选中的寄存器
	this->RValue[selected_reg].clear();
	return selected_reg;
}

std::string RegManager::getArgReg(const std::string& arg, const std::vector<Quadruple>& qList, int cur_q_idx, const std::set<std::string>& out_set, std::vector<std::string>& codes)
{
	std::string reg;
	// 先检查AValue中是否已经有副本
	for (const auto& pos : this->AValue[arg]) {
		if (pos != "Mem") {
			return pos;
		}
	}
	
	// 没有其他副本可以直接使用，则需要请求分配一个寄存器
	reg = this->allocFreeReg(qList, cur_q_idx, out_set, codes);

	// 生成存取命令
	if (this->is_variable(arg)) { // 局部变量或中间变量
		if (this->func_vars.count(arg) || arg[0] == 'T') {
			std::string code = "lw " + reg + ", " + std::to_string(this->memory[arg]) + "($sp)";
			codes.push_back(code);
		}
		else if (this->data_vars.count(arg)) {
			std::string code = "lw " + reg + ", " + arg;
			codes.push_back(code);
		}
	}
	else { // 立即数
		std::string code = "li " + reg + ", " + arg;
		codes.push_back(code);
	}

	return reg;
}

std::string RegManager::getResultReg(const std::string& result, const std::vector<Quadruple>& qList, const std::vector<QuadrupleInfo>& qInfoList, int cur_q_idx, const std::set<std::string>& out_set, std::vector<std::string>& codes)
{
	const auto& q = qList[cur_q_idx];
	const auto& qInfo = qInfoList[cur_q_idx];
	std::string arg1 = q.arg1;

	// 判断能否复用arg1的寄存器（确保arg1是变量，且不抢占全局变量的寄存器）
	if (this->is_variable(arg1)) {
		if (this->data_vars.find(arg1) != this->data_vars.end() &&
			this->func_vars.find(arg1) == this->func_vars.end()) {
			for (const auto& pos : this->AValue[arg1]) {
				if (pos != "Mem" && this->RValue[pos].size() == 1) {
					if (qInfo.arg1_info.getIsActive() == false) {
						// 更新RValue和AValue关系
						this->RValue[pos].erase(arg1);
						this->RValue[pos].insert(result);
						
						this->AValue[arg1].erase(pos);
						this->AValue[result].insert(pos);

						return pos;
					}
				}
			}
		}
	}

	// 重新分配寄存器
	std::string reg = this->allocFreeReg(qList, cur_q_idx, out_set, codes);
	this->RValue[reg].insert(result);
	this->AValue[result].insert(reg);

	return reg;
}

void RegManager::allocStackFrame(const std::string& func_name, const std::set<std::string>& vars)
{
	// 设置栈帧初始大小(ra和旧sp各占4字节)
	this->frame_size = (func_name != "main") ? 2 * 4 : 0;
	// 清空内存状态
	this->resetMemory();
	// 为所有局部变量预留空间，但因没有具体值暂不生成存数指令
	for (const auto& var : vars) {
		this->memory[var] = this->frame_size;
		this->AValue[var].insert("Mem");
		this->frame_size += 4;
	}
}

void RegManager::resetMemory()
{
	this->memory.clear();
}

CodeGenerator::CodeGenerator(const std::unordered_map<std::string, std::vector<Block*>>& func_blocks, const ProcedureTable& procTable) :func_blocks(func_blocks)
{
	// 从procTable中提取func_vars
	for (const auto& proc : procTable) {
		// 生成每个函数及其对应的局部变量的键值对，初始化func_vars
		std::set<std::string> local_vars; // 当前函数的局部变量集合
		
		// 获取当前函数的符号表
		SymbolTable* symboltbl = proc.symbolTable;	

		// 找到对应的函数表后，提取该函数的形参和局部变量
		if (symboltbl != nullptr) {
			for (const auto& entry : symboltbl->table) {
				if (entry.kind == SymbolKind::Variable) {
					local_vars.insert(entry.ID);
				}
			}
		}
		
		// 将当前函数的局部变量信息存入func_vars
		this->func_vars[proc.ID] = local_vars;
	}

	// 规定rust语法上不允许存在全局变量，data_vars应为空
	this->data_vars.clear();
	
	// 同步初始化RegMgr的全局变量信息
	this->regMgr.setDataVars(data_vars);
}

void CodeGenerator::genFuncObjCode(const std::string& func_name, const std::vector<Block*>& blocks)
{
	// 1. 重置regMgr状态
	this->regMgr.resetMemory();
	this->regMgr.setFuncVars(this->func_vars[func_name]);

	// 2. 处理每个基本块
	for (size_t i = 0; i < blocks.size(); ++i) {
		Block* blk = blocks[i];
		std::string label = blk->getName() + ":";
		this->codes.push_back(label);

		// 3. 如果是函数的第一个基本块，需要处理函数序言
		if (i == 0) {
			// 非main函数保存返回地址
			if (func_name != "main"){
				this->codes.push_back("sw $ra, 4($sp)");
			}
			// 分配栈帧空间
			this->regMgr.allocStackFrame(func_name, this->func_vars[func_name]);
		}

		// 4. 生成基本块内部代码
		this->genBlkObjCode(blk, func_name);
	}
}

void CodeGenerator::genBlkObjCode(Block* block, const std::string& func_name)
{
	// 1. 初始化寄存器状态和函数形参列表
	this->regMgr.freeAllRegs(block->getInSet());
	this->param_list.clear();

	// 2. 处理基本块中的每个四元式
	const auto& codes = block->getCodes();
	const auto& codesInfo = block->getCodesInfo();
	for (size_t i = 0; i < codes.size(); ++i) {
		const auto& q = codes[i];
		const auto& qInfo = codesInfo[i];

		if (i == codes.size() - 1) { // 最后一个基本块需特殊处理:出口活跃变量和未定值的全局变量需保存到内存
			auto outset = block->getOutSet();
			outset.insert(this->data_vars.begin(), this->data_vars.end());

			if (q.op == "j" || q.op == "inz" || q.op == "ret") {
				this->regMgr.storeOutSetToMem(outset, this->codes);
				this->genQuarObjCode(q, qInfo, i, block, func_name);
			}
			else if (q.op == "call") {
				// 函数调用前保存除目标变量外的出口活跃变量
				auto tmp_outset = outset;
				tmp_outset.erase(q.result);
				this->regMgr.storeOutSetToMem(tmp_outset, this->codes);

				this->genQuarObjCode(q, qInfo, i, block, func_name);

				// 保存目标变量
				if (!q.result.empty()) {
					std::set<std::string> result_set{ q.result };
					this->regMgr.storeOutSetToMem(result_set, this->codes);
				}
			}
			else {
				this->genQuarObjCode(q, qInfo, i, block, func_name);
				this->regMgr.storeOutSetToMem(outset, this->codes);
			}
		}
		else { // 普通基本块
			this->genQuarObjCode(q, qInfo, i, block, func_name);
		}
	}
}

void CodeGenerator::genQuarObjCode(const Quadruple& q, const QuadrupleInfo& qInfo, int q_idx, Block* block, const std::string& func_name)
{
	if (!this->GetGenErrors().empty())// 有错则不再生成
		return;

	if (q.op == "=") {
		std::string reg = this->regMgr.getArgReg(q.arg1, block->getCodes(), q_idx, block->getOutSet(), this->codes);
		this->regMgr.freeVarRegs(q.result);
		// 更新RValue和AValue
		this->regMgr.storeVarToReg(q.result, reg);
	}
	else if (q.op == "ret") {
		// 操作数1是否为变量
		if (this->regMgr.is_variable(q.arg1)) {
			std::string reg;
			for (const auto& pos : this->regMgr.getAValue(q.arg1)) {
				if (pos != "Mem") {
					reg = pos;
					break;
				}
			}
			if (!reg.empty()) {
				std::string code = "add $v0, " + reg + ", $zero";
				this->codes.push_back(code);
			}
			else {
				std::string code = "lw $v0, " + std::to_string(this->regMgr.getMemory(q.arg1)) + "($sp)";
				this->codes.push_back(code);
			}
		}
		else if (q.arg1 != "-") {
			std::string code = "li $v0, " + q.arg1;
			this->codes.push_back(code);
		}

		// 是否为main函数
		if (func_name == "main") {// main函数直接结束全部程序，跳转到end
			this->codes.push_back("j end");
		}
		else {
			this->codes.push_back("lw $ra, 4($sp)"); // 非main函数需要取回地址到ra中跳转
			this->codes.push_back("jr $ra");
		}
	}
	else if (q.op == "para") {
		this->param_list.push_back( ParamInfo(q.arg1, qInfo.arg1_info.getIsActive()));
	}
	else if (q.op == "call") {
		int top = 0;
		for (const auto& para : this->param_list) {
			std::string reg = this->regMgr.getArgReg(para.name, block->getCodes(), q_idx, block->getOutSet(), this->codes);
			std::string code = "sw " + reg + ", " + std::to_string(2 * 4 + top + this->regMgr.getFrameSize()) + "($sp)";
			this->codes.push_back(code);

			top += 4;
			if (para.is_active == false) {
				this->regMgr.freeVarRegs(para.name);
			}
		}
		// 存储老sp并将sp指向新位置
		std::string code = "sw $sp, " + std::to_string(this->regMgr.getFrameSize()) + "($sp)";
		this->codes.push_back(code);
		code = "addi $sp, $sp, " + std::to_string(this->regMgr.getFrameSize());
		this->codes.push_back(code);

		// 转子
		code = "jal " + q.arg1;
		this->codes.push_back(code);

		// 返回，恢复老sp
		this->codes.push_back("lw $sp, 0($sp)");

		// $v0中的返回值填写到对应的临时变量
		if (q.result != "-") {
			std::string rd = this->regMgr.getResultReg(q.result, block->getCodes(), block->getCodesInfo(), q_idx, block->getOutSet(), this->codes);
			code = "add " + rd + ", $v0, $zero";
			this->codes.push_back(code);
		}
	}
	else if (q.op == "+" || q.op == "-" || q.op == "*" || q.op == "/" || q.op == "%") {// 算术运算符
		std::string rs = this->regMgr.getArgReg(q.arg1, block->getCodes(), q_idx, block->getOutSet(), this->codes);
		std::string rt = this->regMgr.getArgReg(q.arg2, block->getCodes(), q_idx, block->getOutSet(), this->codes);
		std::string rd = this->regMgr.getResultReg(q.result, block->getCodes(), block->getCodesInfo(), q_idx, block->getOutSet(), this->codes);

		std::string code;
		if (q.op == "+") {
			code = "add " + rd + ", " + rs + ", " + rt;
			this->codes.push_back(code);
		}
		else if (q.op == "-") {
			code = "sub " + rd + ", " + rs + ", " + rt;
			this->codes.push_back(code);
		}
		else if (q.op == "*") {
			code = "mul " + rd + ", " + rs + ", " + rt;
			this->codes.push_back(code);
		}
		else if (q.op == "/") {
			code = "div " + rs + ", " + rt;
			this->codes.push_back(code);
			code = "mflo " + rd;	// 获取商
			this->codes.push_back(code);
		}
		else if (q.op == "%") {
			code = "div " + rs + ", " + rt;
			this->codes.push_back(code);
			code = "mfhi " + rd;	// 获取余数
			this->codes.push_back(code);
		}

		// arg1
		if (!this->regMgr.is_variable(q.arg1)) {
			this->regMgr.freeReg(rs);
		}
		else if (!qInfo.arg1_info.getIsActive() && q.arg1 != q.result) {
			this->regMgr.freeVarRegs(q.arg1);
		}
		// arg2
		if (!this->regMgr.is_variable(q.arg2)) {
			this->regMgr.freeReg(rt);
		}
		else if (!qInfo.arg2_info.getIsActive() && q.arg2 != q.result) {
			this->regMgr.freeVarRegs(q.arg2);
		}
	}
	else if (q.op == "==" || q.op == "!=" || q.op == "<" || q.op == "<=" || q.op == ">" || q.op == ">=") {// 比较运算符
		std::map<std::string, std::string> op_map = {
			{"<", "slt"},
			{">", "sgt"},
			{"<=", "sle"},
			{">=", "sge"},
			{"==", "seq"},
			{"!=", "sne"}
		};
		std::string rs = this->regMgr.getArgReg(q.arg1, block->getCodes(), q_idx, block->getOutSet(), this->codes);
		std::string rt = this->regMgr.getArgReg(q.arg2, block->getCodes(), q_idx, block->getOutSet(), this->codes);
		std::string rd = this->regMgr.getResultReg(q.result, block->getCodes(), block->getCodesInfo(), q_idx, block->getOutSet(), this->codes);

		std::string code = op_map[q.op] + " " + rd + ", " + rs + ", " + rt;
		this->codes.push_back(code);

		// arg1
		if (!this->regMgr.is_variable(q.arg1)) {
			this->regMgr.freeReg(rs);
		}
		else if (!qInfo.arg1_info.getIsActive() && q.arg1 != q.result) {
			this->regMgr.freeVarRegs(q.arg1);
		}
		// arg2
		if (!this->regMgr.is_variable(q.arg2)) {
			this->regMgr.freeReg(rt);
		}
		else if (!qInfo.arg2_info.getIsActive() && q.arg2 != q.result) {
			this->regMgr.freeVarRegs(q.arg2);
		}
	}
	else if (q.op == "j") {// 无条件跳转
		std::string code = "j " + q.result;
		this->codes.push_back(code);
	}
	else if (q.op == "j==" || q.op == "j!=" || q.op == "j<" || q.op == "j<=" || q.op == "j>" || q.op == "j>=") {//条件跳转
		std::string rs = this->regMgr.getArgReg(q.arg1, block->getCodes(), q_idx, block->getOutSet(), this->codes);
		std::string rt = this->regMgr.getArgReg(q.arg2, block->getCodes(), q_idx, block->getOutSet(), this->codes);

		std::string code;
		if (q.op == "j==") {
			code = "beq " + rs + ", " + rt + ", " + q.result;
		}
		else if (q.op == "j!=") {
			code = "bne " + rs + ", " + rt + ", " + q.result;
		}
		else if (q.op == "j<") {
			code = "blt " + rs + ", " + rt + ", " + q.result;
		}
		else if (q.op == "j<=") {
			code = "ble " + rs + ", " + rt + ", " + q.result;
		}
		else if (q.op == "j>") {
			code = "blt " + rt + ", " + rs + ", " + q.result;
		}
		else if (q.op == "j>=") {
			code = "bge " + rs + ", " + rt + ", " + q.result;
		}
		this->codes.push_back(code);

		// arg1
		if (!this->regMgr.is_variable(q.arg1)) {
			this->regMgr.freeReg(rs);
		}
		else if (!qInfo.arg1_info.getIsActive() && q.arg1 != q.result) {
			this->regMgr.freeVarRegs(q.arg1);
		}
		// arg2
		if (!this->regMgr.is_variable(q.arg2)) {
			this->regMgr.freeReg(rt);
		}
		else if (!qInfo.arg2_info.getIsActive() && q.arg2 != q.result) {
			this->regMgr.freeVarRegs(q.arg2);
		}
	}
	else if (q.op == "ref" || q.op == "mutref") {
		std::string arg1_reg = this->regMgr.getArgReg(q.arg1, block->getCodes(), q_idx, block->getOutSet(), this->codes);
		std::string result_reg = this->regMgr.getResultReg(q.result, block->getCodes(), block->getCodesInfo(), q_idx, block->getOutSet(), this->codes);
		
		std::string code = "add " + result_reg + ", " + arg1_reg + ", $zero";
		this->codes.push_back(code);
	}
	else if (q.op == "deref") {
		std::string arg1_reg = this->regMgr.getArgReg(q.arg1, block->getCodes(), q_idx, block->getOutSet(), this->codes);
		std::string result_reg = this->regMgr.getResultReg(q.result, block->getCodes(), block->getCodesInfo(), q_idx, block->getOutSet(), this->codes);
		// 解引用
		std::string code = "lw " + result_reg + ", 0(" + arg1_reg + ")";
		this->codes.push_back(code);
	}
	else {
		std::string error = "genError:目标代码生成暂不支持的复杂运算(" + q.op + ")";
		this->genErrors.push_back(error);
	}
	
}

std::vector<std::string> CodeGenerator::genObjCode()
{
	// 1. 生成数据段
	this->codes.push_back(".data");
	for (const auto& var : this->data_vars) {
		std::string code = var + ": .word 0";
		this->codes.push_back(code);
	}
	this->codes.push_back("");

	// 2. 生成代码段
	this->codes.push_back(".text");
	this->codes.push_back("lui $sp, 0x1004"); // 栈指针初始值
	this->codes.push_back("j main");	// 默认先执行main函数

	// 3. 为每个函数生成代码
	for (const auto& [func, blocks] : this->func_blocks) {
		this->genFuncObjCode(func, blocks);
	}
	this->codes.push_back("end:");

	// 4. 添加缩进美化输出
	for (auto& code : this->codes) {
		if (!code.empty() && code[0] != '.' && code.back() != ':') {
			code = "\t" + code;
		}
	}

	return this->codes;
}

const std::vector<std::string>& CodeGenerator::GetGenErrors() const
{
	return this->genErrors;
}