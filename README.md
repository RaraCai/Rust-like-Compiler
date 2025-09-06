# Rust-like-Compiler
![Static Badge](https://img.shields.io/badge/raraCai-pintu?label=Author&link=https%3A%2F%2Fgithub.com%2FraraCai)
![Static Badge](https://img.shields.io/badge/build-MIT-orange?label=License&link=https%3A%2F%2Fgithub.com%2FRaraCai%2FRust-like-Compiler%2Fblob%2Fmain%2FLICENSE)
![Static Badge](https://img.shields.io/badge/HTML%2FCSS%2FJavaScript-blue)
![Static Badge](https://img.shields.io/badge/C%2B%2B-purple)

> 版权所有 © 2025 Yuxuan Cai - 采用 [MIT许可证授权](LICENSE)  
> Copyright © 2025 Yuxuan Cai - Licensed under [MIT.License](LICENSE)

类Rust编译器：基于C++实现的针对Rust语言的词法语法分析工具、语义分析器和目标代码生成器.(2025年同济大学编译原理课程设计项目)💻  
Rust-like Compiler: lexical analyzer, parser, sematic analyzer and object code generator for Rust language based on C++.

## 目录(Content)
- [上手指南(Guide)](#上手指南guide)
  - [项目概述(Project Introduction)](#项目概述project-introduction)
  - [安装步骤(Installation Steps)](#安装步骤installation-steps)
- [免责声明(Disclaimers)](#免责声明disclaimers)

### 上手指南(Guide)

#### 项目概述(Project Introduction)
本项目基于C++和HTML/CSS/JavaScript实现了一个类Rust语言编译器及其可视化界面，为2025年同济大学编译原理课程设计项目。  
项目主要分为两大部分：
- 后端部分`/backend`为编译过程的实现，主要包括词法分析器`LexicalAnalyzer`（一遍扫描，作为语法分析器的子程序）、语法分析器`Parser`（采用LR1语法分析）、语义分析器`SemanticAnalyzer`（一遍扫描，作为语法分析器的子程序）、基本块划分器`BlockDivider`和目标代码生成器`Code Generator`五大核心模块的C++后端实现。用户可以直接通过终端运行的方式查看**文字化的编译结果**。
- 前端部分`/code`为编译器的可视化界面，用户可通过**web应用**的形式在前端编辑区实时输入Rust语言代码，并查看编译过程中各种中间数据结构的可视化表示，整个流程与常见的Ide的使用流程无异。用户可以自主选择查看包括`GOTO表`、`ACTION表`、`语法分析树`、`移进-规约过程`、`四元式形式的中间代码`以及`MIPS32汇编格式的目标代码`的可视化结果。
> The project designs and implements a Rust-like compiler and its visualization application based on C++ language and HTML/CSS/JavaScript, and is one of the Compilation Principle Courseworks of 2025 Tongji University.  
> The project can be divided into two main parts:
> - The backend part `/backend` implements the compilation process, mainly including the C++ backend implementation of five core modules: `Lexical Analyzer` (single-pass scanning, as a subroutine of the parser), `Parser` (using LR1 syntax analysis), `Semantic Analyzer` (single-pass scanning, as a subroutine of the parser), `Block Divider`, and `Code Generator`. Users can directly view the **textual compilation results** by running through the terminal.
> - The frontend part `/code` is the visual interface of the compiler. Users can input Rust code in real time through the editing area of the **web application** and view the visual representations of various intermediate data structures during the compilation process, with the entire workflow consistent with that of common IDEs. Users can independently choose to view visual results including the `GOTO Table`, `ACTION Table`, `Syntax Analysis Tree`, `Shift-Reduce Process`, `Intermediate Code in Quadruple Form`, and `Target Code in MIPS32 Assembly Format`.

<img width="417" height="218" alt="image" src="https://github.com/user-attachments/assets/982f6c38-b510-439a-94c8-32d64a971e21" />  
<img width="417" height="205" alt="image" src="https://github.com/user-attachments/assets/0615042b-fcf1-4a33-b689-14d519fa24a4" />

#### 安装步骤(Installation Steps)
**1. 生成后端应用：**
> [!TIP]
> 若您直接拉取了全部仓库，则`/code`路径下已经完全包含了本步骤要生成的所有可执行文件，**您可以直接跳过此步进行第2步**。
> If you have cloned the entire repo, the `/code` path will contain all required executable files, you may skip to step No.2

进入后端文件夹的`main.cpp`，按所需生成的模块部件生成对应的.exe并放入`/code`路径下。
> Navigate to `main.cpp` in the backend folder, generate the corresponding .exe for the required module components, and place it under the `/code` path.

**2. 运行前端界面：**
- 终端进入`/code`路径，首先使用`npm install`安装web界面运行所需的依赖包。
- 安装完成后，终端输入如下命令，当看到`Server running on Port 3000`表示启动成功，双击进入`index.html`即可启动界面。
  ```sh
  node server.js
  ```
> - Enter the `/code` path in the terminal and first use `npm install` to install the dependency packages required for running the web interface.
> - After the installation is complete, enter the following command in the terminal. When you see `Server running on Port 3000`, it indicates a successful startup. Double-click `index.html` to launch the interface.
>   ```sh
>   node server.js
>   ```

## 免责声明(Disclaimers)
本仓库包含的代码和资料仅用于个人学习和研究目的，不得用于任何商业用途。请其他用户在下载或参考本仓库内容时，严格遵守学术诚信原则，不得将这些资料用于任何形式的作业提交或其他可能违反学术诚信的行为。本人对因不恰当使用仓库内容导致的任何直接或间接后果不承担责任。请在使用前务必确保您的行为符合所在学校或机构的规定，以及适用的法律法规。如有任何问题，请通过[电子邮件](mailto:cyx_yuxuan@outlook.com)与我联系。
> The code and materials contained in this repository are intended for personal learning and research purposes only and may not be used for any commercial purposes. Other users who download or refer to the content of this repository must strictly adhere to the principles of academic integrity and must not use these materials for any form of homework submission or other actions that may violate academic honesty. I am not responsible for any direct or indirect consequences arising from the improper use of the contents of this repository. Please ensure that your actions comply with the regulations of your school or institution, as well as applicable laws and regulations, before using this content. If you have any questions, please contact me via [email](mailto:cyx_yuxuan@outlook.com).
