#include "BlockDivider.h"
#include<cctype>
#include<fstream>

using namespace std;

SymbolInfo::SymbolInfo(int use, bool active) :next_use(use), is_active(active) 
{
}

int SymbolInfo::getNextUse() const 
{
    return this->next_use;
}

bool SymbolInfo::getIsActive() const
{
    return this->is_active;
}

void SymbolInfo::setNextUse(int use)
{
    this->next_use = use;
}

void SymbolInfo::setIsActive(bool active)
{
    this->is_active = active;
}

std::string SymbolInfo::toString() const
{
    return "<" + (this->next_use == -1 ? "None" : std::to_string(this->next_use)) + ", " + (this->is_active ? "true" : "false") + ">";
}

Block::Block() : start_addr(0), next1(nullptr), next2(nullptr)
{
}

std::string Block::getName()const
{
    return this->name;
}

int Block::getStartAddr()const
{
    return this->start_addr;
}

const std::vector<Quadruple>& Block::getCodes()const
{
    return this->codes;
}

const std::vector<QuadrupleInfo>& Block::getCodesInfo() const
{
    return this->codesInfo;
}

void Block::setName(const std::string& n)
{
    this->name = n;
    return;
}

void Block::setStartAddr(int addr)
{
    this->start_addr = addr;
    return;
}

void Block::addCode(const Quadruple& code)
{
    this->codes.push_back(code);
    this->codesInfo.emplace_back();
    return;
}

const Quadruple& Block::getLastCode() const
{
    return this->codes.back();
}

void Block::setNext(Block* next1, Block* next2)
{
    this->next1 = next1;
    this->next2 = next2;
}

const Block* Block::getNext(int n)
{
    return (n == 1) ? this->next1 : (n == 2) ? this->next2 : nullptr;
}

const std::set<std::string>& Block::getOutSet() const
{
    return this->out_set;
}

const std::set<std::string>& Block::getInSet() const
{
    return this->in_set;
}

bool Block::is_variable(const std::string name) const
{
    if (name.empty()) 
        return false;
    else if (std::isalpha(name[0])) 
        return true;
    else if (name[0] == '-' && name.length() > 1) 
        return true;
    return false;
}

void Block::computeUseDefSets()
{
    this->use_set.clear();
    this->def_set.clear();

    for (const auto& q : this->getCodes()) {
        if (q.op == "j") {
            continue;   // ��������ת��䲻�������
        }
        // ����ʣ����Ԫʽ
        if (q.op != "call") {
            // Դ������arg1��arg2
            if (this->is_variable(q.arg1) && this->def_set.find(q.arg1) == this->def_set.end()) {
                this->use_set.insert(q.arg1);
            }
            if (this->is_variable(q.arg2) && this->def_set.find(q.arg2) == this->def_set.end()) {
                this->use_set.insert(q.arg2);
            }
        }
        if (q.op[0] != 'j') {
            // Ŀ�������
            if (this->is_variable(q.result) && this->use_set.find(q.result) == this->use_set.end()) {
                this->def_set.insert(q.result);
            }
        }
    }
    // ������ϣ�ʹ��USE����ʼ��IN��
    this->in_set = this->use_set;
}

bool Block::computeInOutSet()
{
    // ����IN��OUT��ԭ��С�Լ��仯
    size_t prev_in_size = this->in_set.size();
    size_t prev_out_size = this->out_set.size();

    Block* next1 = this->next1;
    Block* next2 = this->next2;

    if (next1) {
        for (const auto& var : next1->in_set) {
            this->out_set.insert(var);
            if (this->def_set.find(var) == this->def_set.end()) {
                this->in_set.insert(var);
            }
        }
    }

    if (next2) {
        for (const auto& var : next2->in_set) {
            this->out_set.insert(var);
            if (this->def_set.find(var) == this->def_set.end()) {
                this->in_set.insert(var);
            }
        }
    }

    // ������
    if (this->in_set.size() != prev_in_size || this->out_set.size() != prev_out_size) 
        return true;
    else 
        return false;
}

void Block::computeSymbolInfo(std::unordered_map<std::string, SymbolInfo>& initSymbolInfoTable)
{
    this->symbolInfoTable = initSymbolInfoTable;

    // �Ӻ���ǰɨ����Ԫʽ
    int idx = this->codes.size();
    for (auto it = this->codes.rbegin(); it != this->codes.rend(); ++it) {
        --idx;
        const auto& q = *it;
        QuadrupleInfo qInfo;// ��ǰ��Ԫʽ�ı�����Ծ��Ϣ

        if (q.op == "j" || q.op == "call") {
            this->codesInfo[idx] = qInfo;// ֱ�ӱ���ջ�Ծ��Ϣ���޶������
            continue;
        }
        // ����Դ������
        if (this->is_variable(q.arg1)) {
            qInfo.arg1_info = this->symbolInfoTable[q.arg1];
            this->symbolInfoTable[q.arg1] = SymbolInfo(idx, true);
        }
        if (this->is_variable(q.arg2)) {
            qInfo.arg2_info = this->symbolInfoTable[q.arg2];
            this->symbolInfoTable[q.arg2] = SymbolInfo(idx, true);
        }

        // ����Ŀ�Ĳ�����
        if (q.op[0] != 'j') {// ����ת��ָ��
            if (this->is_variable(q.result)) {
                qInfo.result_info = this->symbolInfoTable[q.result];
                this->symbolInfoTable[q.result] = SymbolInfo(-1, false);
            }
        }
        // ������Ԫʽ��Ӧ�ı�����Ծ��Ϣ
        this->codesInfo[idx] = qInfo;
    }
}

BlockDivider::BlockDivider(const std::vector<Quadruple>& qList) : start_address(100), block_cnt(0) 
{
	this->qList = qList;
}

BlockDivider::~BlockDivider() 
{
    // �����ڴ�
    for (auto& pair : func_blocks) {
        for (Block* block : pair.second) {
            delete block;
        }
    }
}

std::string BlockDivider::getBlockName()
{
    return "block" + std::to_string(++this->block_cnt);
}

std::unordered_map<std::string, std::vector<Block*>>& BlockDivider::getFuncBlocks()
{
    return this->func_blocks;
}

void BlockDivider::divideBlocks(const ProcedureTable& procTable)
{
    // ���պ����黮�ֻ����飬�����������ÿһ��
    for (size_t i = 0; i < procTable.size(); ++i) {
        const auto& proc = procTable[i];
        std::vector<int> block_enter;   // ��ǰ����������������

        /*  ��һ�����ҵ������������λ�ò���¼  */
        int func_start = proc.addr - this->start_address;
        int func_end;
        if (i == procTable.size() - 1) {// ���һ����������Ԫʽ��ĩβ
            func_end = this->qList.size();  
        } else { // ������һ����������ʼλ��
            func_end = procTable[i + 1].addr - this->start_address;
        }
        // ������ĵ�һ�������������
        block_enter.push_back(func_start); 
        // ������ǰ���������ԪʽѰ������������
        for (int j = func_start; j < func_end; ++j) {
            const auto& q = this->qList[j];

            if (q.op[0] =='j') {// ��ת���
                if (q.op == "j") {// ��������ת��ת�Ƶ��������������
                    if (std::stoi(q.result) != 0)
                        block_enter.push_back(std::stoi(q.result) - this->start_address);
                } else {// ������ת����һ��/ת�Ƶ�������������䣩
                    if (j < func_end - 1)
                        block_enter.push_back(j + 1);
                    block_enter.push_back(std::stoi(q.result) - this->start_address);
                }
            }
            else if (q.op == "call" && j < func_end - 1) {
                // �������ú����һ����������
                block_enter.push_back(j + 1);
            }
        }
        // ������������λ������ȥ��
        std::sort(block_enter.begin(), block_enter.end());
        auto last = std::unique(block_enter.begin(), block_enter.end());
        block_enter.erase(last, block_enter.end());


        /*  �ڶ��������������� */
        this->func_blocks[proc.ID] = std::vector<Block*>();

        for (size_t j = 0; j < block_enter.size(); ++j) {
            Block* blk = new Block();

            int blk_start = block_enter[j];
            int blk_end = (j < block_enter.size() - 1) ? block_enter[j + 1] : func_end;

            // ���ÿ�������ʼ��ַ
            blk->setName(j == 0 ? proc.ID : this->getBlockName());
            blk->setStartAddr(blk_start);

            // ѹ��ÿ�����������Ԫʽ������������������ʼ��ַ֮�� / ��һ��ת�����ֹͣ)
            for (int k = blk_start; k < blk_end; ++k) {
                blk->addCode(this->qList[k]);
                const std::string op = this->qList[k].op;
                if (op[0] == 'j' || op == "call" || op == "ret") {
                    break;
                }
            }

            func_blocks[proc.ID].push_back(blk);
            blocks.push_back(blk);
        }


        /*  ������������������֮���ת�ƹ�ϵ    */
        auto& blks = this->func_blocks[proc.ID];

        for (size_t j = 0; j < blks.size(); ++j) {
            Block* blk = blks[j];
            Block* next_blk = (j < blks.size() - 1) ? blks[j + 1] : nullptr;

            if (blk->getCodes().empty())    // ������Ϊ�������������
                continue;

            const auto& lastq = blk->getLastCode();
            if (lastq.op[0] == 'j') {
                int dest_addr = std::stoi(lastq.result) - this->start_address;
                // ����Ŀ�������
                Block* dest_blk = nullptr;
                for (auto* b : this->blocks) {
                    if (b->getStartAddr() == dest_addr) {
                        dest_blk = b;
                        break;
                    }
                }

                if (lastq.op == "j") { // ��������ת
                    blk->setNext(dest_blk, nullptr);
                }
                else {// ������ת
                    blk->setNext(next_blk, dest_blk);
                }

                // ��Ԫʽ�е���תĿ�����Ϊ����������
                const_cast<Quadruple&>(lastq).result = dest_blk->getName();
            }
            else if (lastq.op == "call") {
                int dest_addr = std::stoi(lastq.arg1) - this->start_address;

                // ����ת��(arg1)���ڻ�����
                Block* dest_blk = nullptr;
                for (auto* b : this->blocks) {
                    if (b->getStartAddr() == dest_addr) {
                        dest_blk = b;
                        break;
                    }
                }
                // ��һ��������ӦΪת�����ڻ�����
                blk->setNext(dest_blk, nullptr);
                // ��Ԫʽ�е�ת�ӵ�ַ(arg1)����Ϊ����������
                const_cast<Quadruple&>(lastq).arg1 = dest_blk->getName();
            }
            else if (lastq.op == "ret") {
                blk->setNext(nullptr, nullptr);
            }
            else {
                blk->setNext(next_blk, nullptr);
            }

        }

    }

}

void BlockDivider::computeBlocks()
{
    for (auto& [func, blocks] : this->func_blocks) {
        /* ��һ��������USE / DEF���� */ 
        for (auto* blk : blocks) {
            blk->computeUseDefSets();
        }

        /* �ڶ���������IN / OUT���� */
        bool changed; // ά�����±�־
        do {
            changed = false;
            for (auto* blk : blocks) {
                if (blk->computeInOutSet()) {
                    changed = true;
                }
            }
        } while (changed);

        /* �����������������Ծ�ʹ�����Ϣ */
        // Ϊÿ�������鴴����ʼsymbolInfoTable�����ڳ��ڻ�Ծ����OUT����
        std::unordered_map<Block*, std::unordered_map<std::string, SymbolInfo>>blockSymbolInfoTable;
        for (auto* blk : blocks) {
            std::unordered_map<std::string, SymbolInfo> symbolTable;
            const auto& out_set = blk->getOutSet();// ��ȡ�û�����ĳ��ڻ�Ծ����
            for (const auto& var : out_set) {
                symbolTable[var] = SymbolInfo(-1, true);
            }
            blockSymbolInfoTable[blk] = symbolTable;
        }

        // ÿ��������������ڲ��ı�����Ծ��Ϣ
        for (auto* blk : blocks) {
            blk->computeSymbolInfo(blockSymbolInfoTable[blk]);
        }
    }
}

void BlockDivider::BlockDivision(const ProcedureTable& procTable)
{
    // ��Ԫʽ���ֽ�������
    this->divideBlocks(procTable);
    // ���������������������������û�Ծ��Ϣ
    this->computeBlocks();
    // д����־�ļ�
    //this->printBlocks();
}

void Block::printBlockInfo()
{
    std::cout << "  Data Flow Analysis:" << std::endl;
    std::cout << "    USE Set: {";
    for (const auto& var : this->use_set) {
        std::cout << " " << var;
    }
    std::cout << " }" << std::endl;

    std::cout << "    DEF Set: {";
    for (const auto& var : this->def_set) {
        std::cout << " " << var;
    }
    std::cout << " }" << std::endl;

    std::cout << "    IN Set:  {";
    for (const auto& var : this->in_set) {
        std::cout << " " << var;
    }
    std::cout << " }" << std::endl;

    std::cout << "    OUT Set: {";
    for (const auto& var : this->out_set) {
        std::cout << " " << var;
    }
    std::cout << " }" << std::endl;

    // ��ӡ�������е���Ԫʽ�����Ծ��Ϣ
    const auto& codes = this->codes;
    std::cout << "  Quadruples (" << codes.size() << "):" << std::endl;

    for (size_t i = 0; i < codes.size(); ++i) {
        const Quadruple& q = codes[i];
        const QuadrupleInfo& info = codesInfo[i];

        // ��ӡ��Ԫʽ
        std::cout << "    [" << i << "]: "
           <<"( " << q.op << " , " << q.arg1 << " , "
            << q.arg2 << " , " << q.result<<" )";

        // ��ӡ��Ծ��Ϣ
        std::cout << "\n      Symbol Info:";
        if (this->is_variable(q.arg1)) {
            std::cout << "\n        arg1: " << info.arg1_info.toString();
        }
        if (this->is_variable(q.arg2)) {
            std::cout << "\n        arg2: " << info.arg2_info.toString();
        }
        if (this->is_variable(q.result)) {
            std::cout << "\n        result: " << info.result_info.toString();
        }
        std::cout << std::endl;
    }

}

void BlockDivider::printBlocks() {
    std::cout << "\n===== Function Blocks Debug Information =====" << std::endl;

    // �������к��������Ӧ�Ļ������б�
    for (const auto& pair : func_blocks) {
        const std::string& funcName = pair.first;
        const std::vector<Block*>& blocks = pair.second;

        std::cout << "\nFunction: " << funcName << std::endl;
        std::cout << "----------------------------------------" << std::endl;

        // �����ú����µ����л�����
        for (Block* block : blocks) {
            if (!block) continue;

            std::cout << "\nBlock: " << block->getName()
                << " (Start Address: " << block->getStartAddr() << ")" << std::endl;
            std::cout << "  Transitions: Next1="
                << (block->getNext(1) ? block->getNext(1)->getName() : "null")
                << ", Next2="
                << (block->getNext(2) ? block->getNext(2)->getName() : "null") << std::endl;

            // ��ӡ�������е���Ԫʽ
            const auto& codes = block->getCodes();
            std::cout << "  Quadruples (" << codes.size() << "):" << std::endl;

            for (size_t i = 0; i < codes.size(); ++i) {
                const Quadruple& q = codes[i];
                std::cout << "    [" << i << "]: "
                    << "( " << q.op << " , " << q.arg1 << " , "
                    << q.arg2 << " , " << q.result << " )" << endl;
            }

            // ��ӡ������������Ϣ
            block->printBlockInfo();

        }
    }

    std::cout << "\n============================================\n" << std::endl;
}

