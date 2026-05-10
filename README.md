# Git GUI - 简易版

一个使用 C++ 和 GTKmm 编写的轻量级 Git 图形界面客户端。

## 功能特性

- ✅ 查看 Git 仓库状态（未暂存/已暂存文件）
- ✅ 查看文件差异（Diff）
- ✅ 提交更改
- ✅ 实时刷新状态
- ✅ 手动选择 Git 仓库路径（可在非 Git 目录启动）
- 🚧 暂存/取消暂存文件
- 🚧 分支切换与管理

## 系统要求

- Windows 10/11 + WSL2，或原生 Linux
- GTKmm 3.0
- CMake 3.10+
- Git 2.0+

## 安装依赖

### Ubuntu/Debian/WSL

```bash
sudo apt update
sudo apt install libgtkmm-3.0-dev cmake git
```

验证安装：

```bash
g++ --version
pkg-config --modversion gtkmm-3.0
git --version
```

## 编译运行

```bash
# 进入项目目录
cd ~/my_git_gui

# 创建构建目录
mkdir -p build && cd build

# 配置并编译
cmake ..
make

# 运行
./gitgui
```

## 使用说明

### 基本操作

1. 刷新：点击“刷新”按钮
2. 查看 Diff：点击左侧文件列表中的文件
3. 提交：在输入框中填写提交消息，点击“提交”
4. 选择仓库：点击“选择仓库”按钮，选择 Git 仓库目录

### 文件状态标识

| 状态码 | 含义 |
| --- | --- |
| `??` | 未跟踪文件 |
| ` M` | 已修改（未暂存） |
| `M ` | 已修改（已暂存） |
| `A ` | 新增（已暂存） |
| `D ` | 删除（已暂存） |

## 项目结构

```text
my_git_gui/
├── .gitignore
├── CMakeLists.txt
├── main.cpp
├── README.md
└── build/
```

## 技术栈

- 语言：C++17
- GUI 框架：GTKmm 3.0
- 构建工具：CMake + Make

## 待实现功能

- 暂存文件（git add）
- 取消暂存（git reset HEAD）
- 丢弃更改（git checkout --）
- 显示当前分支
- 分支切换
- 推送/拉取

## 常见问题

### Q: 编译时找不到 gtkmm.h

```bash
sudo apt install libgtkmm-3.0-dev
```

### Q: 运行后显示“不是 Git 仓库”

可先点击“选择仓库”手动选择已有 Git 仓库目录，或在目标目录执行：

```bash
git init
```

### Q: VS Code 显示红色波浪线

通常不影响编译。可执行：

- `Ctrl+Shift+P`
- 输入并执行 `C/C++: Reset IntelliSense Database`

## 许可证

MIT License

## 作者

gyx114
