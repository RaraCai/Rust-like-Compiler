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
	// ȫ�ֱ��������
	if (this->data_vars.find(var) != this->data_vars.end() && 
		this->func_vars.find(var) == this->func_vars.end()) {
		return;
	}

	for (const auto& pos : this->AValue[var]) {
		if (pos != "Mem") {// �����ڴ棬�ڼĴ�����
			this->RValue[pos].erase(var); // �Ĵ��������
			if (this->RValue[pos].empty()) {
				this->free_regs.push_back(pos);	// ���мĴ����б����Ӵ˼Ĵ���
			}
		}
	}
	this->AValue[var].clear();
}

void RegManager::freeAllRegs(const std::set<std::string>in_set)
{
	this->RValue.clear();
	this->AValue.clear();

	// ��ڻ�Ծ����������ڴ�
	for (const auto& var : in_set) {
		this->AValue[var].insert("Mem");
	}
	// ���üĴ��������б�
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
	// �����ڴ�ռ�
	if (this->memory.find(var) == this->memory.end()) {// ��ǰ���������ڴ��У�������¿ռ�
		this->memory[var] = this->frame_size;
		this->frame_size += 4;
	}
	this->AValue[var].insert("Mem"); // ���¸ñ�����AValue

	// ���ɴ洢����
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
		// �������Ƿ��Ѿ����ڴ���
		for (const auto& pos : this->AValue[var]) {
			if (pos == "Mem") {
				in_mem = true;
				break;
			}
			else {
				reg = pos;
			}
		}
		// ��������ڴ浫�ڼĴ����У���洢���ڴ�
		if (!in_mem && !reg.empty()) {
			this->storeVarToMem(var, reg, codes);
		}
	}
}

std::string RegManager::allocFreeReg(const std::vector<Quadruple>& qList, int cur_q_idx, const std::set<std::string>& out_set, std::vector<std::string>& codes)
{
	// ���п��мĴ�����ֱ�ӷ���
	if (!this->free_regs.empty()) {
		std::string free_reg = this->free_regs.back();
		this->free_regs.pop_back();
		return free_reg;
	}
	// �޿��мĴ�����Ѱ����Զ�Żᱻʹ�õı������ڼĴ���
	double farest_usepos = std::numeric_limits<double>::lowest();
	std::string selected_reg;
	for (const auto& [reg, vars] : this->RValue) {
		double cur_usepos = std::numeric_limits<double>::infinity();

		for (const auto& var : vars) {
			if (this->AValue[var].size() > 1) {
				// �ñ������ڶദ��������ֱ������
				continue;
			}

			// �ñ�������һ�������ڵ�ǰ�Ĵ�������Ѱ�Ҹñ�������Ԫʽ�����ʹ�õ�λ��
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
		//������Զʹ��λ�õļĴ���
		if (cur_usepos == std::numeric_limits<double>::infinity()) {// ��ֹfarest_usepos����ֵΪ+inf
			selected_reg = reg;
			break;
		}
		else if (cur_usepos > farest_usepos) {
			farest_usepos = cur_usepos;
			selected_reg = reg;
		}
	}

	// ����ԭ�������ѡ�мĴ����ı���
	for (const auto& var : this->RValue[selected_reg]) {
		this->AValue[var].erase(selected_reg);
		// �ж��Ƿ�Ҫ���浽�ڴ�
		if (this->AValue[var].empty()) {// ������������
			bool need_store = false;
			// ��������Ԫʽ�����Ƿ񻹻�ʹ��
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
			// ���ڵ�ǰ�������ں���δʹ�ã��������ڻ�����ĳ��ڻ�Ծ������Ҳ��Ҫ����
			if (need_store == false) {
				if (out_set.find(var) != out_set.end()) {
					need_store = true;
				}
			}

			// ����Ҫ���棬�����
			if (need_store) {
				this->storeVarToMem(var, selected_reg, codes);
			}
		}
	}

	// �ͷ�ѡ�еļĴ���
	this->RValue[selected_reg].clear();
	return selected_reg;
}

std::string RegManager::getArgReg(const std::string& arg, const std::vector<Quadruple>& qList, int cur_q_idx, const std::set<std::string>& out_set, std::vector<std::string>& codes)
{
	std::string reg;
	// �ȼ��AValue���Ƿ��Ѿ��и���
	for (const auto& pos : this->AValue[arg]) {
		if (pos != "Mem") {
			return pos;
		}
	}
	
	// û��������������ֱ��ʹ�ã�����Ҫ�������һ���Ĵ���
	reg = this->allocFreeReg(qList, cur_q_idx, out_set, codes);

	// ���ɴ�ȡ����
	if (this->is_variable(arg)) { // �ֲ��������м����
		if (this->func_vars.count(arg) || arg[0] == 'T') {
			std::string code = "lw " + reg + ", " + std::to_string(this->memory[arg]) + "($sp)";
			codes.push_back(code);
		}
		else if (this->data_vars.count(arg)) {
			std::string code = "lw " + reg + ", " + arg;
			codes.push_back(code);
		}
	}
	else { // ������
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

	// �ж��ܷ���arg1�ļĴ�����ȷ��arg1�Ǳ������Ҳ���ռȫ�ֱ����ļĴ�����
	if (this->is_variable(arg1)) {
		if (this->data_vars.find(arg1) != this->data_vars.end() &&
			this->func_vars.find(arg1) == this->func_vars.end()) {
			for (const auto& pos : this->AValue[arg1]) {
				if (pos != "Mem" && this->RValue[pos].size() == 1) {
					if (qInfo.arg1_info.getIsActive() == false) {
						// ����RValue��AValue��ϵ
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

	// ���·���Ĵ���
	std::string reg = this->allocFreeReg(qList, cur_q_idx, out_set, codes);
	this->RValue[reg].insert(result);
	this->AValue[result].insert(reg);

	return reg;
}

void RegManager::allocStackFrame(const std::string& func_name, const std::set<std::string>& vars)
{
	// ����ջ֡��ʼ��С(ra�;�sp��ռ4�ֽ�)
	this->frame_size = (func_name != "main") ? 2 * 4 : 0;
	// ����ڴ�״̬
	this->resetMemory();
	// Ϊ���оֲ�����Ԥ���ռ䣬����û�о���ֵ�ݲ����ɴ���ָ��
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
	// ��procTable����ȡfunc_vars
	for (const auto& proc : procTable) {
		// ����ÿ�����������Ӧ�ľֲ������ļ�ֵ�ԣ���ʼ��func_vars
		std::set<std::string> local_vars; // ��ǰ�����ľֲ���������
		
		// ��ȡ��ǰ�����ķ��ű�
		SymbolTable* symboltbl = proc.symbolTable;	

		// �ҵ���Ӧ�ĺ��������ȡ�ú������βκ;ֲ�����
		if (symboltbl != nullptr) {
			for (const auto& entry : symboltbl->table) {
				if (entry.kind == SymbolKind::Variable) {
					local_vars.insert(entry.ID);
				}
			}
		}
		
		// ����ǰ�����ľֲ�������Ϣ����func_vars
		this->func_vars[proc.ID] = local_vars;
	}

	// �涨rust�﷨�ϲ��������ȫ�ֱ�����data_varsӦΪ��
	this->data_vars.clear();
	
	// ͬ����ʼ��RegMgr��ȫ�ֱ�����Ϣ
	this->regMgr.setDataVars(data_vars);
}

void CodeGenerator::genFuncObjCode(const std::string& func_name, const std::vector<Block*>& blocks)
{
	// 1. ����regMgr״̬
	this->regMgr.resetMemory();
	this->regMgr.setFuncVars(this->func_vars[func_name]);

	// 2. ����ÿ��������
	for (size_t i = 0; i < blocks.size(); ++i) {
		Block* blk = blocks[i];
		std::string label = blk->getName() + ":";
		this->codes.push_back(label);

		// 3. ����Ǻ����ĵ�һ�������飬��Ҫ����������
		if (i == 0) {
			// ��main�������淵�ص�ַ
			if (func_name != "main"){
				this->codes.push_back("sw $ra, 4($sp)");
			}
			// ����ջ֡�ռ�
			this->regMgr.allocStackFrame(func_name, this->func_vars[func_name]);
		}

		// 4. ���ɻ������ڲ�����
		this->genBlkObjCode(blk, func_name);
	}
}

void CodeGenerator::genBlkObjCode(Block* block, const std::string& func_name)
{
	// 1. ��ʼ���Ĵ���״̬�ͺ����β��б�
	this->regMgr.freeAllRegs(block->getInSet());
	this->param_list.clear();

	// 2. ����������е�ÿ����Ԫʽ
	const auto& codes = block->getCodes();
	const auto& codesInfo = block->getCodesInfo();
	for (size_t i = 0; i < codes.size(); ++i) {
		const auto& q = codes[i];
		const auto& qInfo = codesInfo[i];

		if (i == codes.size() - 1) { // ���һ�������������⴦��:���ڻ�Ծ������δ��ֵ��ȫ�ֱ����豣�浽�ڴ�
			auto outset = block->getOutSet();
			outset.insert(this->data_vars.begin(), this->data_vars.end());

			if (q.op == "j" || q.op == "inz" || q.op == "ret") {
				this->regMgr.storeOutSetToMem(outset, this->codes);
				this->genQuarObjCode(q, qInfo, i, block, func_name);
			}
			else if (q.op == "call") {
				// ��������ǰ�����Ŀ�������ĳ��ڻ�Ծ����
				auto tmp_outset = outset;
				tmp_outset.erase(q.result);
				this->regMgr.storeOutSetToMem(tmp_outset, this->codes);

				this->genQuarObjCode(q, qInfo, i, block, func_name);

				// ����Ŀ�����
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
		else { // ��ͨ������
			this->genQuarObjCode(q, qInfo, i, block, func_name);
		}
	}
}

void CodeGenerator::genQuarObjCode(const Quadruple& q, const QuadrupleInfo& qInfo, int q_idx, Block* block, const std::string& func_name)
{
	if (!this->GetGenErrors().empty())// �д���������
		return;

	if (q.op == "=") {
		std::string reg = this->regMgr.getArgReg(q.arg1, block->getCodes(), q_idx, block->getOutSet(), this->codes);
		this->regMgr.freeVarRegs(q.result);
		// ����RValue��AValue
		this->regMgr.storeVarToReg(q.result, reg);
	}
	else if (q.op == "ret") {
		// ������1�Ƿ�Ϊ����
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

		// �Ƿ�Ϊmain����
		if (func_name == "main") {// main����ֱ�ӽ���ȫ��������ת��end
			this->codes.push_back("j end");
		}
		else {
			this->codes.push_back("lw $ra, 4($sp)"); // ��main������Ҫȡ�ص�ַ��ra����ת
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
		// �洢��sp����spָ����λ��
		std::string code = "sw $sp, " + std::to_string(this->regMgr.getFrameSize()) + "($sp)";
		this->codes.push_back(code);
		code = "addi $sp, $sp, " + std::to_string(this->regMgr.getFrameSize());
		this->codes.push_back(code);

		// ת��
		code = "jal " + q.arg1;
		this->codes.push_back(code);

		// ���أ��ָ���sp
		this->codes.push_back("lw $sp, 0($sp)");

		// $v0�еķ���ֵ��д����Ӧ����ʱ����
		if (q.result != "-") {
			std::string rd = this->regMgr.getResultReg(q.result, block->getCodes(), block->getCodesInfo(), q_idx, block->getOutSet(), this->codes);
			code = "add " + rd + ", $v0, $zero";
			this->codes.push_back(code);
		}
	}
	else if (q.op == "+" || q.op == "-" || q.op == "*" || q.op == "/" || q.op == "%") {// ���������
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
			code = "mflo " + rd;	// ��ȡ��
			this->codes.push_back(code);
		}
		else if (q.op == "%") {
			code = "div " + rs + ", " + rt;
			this->codes.push_back(code);
			code = "mfhi " + rd;	// ��ȡ����
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
	else if (q.op == "==" || q.op == "!=" || q.op == "<" || q.op == "<=" || q.op == ">" || q.op == ">=") {// �Ƚ������
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
	else if (q.op == "j") {// ��������ת
		std::string code = "j " + q.result;
		this->codes.push_back(code);
	}
	else if (q.op == "j==" || q.op == "j!=" || q.op == "j<" || q.op == "j<=" || q.op == "j>" || q.op == "j>=") {//������ת
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
		// ������
		std::string code = "lw " + result_reg + ", 0(" + arg1_reg + ")";
		this->codes.push_back(code);
	}
	else {
		std::string error = "genError:Ŀ����������ݲ�֧�ֵĸ�������(" + q.op + ")";
		this->genErrors.push_back(error);
	}
	
}

std::vector<std::string> CodeGenerator::genObjCode()
{
	// 1. �������ݶ�
	this->codes.push_back(".data");
	for (const auto& var : this->data_vars) {
		std::string code = var + ": .word 0";
		this->codes.push_back(code);
	}
	this->codes.push_back("");

	// 2. ���ɴ����
	this->codes.push_back(".text");
	this->codes.push_back("lui $sp, 0x1004"); // ջָ���ʼֵ
	this->codes.push_back("j main");	// Ĭ����ִ��main����

	// 3. Ϊÿ���������ɴ���
	for (const auto& [func, blocks] : this->func_blocks) {
		this->genFuncObjCode(func, blocks);
	}
	this->codes.push_back("end:");

	// 4. ��������������
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