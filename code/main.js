/**
 * 文件: main.js
 * 描述: 处理用户HTTP请求的控制器，完成功能切换与前端渲染
 * 创建日期: 2025-06-04
 * 
 * 负责解析请求、调用服务层并返回响应，嵌入index.html
 */

/* 页面数据 */
let reduceProductions = null;                   // 维护 reduceProductions 数据(tab3、tab4共用)
let gotoTableData = null;                       // 维护 goto 表数据
let actionTableData = null;                     // 维护 action 表数据
let syntaxErrors = null;                        // 维护语法错误数据
let quadRuples = null;                          // 维护四元式数据
let semanticErrors = null;                      // 维护语义错误数据
let codeString = null;                          // 维护代码字符串
/* 防抖 */
let timeout;                                    // 词法分析防抖计时器


// 初始化编辑器
const editor = CodeMirror(document.getElementById("editor"), {
    lineNumbers: true,
    indentUnit: 4,
    smartIndent: true,
    matchBrackets: true,
    autoCloseBrackets: true,
    value: `//在此输入Rust代码`,
    extraKeys: {
        "Ctrl-Space": "autocomplete",
    },
});

function updateEditorHighlight(tokens) {
    const doc = editor.getDoc();

    // 清除之前的高亮
    doc.getAllMarks().forEach((mark) => mark.clear());

    // 获取所有代码内容
    const allContent = editor.getValue();
    
    // 移除多行注释
    const withoutMultiComments = allContent.replace(/\/\*[\s\S]*?\*\//g, function(match) {
        return match.split('\n').map(() => '').join('\n');
    });
    
    // 移除单行注释 (保持换行符)
    const withoutComments = withoutMultiComments
        .split('\n')
        .map(line => line.replace(/\/\/.*$/, ''))
        .join('\n');

    tokens.forEach((token) => {
        const { column, length, line, type } = token;

        // 忽略无效的 token 和结束标记
        if (line < 1 || column < 1 || length < 1 || type === "End" ) return;

        // 获取实际代码行内容 (注意: line从1开始,需要-1)
        const lineContent = withoutComments.split('\n')[line - 1] || '';

        // 计算当前行的实际缩进宽度
        let actualIndentWidth = 0;
        for (let i = 0; i < lineContent.length; i++) {
            if (lineContent[i] !== ' ') break;
            actualIndentWidth++;
        }

        // 计算 token 的实际起始列数
        let adjustedColumn = 0;
        if (actualIndentWidth) {
            adjustedColumn = actualIndentWidth + (column - 2); // 后端的 column 从 1 开始
        } else {
            adjustedColumn = column - 1;
        }

        // 如果列超出行的长度，跳过该 token
        if (adjustedColumn >= lineContent.length) return;

        // 特殊处理 string_ 类型
        let adjustedLength = length;
        if (type === "string_") {
            adjustedLength += 2; // 加上引号的长度
        }

        try {
            // 获取当前行的注释前缀长度
            let commentPrefixLength = 0;
            const multilineCommentMatch = lineContent.match(/\/\*.*\*\//);
            if (multilineCommentMatch && multilineCommentMatch.index < adjustedColumn) {
                // 如果是单行的多行注释，计算其长度
                commentPrefixLength = multilineCommentMatch[0].length;
            }

            // 特殊处理多行字符串
            if (type === "string_" && token.value && token.value.includes('\n')) {
                const lines = token.value.split('\n');
                const startLine = line - 1;  // CodeMirror的行从0开始
                
                // 处理首行（包含开始引号）
                editor.markText(
                    { line: startLine, ch: adjustedColumn + commentPrefixLength },
                    { line: startLine, ch: lineContent.length },
                    { className: getClassNameForTokenType(type) }
                );

                // 处理中间行（完整行高亮）
                for (let i = 1; i < lines.length - 1; i++) {
                    const currentLine = startLine + i;
                    const currentContent = withoutComments.split('\n')[currentLine] || '';
                    if (currentContent) {
                        editor.markText(
                            { line: currentLine, ch: 0 },
                            { line: currentLine, ch: currentContent.length },
                            { className: getClassNameForTokenType(type) }
                        );
                    }
                }

                // 处理最后一行（包含结束引号）
                const lastLine = startLine + lines.length - 1;
                const lastContent = withoutComments.split('\n')[lastLine] || '';
                const lastLineEndPos = lastContent.indexOf('"') + 1;
                if (lastContent) {
                    editor.markText(
                        { line: lastLine, ch: 0 },
                        { line: lastLine, ch: lastLineEndPos > 0 ? lastLineEndPos : lastContent.length },
                        { className: getClassNameForTokenType(type) }
                    );
                }
            } else {
                // 处理普通token，加上注释前缀的长度
                editor.markText(
                    { line: line - 1, ch: adjustedColumn + commentPrefixLength },
                    { line: line - 1, ch: adjustedColumn + commentPrefixLength + adjustedLength },
                    { className: getClassNameForTokenType(type) }
                );
            }
        } catch (e) {
            console.error(`Highlighting error for token:`, token, e);
        }
    });
}

// 代码编辑区高亮错误
function ErrorHighlight(syntaxErrors) {
    syntaxErrors.forEach((error) => {
        if (error.line === -1 && error.column === -1) return;

        // CodeMirror 的行从 0 开始
        const line = error.line - 1;

        // 获取当前行的内容和长度
        const lineContent = editor.getLine(line);
        if (lineContent === null || lineContent === undefined) return;

        // 计算标记始末位置
        const startPos = { line, ch: 0 };
        const endPos = { line, ch: lineContent.length };

        // 添加红色标记
        editor.markText(startPos, endPos, { className: "error-marker" });
    });
}


function calculateStringLength(content) {
    if (!content) return 0;
    
    // 如果是函数调用的形式 (标识符后面紧跟括号)
    if (/^[a-zA-Z_][a-zA-Z0-9_]*\(/.test(content)) {
        const match = content.match(/^[a-zA-Z_][a-zA-Z0-9_]*\(/);
        if (match) {
            return match[0].length - 1; // 减1是因为括号不算在标识符长度内
        }
    }
    
    // 如果是字符串
    const stringMatch = content.match(/^"([^"\\]|\\.)*"/);
    if (stringMatch) {
        return stringMatch[0].length;
    }
    
    // 其他情况,返回原始长度
    return content.length;
}


function getClassNameForTokenType(type) {
    const typeClassMap = {
        // var-keyword
        I8: "cm-var-keyword",
        U8: "cm-var-keyword",
        I16: "cm-var-keyword",
        U16: "cm-var-keyword",
        I32: "cm-var-keyword",
        U32: "cm-var-keyword",
        I64: "cm-var-keyword",
        U64: "cm-var-keyword",
        I128: "cm-var-keyword",
        U128: "cm-var-keyword",
        F32: "cm-var-keyword",
        F64: "cm-var-keyword",
        ISIZE: "cm-var-keyword",
        USIZE: "cm-var-keyword",
        BOOL: "cm-var-keyword",
        CHAR: "cm-var-keyword",
        UNIT: "cm-var-keyword",
        ARRAY: "cm-var-keyword",
        TRUE: "cm-var-keyword",
        FALSE: "cm-var-keyword",

        // ctrl-keyword
        LET: "cm-ctrl-keyword",
        IF: "cm-ctrl-keyword",
        ELSE: "cm-ctrl-keyword",
        WHILE: "cm-ctrl-keyword",
        RETURN: "cm-ctrl-keyword",
        MUT: "cm-ctrl-keyword",
        FN: "cm-ctrl-keyword",
        FOR: "cm-ctrl-keyword",
        IN: "cm-ctrl-keyword",
        LOOP: "cm-ctrl-keyword",
        BREAK: "cm-ctrl-keyword",
        CONTINUE: "cm-ctrl-keyword",

        // Identifier
        Identifier: "cm-identifier",

        // Constant
        i32_: "cm-constant",
        string_: "cm-string",
        char_: "cm-string",

        // Operator
        Assignment: "cm-operator",
        Addition: "cm-operator",
        Subtraction: "cm-operator",
        Multiplication: "cm-operator",
        Division: "cm-operator",
        Equality: "cm-operator",
        GreaterThan: "cm-operator",
        GreaterOrEqual: "cm-operator",
        LessThan: "cm-operator",
        LessOrEqual: "cm-operator",
        Inequality: "cm-operator",

        // Delimiter
        ParenthesisL: "cm-delimiter",
        ParenthesisR: "cm-delimiter",
        CurlyBraceL: "cm-delimiter",
        CurlyBraceR: "cm-delimiter",
        SquareBracketL: "cm-delimiter",
        SquareBracketR: "cm-delimiter",
        Semicolon: "cm-delimiter",
        Comma: "cm-delimiter",
        Colon: "cm-delimiter",
        ArrowOperator: "cm-delimiter",
        DotOperator: "cm-delimiter",
        RangeOperator: "cm-delimiter",

        // Quote
        DoubleQuote: "cm-quote",
        SingleQuote: "cm-quote",

        // logical-operator: ! % %= & &= && | |= || ?
        Not: "cm-logical-operator",
        Modulo: "cm-logical-operator",
        ModuloAssign: "cm-logical-operator",
        BitAnd: "cm-logical-operator",
        BitAndAssign: "cm-logical-operator",
        LogicalAnd: "cm-logical-operator",
        BitOr: "cm-logical-operator",
        BitOrAssign: "cm-logical-operator",
        LogicalOr: "cm-logical-operator",
        ErrorPropagation: "cm-logical-operator",
        // assign: *= += -= /= << <<= => >> >>= @ ^ ^=
        MultiplicationAssign: "cm-assign",
        AdditionAssign: "cm-assign",
        SubtractionAssign: "cm-assign",
        DivisionAssign: "cm-assign",
        LeftShift: "cm-assign",
        LeftShiftAssign: "cm-assign",
        Arrowmatch: "cm-assign",
        RightShift: "cm-assign",
        RightShiftAssign: "cm-assign",
        PatternBinding: "cm-assign",
        BitXor: "cm-assign",
        BitXorAssign: "cm-assign",

        // reference
        ReferenceMut: "cm-reference",
    };

    return typeClassMap[type] || "cm-default";
}

// 通用函数：获取后端数据
async function fetchData(api) {
    const code = editor.getValue();
    const response = await fetch(`${BASE_URL}/${api}`, {
        method: "POST",
        headers: {
            "Content-Type": "application/json",
        },
        body: JSON.stringify({ code: code }),
    });

    if (!response.ok) {
        throw new Error("Failed to fetch table data");
    }
    return await response.json();
}

// 渲染 goto 和 action 表
function renderTable(tableHeaderId, tableBodyId, data) {
    const tableHeaders = document.getElementById(tableHeaderId);
    const tableBody = document.getElementById(tableBodyId);

    // 清空表头和表体
    tableHeaders.innerHTML = "";
    tableBody.innerHTML = "";

    // 获取所有列
    var columns = [];
    let dataKey = ''; // 存储数据的键名, GOTO表为entries，ACTION表为actions

    if (tableHeaderId === "goto-table-headers") {
        columns = [
            "state",
            ...Object.keys(data[0].entries).filter((column) => column.trim() !== ""),
        ];
        dataKey = 'entries';
    }
    else if (tableHeaderId === "action-table-headers") {
        columns = [
            "state",
            ...Object.keys(data[0].actions).filter((column) => column.trim() !== ""),
        ];
        dataKey = 'actions';
    }

    // 渲染表头
    columns.forEach((column) => {
        const th = document.createElement("th");
        th.textContent = column;
        tableHeaders.appendChild(th);
    });

    // 渲染表体
    data.forEach((row) => {
        const tr = document.createElement("tr");

        // 渲染 state 列
        const stateCell = document.createElement("td");
        stateCell.textContent = row.state;
        tr.appendChild(stateCell);

        // 渲染剩余列
        columns.slice(1).forEach((column) => {
            const td = document.createElement("td");
            // 关键修改：使用正确的数据键名访问数据
            td.textContent = row[dataKey][column] || "-"; // 如果没有值，显示 '-'
            tr.appendChild(td);
        });

        tableBody.appendChild(tr);
    });
}

// 渲染"移进-规约"表
function renderReduceTable(tableHeaderId, tableBodyId, data) {
    const tab3Content = document.getElementById("tab3");
    if (data.length === 0) {
        tab3Content.innerHTML = `
                    <div style="display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100%; text-align: center;">
                        <div class="nodata"></div>
                        <p style="color: #919191; margin-top: 0px; font-size: 14px;">数据暂无</p>
                    </div>
                `;
    } else {
        tab3Content.innerHTML = `
            <div class="data-table-container">
                <table class="data-table">
                    <thead>
                        <tr class="table-headers" id="reduce-table-headers"></tr>
                    </thead>
                    <tbody class="table-body" id="reduce-table-body"></tbody>
                </table>
            </div>
        `;

        const tableHeaders = document.getElementById(tableHeaderId);
        const tableBody = document.getElementById(tableBodyId);

        // 清空表头和表体
        tableHeaders.innerHTML = "";
        tableBody.innerHTML = "";

        // 定义表头
        const columns = ["序号", "产生式左部", "产生式右部", "完整产生式"];

        // 渲染表头
        columns.forEach((column) => {
            const th = document.createElement("th");
            th.textContent = column;
            tableHeaders.appendChild(th);
        });

        // 渲染表体
        data.forEach((row, index) => {
            const tr = document.createElement("tr");

            // 序号列
            const indexCell = document.createElement("td");
            indexCell.textContent = index + 1; // 序号从 1 开始
            tr.appendChild(indexCell);

            // 产生式左部
            const leftCell = document.createElement("td");
            leftCell.textContent = row.left || "-";
            tr.appendChild(leftCell);

            // 产生式右部
            const rightCell = document.createElement("td");
            rightCell.textContent = row.right.length > 0 ? row.right.join(" ") : "-";
            tr.appendChild(rightCell);

            // 完整产生式
            const displayCell = document.createElement("td");
            displayCell.textContent = row.display || "-";
            tr.appendChild(displayCell);

            tableBody.appendChild(tr);
        });
    }
}

// 渲染语法分析树
function renderSyntaxTree(data) {
    const tab4Content = document.getElementById("tab4");
    tab4Content.innerHTML = `<div id="syntax-tree-container"></div>`;

    // 创建iframe并传递数据
    const reduceProductionList = encodeURIComponent(JSON.stringify(data));
    const errorList = encodeURIComponent(JSON.stringify(syntaxErrors));
    const iframe = document.createElement("iframe");
    iframe.src = `tree.html?reduceProduction=${reduceProductionList}&syntaxErrors=${errorList}`;
    iframe.width = "100%";
    iframe.height = "100%";

    const container = document.getElementById("syntax-tree-container");
    container.appendChild(iframe);
}

// 渲染中间代码生成的四元式
function renderQuadRuple() {
    const tab5Content = document.getElementById("tab5");

    if (quadRuples.length === 0){
        tab5Content.innerHTML = `
                    <div style="display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100%; text-align: center;">
                        <div class="nodata"></div>
                        <p style="color: #919191; margin-top: 0px; font-size: 14px;">数据暂无</p>
                    </div>
                `;
        return;
    }

    tab5Content.innerHTML = `
        <div class="data-table-container">
            <table class="data-table">
                <thead>
                    <tr class="table-headers" id="quadruple-table-headers"></tr>
                </thead>
                <tbody class="table-body" id="quadruple-table-body"></tbody>
            </table>
            <button class="button" id="tmp-button">保存中间代码到文件</button>
        </div>
    `;
    const tableHeaders = document.getElementById("quadruple-table-headers");
    const tableBody = document.getElementById("quadruple-table-body");

    // 清空表头和表体
    tableHeaders.innerHTML = "";
    tableBody.innerHTML = "";

    // 定义表头
    const columns = ["地址", "四元式"];

    // 渲染表头
    columns.forEach((column) => {
        const th = document.createElement("th");
        th.textContent = column;
        tableHeaders.appendChild(th);
    });

    quadRuples.forEach((quadruple) => {
        const tr = document.createElement("tr");
                
        // 地址列
        const addressTd = document.createElement("td");
        addressTd.textContent = quadruple.address || "-";
        tr.appendChild(addressTd);
                
        // 四元式列
        const displayTd = document.createElement("td");
        displayTd.textContent = quadruple.display || "-";
        tr.appendChild(displayTd);
                
        tableBody.appendChild(tr);
    });

    // 绑定保存按钮事件
    const saveButton = document.getElementById("tmp-button");
    saveButton.addEventListener("click", () => {
        const blob = new Blob([quadRuples.map(q => `${q.address} ${q.display}`).join('\n')], { type: "text/plain;charset=utf-8" });
        const link = document.createElement("a");
        link.href = URL.createObjectURL(blob);
        link.download = "quadruples.txt";
        link.click();
        URL.revokeObjectURL(link.href); // 释放内存
    });

}


// 渲染错误列表
function renderError(errors,tab,table_headers,table_body,no_semantic) {
    const tabContent = document.getElementById(tab);
    const errorInfo = (tab === 'tab3') ? "代码语法存在错误，查看下方错误列表" : (no_semantic === true) ? "代码语法存在错误，中间代码不可用" :"代码语义存在错误，查看下方错误列表";
    tabContent.innerHTML = `
        <div style="display: flex; flex-direction: column; align-items: center; justify-content: center; height: 25%; text-align: center;">
            <div class="warning"></div>
            <p style="color: #F0605C; margin-top: 0px; font-size: 14px;">${errorInfo}</p>
        </div>
        <div class="data-table-container">
            <table class="data-table">
                <thead>
                    <tr class="table-headers" id=${table_headers}></tr>
                </thead>
                <tbody class="table-body" id=${table_body}></tbody>
            </table>
        </div>
    `;
    if (no_semantic === false){
        const tableHeaders = document.getElementById(table_headers);
        const tableBody = document.getElementById(table_body);

        // 创建表头
        const headers = ["序号", "说明", "位置"];
        let headerHTML = "";
        headers.forEach((header) => {
            headerHTML += `<th style="background-color: #FFF1F1; color: #F0605C;">${header}</th>`;
        });
        tableHeaders.innerHTML = headerHTML;

        // 创建表格行
        let bodyHTML = "";
        errors.forEach((error, index) => {
            const position = `(${error.line},${error.column})`;

            bodyHTML += `
                <tr>
                    <td>${index + 1}</td>
                    <td>${error.message}</td>
                    <td>${position}</td>
                </tr>
            `;
        });
        tableBody.innerHTML = bodyHTML;
    }
}


// 向后端获取语法分析结果
async function getParser() {
    const tab3Content = document.getElementById("tab3");
    const tab4Content = document.getElementById("tab4");
    const tab5Content = document.getElementById("tab5");
    tab3Content.innerHTML = `
        <div class="loading-container">
            <div class="loading"></div>
            <p>语法分析进行中，正在获取"移进-规约"过程...</p>
        </div>
    `;
    tab4Content.innerHTML = `
        <div class="loading-container">
            <div class="loading"></div>
            <p>语法分析进行中，正在获取语法分析树...</p>
        </div>
    `;
    tab5Content.innerHTML = `
        <div class="loading-container">
            <div class="loading"></div>
                <p>语义分析进行中，正在生成中间代码...</p>
            </div>
        `;
        try {
            // 从后端获取语法分析树数据
            const data = await fetchData("parse");
            console.log(data);
            reduceProductions = data.reduceProductions;
            syntaxErrors = data.parseErrors;
            quadRuples = data.quadruples;
            semanticErrors = data.semanticErrors;
        } catch (error) {
            console.error("Error executing Parser:", error);
            tab4Content.innerHTML = tab3Content.innerHTML = `
                        <div style="display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100%; text-align: center;">
                            <div class="nodata"></div>
                            <p style="color: #919191; margin-top: 0px; font-size: 14px;">数据暂无</p>
                        </div>
                    `;
            return;
        }
    // 语法分析部分
    if (syntaxErrors && syntaxErrors.length > 0) {
        // 渲染语法错误
        renderError(syntaxErrors,'tab3',"syntax-error-header","syntax-error-body", false);
        renderSyntaxTree([]);
        ErrorHighlight(syntaxErrors);
        // 语法错误了语义一定错误
        renderError(semanticErrors,'tab5',"semantic-error-header","semantic-error-body", true);
        return;
    }
    // 正确结果渲染到前端
    if (reduceProductions && reduceProductions.length > 0) {
        renderReduceTable("reduce-table-headers","reduce-table-body",reduceProductions);
        renderSyntaxTree(reduceProductions);
    }
    // 语义分析部分
    if (semanticErrors && semanticErrors.length >0 ){
        
        // 渲染语义分析错误
        renderError(semanticErrors,'tab5',"semantic-error-header","semantic-error-body", false);
        ErrorHighlight(semanticErrors);
    }
    else{
        // 渲染四元式
        renderQuadRuple();
    }
}

// 向后端获取目标代码生成结果
async function getGenerator() {
    const tab6Content = document.getElementById("tab6");
    tab6Content.innerHTML = `
        <div class="loading-container">
            <div class="loading"></div>
            <p>目标代码生成中，正在获取目标代码...</p>
        </div>
    `;
    try {
        // 从后端获取目标代码
        const response = await fetch(`${BASE_URL}/generate`, {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify({ code: editor.getValue() }),
        });
        if (!response.ok) {
            throw new Error("请求失败");
        }
        const data = await response.json();
        if(data.parseErrors.length > 0 || data.semanticErrors.length > 0){
            
            tab6Content.innerHTML = `
                <div style="display: flex; flex-direction: column; align-items: center; justify-content: center; height: 25%; text-align: center;">
                    <div class="warning"></div>
                    <p style="color: #F0605C; margin-top: 0px; font-size: 14px;">因语法或语义存在错误，无法生成目标代码</p>
                </div>
            `;
        }else{
            if(data.objErrors.length > 0){
            tab6Content.innerHTML = `
                <div style="display: flex; flex-direction: column; align-items: center; justify-content: center; height: 25%; text-align: center;">
                    <div class="warning"></div>
                    <p style="color: #F0605C; margin-top: 0px; font-size: 14px;">目标代码生成失败，查看下方错误列表</p>
                </div>
                <div class="data-table-container">
                    <table class="data-table">
                        <thead>
                            <tr class="table-headers" id="obj-headers"></tr>
                        </thead>
                        <tbody class="table-body" id="obj-body"></tbody>
                    </table>
                </div>
            `;
            const tableHeaders = document.getElementById("obj-headers");
            const tableBody = document.getElementById("obj-body");
            // 创建表头
            const headers = ["序号", "错误信息"];
            let headerHTML = "";
            headers.forEach((header) => {
                headerHTML += `<th style="background-color: #FFF1F1; color: #F0605C;">${header}</th>`;
            });
            tableHeaders.innerHTML = headerHTML; 
            // 创建表格行
            let bodyHTML = "";
            errors.forEach((error, index) => {
                bodyHTML += `
                    <tr>
                        <td>${index + 1}</td>
                        <td>${error.message}</td>
                    </tr>
                `;
            });
            tableBody.innerHTML = bodyHTML;
        }
        else{
            tab6Content.innerHTML = `
                <div id="code"></div>
                <button class="button" id="obj-button">保存目标代码到文件</button>
            `;
            
            codeString = data.objCode.map(item => item.code).join('\n');
            
            // 绑定保存按钮事件
            const saveButton = document.getElementById("obj-button");
            saveButton.addEventListener("click", () => {
                const blob = new Blob([codeString], { type: "text/plain;charset=utf-8" });
                const link = document.createElement("a");
                link.href = URL.createObjectURL(blob);
                link.download = "code.asm";
                link.click();
                URL.revokeObjectURL(link.href); // 释放内存
            });

            // 目标代码展示
            const code= CodeMirror(document.getElementById("code"), {
                lineNumbers: true,
                mode: "text/x-asm",
                readOnly: true,
                indentUnit: 4,
                smartIndent: true,
                matchBrackets: true,
                autoCloseBrackets: true,
                value: `$目标代码将在此显示\n$点击下方按钮保存为code.asm文件`,
                extraKeys: {
                    "Ctrl-Space": "autocomplete",
                },
            });
            code.setValue(codeString);
        }          
    }
    } catch (error) {
        console.error("Error executing Generator:", error);
        tab6Content.innerHTML = `
        <div style="display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100%; text-align: center;">
            <div class="nodata"></div>
                <p style="color: #919191; margin-top: 0px; font-size: 14px;">数据暂无</p>
            </div>
        `;
        return;     
    }
}

/* 监听编辑器变化 */
// 词法高亮
editor.on("change", () => {
    clearTimeout(timeout);
    timeout = setTimeout(async () => {
        const code = editor.getValue();
        try {
            const response = await fetch(`${BASE_URL}/analyze`, {
                method: "POST",
                headers: {
                    "Content-Type": "application/json",
                },
                body: JSON.stringify({ code: code }),
            });
            if (!response.ok) {
                throw new Error("请求失败");
            }
            const data = await response.json();
            updateEditorHighlight(data.tokens);
            getParser();
        } catch (error) {
            console.error("分析失败:", error);
        }
    }, 100); // 100毫秒的防抖时间
});


/* tab切换控制 */
// 页面挂载时自动获取goto表，动态监测TAB
document.addEventListener("DOMContentLoaded", async () => {
    const tabs = document.querySelectorAll(".tab");
    const tabContents = document.querySelectorAll(".tab-content");

    // 页面挂载时直接加载语法规则
    const gotoTabContent = document.getElementById("tab1");
    const actionTabContent = document.getElementById("tab2");
    gotoTabContent.innerHTML = `
                <div class="loading-container">
                    <div class="loading"></div>
                    <p>语法规则解析中，GOTO表正在加载...</p>
                </div>
            `;

    try {
        // 获取语法规则
        const data = await fetchData("grammar");
        // 缓存页面数据
        gotoTableData = data.gototable;
        actionTableData = data.actiontable;

        gotoTabContent.innerHTML = `
                    <div class="data-table-container">
                        <table class="data-table">
                            <thead>
                                <tr class="table-headers" id="goto-table-headers"></tr>
                            </thead>
                            <tbody class="table-body" id="goto-table-body"></tbody>
                        </table>
                    </div>
                `;
        renderTable("goto-table-headers", "goto-table-body", gotoTableData);
    } catch (error) {
        console.error("Error fetching GOTOtable data:", error);
        gotoTabContent.innerHTML = `
                    <div style="display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100%; text-align: center;">
                        <div class="warning"></div>
                        <p style="color: #F0605C; margin-top: 0px; font-size: 14px;">加载失败，请重试</p>
                    </div>
                `;
        actionTabContent.innerHTML = gotoTabContent.innerHTML;
    }

    // Tab 切换逻辑
    tabs.forEach((tab) => {
        tab.addEventListener("click", async () => {
            tabs.forEach((t) => t.classList.remove("active"));
            tabContents.forEach((content) => content.classList.add("hidden"));

            // 激活当前 Tab
            tab.classList.add("active");
            // 显示对应的 Tab 内容
            const targetId = tab.getAttribute("data-tab");
            const targetContent = document.getElementById(targetId);
            targetContent.classList.remove("hidden");

            if (targetId === "tab2") {
                const actionTabContent = document.getElementById("tab2");
                actionTabContent.innerHTML = `
                            <div class="data-table-container">
                                <table class="data-table">
                                    <thead>
                                        <tr class="table-headers" id="action-table-headers"></tr>
                                    </thead>
                                    <tbody class="table-body" id="action-table-body"></tbody>
                                </table>
                            </div>
                        `;
                renderTable(
                    "action-table-headers",
                    "action-table-body",
                    actionTableData
                );
            } else if (targetId === "tab3" || targetId === "tab4" || targetId === "tab5") {
                getParser();
            }
            else if(targetId === "tab6"){
                getGenerator();
            }
        });
    });
});
