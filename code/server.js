/**
 * 文件: server.js
 * 描述: 接收前端用户的HTTP请求，调用C++程序并将结果返回
 * 创建日期: 2025-04-26
 * 
 * 负责前端web项目与后端C++程序交互的服务器中间件
 */
const express = require('express');
const { spawn } = require('child_process');
const cors = require('cors');
const app = express();
const process = require('process');

// 启用CORS
app.use(cors());
app.use(express.json());


// 通用函数：调用c++可执行文件
function executeCppProgram(executablePath, input, callback) {
    const child = spawn(executablePath, [], {
        stdio: ['pipe', 'pipe', 'pipe']
    });

    let result = '';
    let errorOutput = '';

    // 将输入传递给 C++ 程序
    child.stdin.write(input);
    child.stdin.end();

    // 收集标准输出
    child.stdout.on('data', (data) => {
        result += data.toString();
    });

    // 收集错误输出
    child.stderr.on('data', (data) => {
        errorOutput += data.toString();
    });

    // 子进程关闭时调用回调
    child.on('close', (code) => {
        if (code === 0) {
            callback(null, result);
        } else {
            const errorMsg = `C++ 程序执行失败，错误输出: ${errorOutput}`;
            console.error(errorMsg);
            callback(new Error(errorMsg));
            // 立即重启服务器
            restartServer();
        }
    });

    // 捕获子进程启动错误
    child.on('error', (error) => {
        const errorMsg = `子进程启动失败: ${error.message}`;
        console.error(errorMsg);
        callback(new Error(errorMsg));
        // 立即重启服务器
        restartServer();
    });
}

// 重启服务器
function restartServer() {
    console.log('C++ 程序执行失败，正在重启服务器...');
    
    if (serverInstance) {
        serverInstance.close(() => {
            startServer();
        });
    } else {
        startServer();
    }
}

// 服务器实例
let serverInstance;
let isRestarting = false; // 防止重复重启
let restartAttempts = 0;  // 记录重启尝试次数
const MAX_RESTART_ATTEMPTS = 100; // 最大重启次数

// 通用函数：调用c++可执行文件
function executeCppProgram(executablePath, input, callback) {
    const child = spawn(executablePath, [], {
        stdio: ['pipe', 'pipe', 'pipe']
    });

    let result = '';
    let errorOutput = '';

    // 将输入传递给 C++ 程序
    child.stdin.write(input);
    child.stdin.end();

    // 收集标准输出
    child.stdout.on('data', (data) => {
        result += data.toString();
    });

    // 收集错误输出
    child.stderr.on('data', (data) => {
        errorOutput += data.toString();
    });

    // 子进程关闭时调用回调
    child.on('close', (code) => {
        if (code === 0) {
            callback(null, result);
        } else {
            const errorMsg = `C++ 程序执行失败，错误输出: ${errorOutput}`;
            console.error(errorMsg);
            callback(new Error(errorMsg));
            // 立即重启服务器
            safeRestartServer();
        }
    });

    // 捕获子进程启动错误
    child.on('error', (error) => {
        const errorMsg = `子进程启动失败: ${error.message}`;
        console.error(errorMsg);
        callback(new Error(errorMsg));
        // 立即重启服务器
        safeRestartServer();
    });
}

// 安全重启服务器
function safeRestartServer() {
    // 防止重复重启
    if (isRestarting) {
        console.log('已有重启操作在进行中，跳过此次重启');
        return;
    }
    
    // 限制重启次数
    restartAttempts++;
    if (restartAttempts > MAX_RESTART_ATTEMPTS) {
        console.error(`已达到最大重启次数(${MAX_RESTART_ATTEMPTS})，停止尝试`);
        return;
    }
    
    isRestarting = true;
    console.log(`第 ${restartAttempts} 次尝试重启服务器...`);
    
    if (serverInstance) {
        console.log('正在关闭当前服务器实例...');
        
        // 设置超时防止永久等待
        const closeTimeout = setTimeout(() => {
            console.error('关闭服务器超时，强制退出进程');
            process.exit(1);
        }, 100000); // 100秒超时
        
        serverInstance.close((err) => {
            clearTimeout(closeTimeout); // 清除超时计时器
            
            if (err) {
                console.error('关闭服务器失败:', err.message);
                console.log('尝试强制重启...');
                forceRestartServer();
            } else {
                console.log('服务器已成功关闭，准备重新启动');
                startServer();
            }
            
            isRestarting = false;
        });
    } else {
        console.log('没有活动的服务器实例，直接启动新实例');
        startServer();
        isRestarting = false;
    }
}

// 强制重启服务器（不尝试关闭现有实例）
function forceRestartServer() {
    console.log('尝试强制重启服务器...');
    
    // 销毁现有实例引用
    serverInstance = null;
    
    // 增加端口释放的延迟时间
    setTimeout(() => {
        startServer();
        isRestarting = false;
    }, 2000); // 增加延迟时间到2秒
}

// 启动服务器函数
function startServer() {
    serverInstance = app.listen(3000, () => {
        console.log('Server running on port 3000');
        restartAttempts = 0; // 重置重启计数
    });
    
    // 处理服务器错误
    serverInstance.on('error', (err) => {
        console.error('服务器启动失败:', err.message);
        
        if (err.code === 'EADDRINUSE') {
            console.error('端口3000被占用，尝试释放...');
            forceRestartServer();
        } else {
            // 其他错误，延迟重启避免循环崩溃
            setTimeout(safeRestartServer, 2000);
        }
    });
}

// 词法分析
app.post('/analyze', (req, res) => {
    const code = req.body.code;
    if (!code) {
        return res.status(200).send('未提供代码');
    }
 
    executeCppProgram('./LexicalAnalyzer.exe', code, (error, result) => {
        if (error) {
            return res.status(500).send(error.message);
        }
        // 返回json到前端
        try {
            const jsonResult = JSON.parse(result);
            res.json(jsonResult);
        } catch (parseError) {
            console.error('JSON解析错误:', parseError);
            res.status(500).send('C++返回了无效的JSON');
            // JSON解析错误也触发重启
            restartServer();
        }
    });
});

// 语法规则
app.post('/grammar', (req, res) => {
    const code = req.body.code;
    if (!code) {
        return res.status(200).send('未提供代码');
    }

    executeCppProgram('./Grammar.exe', code, (error, result) => {
        if (error) {
            return res.status(500).send(error.message);
        }
        // 返回json到前端
        try {
            const jsonResult = JSON.parse(result);
            res.json(jsonResult);
        } catch (parseError) {
            console.error('JSON解析错误:', parseError);
            res.status(500).send('C++返回了无效的JSON');
            // JSON解析错误也触发重启
            restartServer();
        }
    });
});

// 语法分析
app.post('/parse', (req, res) => {
    const code = req.body.code;
    if (!code) {
        return res.status(200).send('未提供代码');
    }
 
    executeCppProgram('./Parser.exe', code, (error, result) => {
        if (error) {
            return res.status(500).send(error.message);
        }
        // 返回json到前端
        try {
            const jsonResult = JSON.parse(result);
            res.json(jsonResult);
            
        } catch (parseError) {
            console.error('JSON解析错误:', parseError);
            res.status(500).send('C++返回了无效的JSON');
            // JSON解析错误也触发重启
            restartServer();
        }
    });
});

// 目标代码生成
app.post('/generate', (req, res) => {
    const code = req.body.code;
    if (!code) {
        return res.status(200).send('未提供代码');
    }
 
    executeCppProgram('./CodeGenerator.exe', code, (error, result) => {
        if (error) {
            return res.status(500).send(error.message);
        }
        // 返回json到前端
        try {
            const jsonResult = JSON.parse(result);
            res.json(jsonResult);
        } catch (parseError) {
            console.error('JSON解析错误:', parseError);
            res.status(500).send('C++返回了无效的JSON');
            // JSON解析错误也触发重启
            restartServer();
        }
    });
}); 

// 启动服务器
startServer();