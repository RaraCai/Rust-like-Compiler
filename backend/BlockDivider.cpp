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
            continue;   // 无条件跳转语句不参与分析
        }
        // 分析剩余四元式
        if (q.op != "call") {
            // 源操作数arg1、arg2
            if (this->is_variable(q.arg1) && this->def_set.find(q.arg1) == this->def_set.end()) {
                this->use_set.insert(q.arg1);
            }
            if (this->is_variable(q.arg2) && this->def_set.find(q.arg2) == this->def_set.end()) {
                this->use_set.insert(q.arg2);
            }
        }
        if (q.op[0] != 'j') {
            // 目标操作数
            if (this->is_variable(q.result) && this->use_set.find(q.result) == this->use_set.end()) {
                this->def_set.insert(q.result);
            }
        }
    }
    // 计算完毕，使用USE集初始化IN集
    this->in_set = this->use_set;
}

bool Block::computeInOutSet()
{
    // 保存IN、OUT集原大小以检测变化
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

    // 检查更新
    if (this->in_set.size() != prev_in_size || this->out_set.size() != prev_out_size) 
        return true;
    else 
        return false;
}

void Block::computeSymbolInfo(std::unordered_map<std::string, SymbolInfo>& initSymbolInfoTable)
{
    this->symbolInfoTable = initSymbolInfoTable;

    // 从后向前扫描四元式
    int idx = this->codes.size();
    for (auto it = this->codes.rbegin(); it != this->codes.rend(); ++it) {
        --idx;
        const auto& q = *it;
        QuadrupleInfo qInfo;// 当前四元式的变量活跃信息

        if (q.op == "j" || q.op == "call") {
            this->codesInfo[idx] = qInfo;// 直接保存空活跃信息，无额外操作
            continue;
        }
        // 处理源操作数
        if (this->is_variable(q.arg1)) {
            qInfo.arg1_info = this->symbolInfoTable[q.arg1];
            this->symbolInfoTable[q.arg1] = SymbolInfo(idx, true);
        }
        if (this->is_variable(q.arg2)) {
            qInfo.arg2_info = this->symbolInfoTable[q.arg2];
            this->symbolInfoTable[q.arg2] = SymbolInfo(idx, true);
        }

        // 处理目的操作数
        if (q.op[0] != 'j') {// 非跳转类指令
            if (this->is_variable(q.result)) {
                qInfo.result_info = this->symbolInfoTable[q.result];
                this->symbolInfoTable[q.result] = SymbolInfo(-1, false);
            }
        }
        // 更新四元式对应的变量活跃信息
        this->codesInfo[idx] = qInfo;
    }
}

BlockDivider::BlockDivider(const std::vector<Quadruple>& qList) : start_address(100), block_cnt(0) 
{
	this->qList = qList;
}

BlockDivider::~BlockDivider() 
{
    // 清理内存
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
    // 按照函数块划分基本块，遍历函数表的每一项
    for (size_t i = 0; i < procTable.size(); ++i) {
        const auto& proc = procTable[i];
        std::vector<int> block_enter;   // 当前函数的所有入口语句

        /*  第一步：找到所有入口语句的位置并记录  */
        int func_start = proc.addr - this->start_address;
        int func_end;
        if (i == procTable.size() - 1) {// 最后一个函数到四元式表末尾
            func_end = this->qList.size();  
        } else { // 否则到下一个函数的起始位置
            func_end = procTable[i + 1].addr - this->start_address;
        }
        // 函数块的第一个语句是入口语句
        block_enter.push_back(func_start); 
        // 遍历当前函数块的四元式寻找其他入口语句
        for (int j = func_start; j < func_end; ++j) {
            const auto& q = this->qList[j];

            if (q.op[0] =='j') {// 跳转语句
                if (q.op == "j") {// 无条件跳转，转移到的语句是入口语句
                    if (std::stoi(q.result) != 0)
                        block_enter.push_back(std::stoi(q.result) - this->start_address);
                } else {// 条件跳转（下一条/转移到的语句是入口语句）
                    if (j < func_end - 1)
                        block_enter.push_back(j + 1);
                    block_enter.push_back(std::stoi(q.result) - this->start_address);
                }
            }
            else if (q.op == "call" && j < func_end - 1) {
                // 函数调用后的下一句是入口语句
                block_enter.push_back(j + 1);
            }
        }
        // 对所有入口语句位置排序去重
        std::sort(block_enter.begin(), block_enter.end());
        auto last = std::unique(block_enter.begin(), block_enter.end());
        block_enter.erase(last, block_enter.end());


        /*  第二步：创建基本块 */
        this->func_blocks[proc.ID] = std::vector<Block*>();

        for (size_t j = 0; j < block_enter.size(); ++j) {
            Block* blk = new Block();

            int blk_start = block_enter[j];
            int blk_end = (j < block_enter.size() - 1) ? block_enter[j + 1] : func_end;

            // 设置块名和起始地址
            blk->setName(j == 0 ? proc.ID : this->getBlockName());
            blk->setStartAddr(blk_start);

            // 压入每个基本块的四元式（介于两个基本块起始地址之间 / 到一个转移语句停止)
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


        /*  第三步：建立基本块之间的转移关系    */
        auto& blks = this->func_blocks[proc.ID];

        for (size_t j = 0; j < blks.size(); ++j) {
            Block* blk = blks[j];
            Block* next_blk = (j < blks.size() - 1) ? blks[j + 1] : nullptr;

            if (blk->getCodes().empty())    // 基本块为空无需后续操作
                continue;

            const auto& lastq = blk->getLastCode();
            if (lastq.op[0] == 'j') {
                int dest_addr = std::stoi(lastq.result) - this->start_address;
                // 查找目标基本块
                Block* dest_blk = nullptr;
                for (auto* b : this->blocks) {
                    if (b->getStartAddr() == dest_addr) {
                        dest_blk = b;
                        break;
                    }
                }

                if (lastq.op == "j") { // 无条件跳转
                    blk->setNext(dest_blk, nullptr);
                }
                else {// 条件跳转
                    blk->setNext(next_blk, dest_blk);
                }

                // 四元式中的跳转目标更新为基本块名称
                const_cast<Quadruple&>(lastq).result = dest_blk->getName();
            }
            else if (lastq.op == "call") {
                int dest_addr = std::stoi(lastq.arg1) - this->start_address;

                // 查找转子(arg1)所在基本块
                Block* dest_blk = nullptr;
                for (auto* b : this->blocks) {
                    if (b->getStartAddr() == dest_addr) {
                        dest_blk = b;
                        break;
                    }
                }
                // 下一个基本块应为转子所在基本块
                blk->setNext(dest_blk, nullptr);
                // 四元式中的转子地址(arg1)更新为基本块名称
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
        /* 第一步：计算USE / DEF集合 */ 
        for (auto* blk : blocks) {
            blk->computeUseDefSets();
        }

        /* 第二步：计算IN / OUT集合 */
        bool changed; // 维护更新标志
        do {
            changed = false;
            for (auto* blk : blocks) {
                if (blk->computeInOutSet()) {
                    changed = true;
                }
            }
        } while (changed);

        /* 第三步：计算变量活跃和待用信息 */
        // 为每个基本块创建初始symbolInfoTable（基于出口活跃变量OUT集）
        std::unordered_map<Block*, std::unordered_map<std::string, SymbolInfo>>blockSymbolInfoTable;
        for (auto* blk : blocks) {
            std::unordered_map<std::string, SymbolInfo> symbolTable;
            const auto& out_set = blk->getOutSet();// 获取该基本块的出口活跃变量
            for (const auto& var : out_set) {
                symbolTable[var] = SymbolInfo(-1, true);
            }
            blockSymbolInfoTable[blk] = symbolTable;
        }

        // 每个基本块计算其内部的变量活跃信息
        for (auto* blk : blocks) {
            blk->computeSymbolInfo(blockSymbolInfoTable[blk]);
        }
    }
}

void BlockDivider::BlockDivision(const ProcedureTable& procTable)
{
    // 四元式划分进基本块
    this->divideBlocks(procTable);
    // 数据流分析，计算基本块变量待用活跃信息
    this->computeBlocks();
    // 写入日志文件
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

    // 打印基本块中的四元式及其活跃信息
    const auto& codes = this->codes;
    std::cout << "  Quadruples (" << codes.size() << "):" << std::endl;

    for (size_t i = 0; i < codes.size(); ++i) {
        const Quadruple& q = codes[i];
        const QuadrupleInfo& info = codesInfo[i];

        // 打印四元式
        std::cout << "    [" << i << "]: "
           <<"( " << q.op << " , " << q.arg1 << " , "
            << q.arg2 << " , " << q.result<<" )";

        // 打印活跃信息
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

    // 遍历所有函数及其对应的基本块列表
    for (const auto& pair : func_blocks) {
        const std::string& funcName = pair.first;
        const std::vector<Block*>& blocks = pair.second;

        std::cout << "\nFunction: " << funcName << std::endl;
        std::cout << "----------------------------------------" << std::endl;

        // 遍历该函数下的所有基本块
        for (Block* block : blocks) {
            if (!block) continue;

            std::cout << "\nBlock: " << block->getName()
                << " (Start Address: " << block->getStartAddr() << ")" << std::endl;
            std::cout << "  Transitions: Next1="
                << (block->getNext(1) ? block->getNext(1)->getName() : "null")
                << ", Next2="
                << (block->getNext(2) ? block->getNext(2)->getName() : "null") << std::endl;

            // 打印基本块中的四元式
            const auto& codes = block->getCodes();
            std::cout << "  Quadruples (" << codes.size() << "):" << std::endl;

            for (size_t i = 0; i < codes.size(); ++i) {
                const Quadruple& q = codes[i];
                std::cout << "    [" << i << "]: "
                    << "( " << q.op << " , " << q.arg1 << " , "
                    << q.arg2 << " , " << q.result << " )" << endl;
            }

            // 打印数据流分析信息
            block->printBlockInfo();

        }
    }

    std::cout << "\n============================================\n" << std::endl;
}

