# 学生成绩管理系统 B：macOS / Apple Clang 版

本项目为课程对比实验中的 **B 方案（ AI 为主 + 人工辅助）**，使用 C++17、标准库和本地文本文件，实现学生、教师、管理员三类账号的成绩管理。方案风格为分层建模、集中权限策略、候选状态验证和可重复业务自测。

“AI 为主 + 人工辅助”是课程要求中的**开发方式**；主要代码由 AI 辅助生成。

## 1. 在 Mac 上运行

要求 **macOS 10.15 或以上**（使用 C++17 `std::filesystem`），以及支持 C++17 的 Apple Command Line Tools。无需安装 Homebrew、CMake、数据库或其他第三方库。Apple Silicon 与 Intel Mac 均在本机用源码编译，生成适合当前机器的原生程序；本包不将 Windows `.exe` 当作 Mac 可执行文件。

在终端进入解压后的 `ai_assisted` 目录，执行：

```sh
sh run.sh
```

也可以在任意当前目录，用脚本完整路径启动；中文与空格路径须加引号：

```sh
sh "/你的解压目录/课程项目/ai_assisted/run.sh"
```

`run.sh` **每次启动都会重新编译当前源码**，成功后切换到项目目录再启动，因此 `import-grades examples/grades_valid.txt` 等相对路径不受终端原来所在目录影响。使用 `sh` 调用不要求脚本预先具有执行权限。源码与脚本为 UTF-8；脚本使用 LF 换行且无 BOM。

若缺少 Apple 命令行开发工具，脚本会提示并退出。请自行执行下面的安装命令，完成系统安装流程后重试；脚本不会代为安装：

```sh
xcode-select --install
```

如果已安装完整 Xcode，已选中的有效 Xcode 开发者目录也可满足要求。脚本在 Darwin 上检查 `xcode-select -p` 与 `xcrun --find clang++`。

默认数据目录为本项目的 `data/`。不存在的目录会自动生成演示数据；已有数据库存在时读取原数据，不会覆盖为演示数据。附带数据只含虚构学生。

| 角色 | 账号 | 初始密码 | 访问范围 |
|---|---|---|---|
| 管理员 | admin | Admin@123 | 全校 |
| 教师 | teacher1 | Teach@123 | C1 |
| 教师 | teacher2 | Teach@123 | C2 |
| 学生 | stu1、stu2、stu3 | Stu@123 | 各自资料，属于 C1 |
| 学生 | stu4 | Stu@123 | 本人资料，属于 C2 |

输入 `help` 查看全部中文指令。命令、账号和编号区分大小写；含空格的参数使用英文双引号。例如 `import-grades "/Users/你的用户名/Documents/课程资料/成绩 文本.txt"`。程序内输入路径时请使用完整路径，或相对于项目目录的路径；程序不会把 `~` 展开为用户目录。

可用启动参数覆盖默认数据目录：

```sh
sh run.sh --data "/Users/你的用户名/Documents/成绩 数据"
sh run.sh --data demo_replay
sh run.sh --self-test
sh run.sh --help
```

通过 `run.sh` 传入的相对 `--data` 路径也是**相对于项目目录**，完整路径则按原样使用。每次传入的所有参数均完整转交程序。不要让两个实例同时使用同一数据目录。

## 2. 编译、自测与验证范围

只编译、不运行：

```sh
sh build.sh
```

默认编译器为 `clang++`，参数为 `-std=c++17 -O2 -Wall -Wextra -Wpedantic`，入口为 `src/main.cpp`，输出为 **`bin/ai_grade`**。可用 `CXX` 覆盖为一个可执行文件名或完整路径，例如：

```sh
CXX=/usr/bin/clang++ sh build.sh
```

`CXX` 只指定一个编译器程序，不接受 `ccache clang++` 之类的多个命令词，也不通过 `eval` 执行。构建先在 `bin/` 下生成唯一临时文件，仅编译成功后替换 `bin/ai_grade`；检查工具或编译失败时，原有可用程序保持不变。启动脚本在编译失败时退出，不会悄悄运行旧程序。

编译后可直接运行二进制，省去再次编译：

```sh
./bin/ai_grade --self-test
./bin/ai_grade --data data
./bin/ai_grade --data demo_replay < examples/demo_commands.txt
```

以上三个直接执行命令以当前终端已经进入项目目录为前提。直接执行二进制时，相对路径按终端当前目录解释。自测在系统临时目录创建随机的独立目录，并只清理本次创建的目录，不修改演示数据；其中覆盖密码、权限、统计、原子导入、损坏恢复、审计故障以及 SHA-256 已知向量等检查。macOS 临时目录的末尾斜杠或 `/private` 路径别名通过文件系统等价判断处理。

如已经安装 CMake，也可选择以下方式；这不是使用本项目的前提：

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

CMake 明确要求 C++17，关闭编译器方言扩展；macOS 不链接 Windows 的 `shell32`。Mac 代码路径使用 `localtime_r`，Windows 专有头文件和命令行 API 均位于条件编译分支中。补齐了字符分类、整型、移动语义和错误码等显式标准头，避免依赖 MSVC 头文件的间接包含。

**验证范围须区分：**原 Windows/MSVC 交付在 2026-09-20 完成 **74/74 业务自测**、中文启动路径 **3/3 检查**；原有 `self_test_results.txt`、`unicode_startup_results.txt`、`build_results.txt`、`demo_run_results.txt` 都是该 Windows 版本的历史证据，不能作为 Apple Clang 或 macOS 实机通过的证明。本次副本做了 Mac 源码和启动脚本适配；Mac 实际结果应以你本机执行 `sh run.sh --self-test` 的输出，以及交付根目录另行标明的适配验证记录为准。

交互命令遇到可预期的业务错误时会显示错误并允许继续输入；因此批量演示脚本的进程退出码不能代替逐条业务断言。`--self-test` 有独立失败计数，有失败时返回非零退出码。

## 3. 日常操作示例

### 学生

```text
login stu1 Stu@123
me
grades
grades CPP
rank-me
export-me CPP
history
passwd Stu@123 MyNew@123
logout
```

学生只可看到本人备案信息、本人全部或筛选科目成绩、本人班级名次和本人登录记录。不能查看他人成绩、班级名单、班级统计或任何管理入口。修改密码属于账号自助功能，不开放学籍或成绩编辑。

### 班主任教师

```text
login teacher1 Teach@123
students C1
students C1 张
student-edit S001 张三 男 2025 C1 13800000000
student-grades S001
score S001 CPP 86
import-grades examples/grades_valid.txt
rank C1 total desc
rank C1 CPP asc
stats C1
chart C1 MATH
export students C1
export grades C1
export rank C1 CPP asc
export stats C1
logs
export-logs
logout
```

教师不能跨班访问，不能转班，不能新增、删除或禁用账号；只能查询自己的操作日志。权限检查位于业务服务入口，即使跳过菜单直接调用业务接口，仍执行相同限制。管理员变更教师辖班或禁用账号后，现有会话下一次业务调用会读取最新授权信息。

### 管理员

```text
login admin Admin@123
users
catalog
class-add C3 计算机三班
subject-add PHY 物理
add-student stu5 Stu@123 S005 孙同学 C3 女 2025 未填写
add-teacher teacher3 Teach@123 T003 李老师 C1,C3
teacher-edit T003 李老师 C3
reset-password stu5 New@123
user-disable stu5,teacher3
user-enable stu5,teacher3
user-delete stu5,teacher3 CONFIRM
class-delete C3 CONFIRM
subject-delete PHY CONFIRM
backup
logs LOGIN
export-logs LOGIN
```

班级和科目的改名分别使用 `class-edit 编号 名称`、`subject-edit 编号 名称`。学生和教师账号创建时同时创建关联资料；删除学生账号同时删除其资料与成绩。管理员不能通过批量账号接口删除或禁用管理员账号，但可以重置任意角色密码。学生信息改动使用 `student-edit`，教师信息使用 `teacher-edit`。

仍有学生或教师关联的班级不能删除；删除科目则同时删除该科目成绩。`score-delete 学号 科目 CONFIRM` 删除一条成绩。所有上述删除均要求显式 `CONFIRM`。

## 4. 批量导入规范

成绩文件：UTF-8 文本（支持 BOM 和 CRLF），每行三列，以空格或制表符分隔，空行与以 `#` 开头的注释行忽略。

```text
# 学号 科目代码 成绩
S001 MATH 90
S001 CPP 80
S002 CPP 80
```

调用：`import-grades examples/grades_valid.txt`。已有成绩更新，没有成绩则新增。整个文件在候选状态中完成解析、角色范围、引用、重复项与数字校验后才提交。任一未知学号/科目、重复的“学号+科目”、越权、额外字段、NaN、无穷值或越界成绩，都会拒绝整批。`examples/grades_invalid.txt` 可展示第二行非法时第一行也不提交。

账号文件使用以下两种行格式；文本值有空格时使用英文双引号。

```text
student 账号 密码 学号 姓名 班级 性别 年级 联系方式
teacher 账号 密码 工号 姓名 班级1,班级2
```

调用：`import-users examples/users_batch.txt`。该功能仅管理员可用，整个文件原子校验，不允许同批或存量中的重复账号/编号。示例文件使用 `stu5`、`stu6`、`teacher3`，重复导入会被拒绝。批量删除、禁用和启用使用逗号分隔账号列表，也全部预先校验。

## 5. 统计、排名与图表口径

- 假定全体学生统一选修科目列表，所有科目为 1 学分。
- 单科成绩范围为 0—100，可含小数，拒绝 NaN、无穷值及数字后的杂字符。
- `GPA = 0`（成绩低于 60）；60≤成绩<90 时为 `(成绩-50)/10`；90≤成绩≤100 时为 4。平均绩点取已录科目 GPA 均值，是本项目自行说明的课程实验口径。
- 总分、均分、GPA 均标注“已录”；没有成绩时均分和 GPA 显示 `N/A`。
- 缺成绩不作 0 分；只有全部科目完整的学生参加总分/均分排名，单科排名只比较该科已录学生。
- 同分采用竞争排名，例如 1、1、3。为避免十进制小数求和的二进制浮点误差，排序后与本组首值差不超过 `1e-9` 视为同分（不进行相邻值链式归组）。`asc` 仅改变显示顺序，保留按高分优先计算的学业名次。
- 及格线为 60；及格率分母是该科已录人数，同时显示班级人数和缺成绩人数。
- 图表按 `[0,60)`、`[60,70)`、`[70,80)`、`[80,90)`、`[90,100]` 五段统计；无成绩时显示空状态，不作除零运算。

统一演示 C1 数据的结果：张三和李四总分均为 240、均分均为 80，并列第 1；王五英语未录，不参与总分/均分排名。CPP 成绩为 80、80、60，名次为 1、1、3。数学均分 66.67、最高 90、最低 50、及格 2 人、不及格 1 人、已录及格率 66.67%。

`chart C1 MATH` 生成自包含 HTML，无网络依赖，内嵌 SVG 饼图与柱图，可用浏览器打开；同时持久化对应的 TXT 统计快照。所有导出按 `data/reports/类型/范围_时间戳.扩展名` 分类，若同秒重名则自动加序号，避免覆盖。

## 6. 文件存储、日志与恢复

```text
data/
  school.db          带版本号、引用校验和 SHA-256 校验尾的数据快照
  audit.log          时间、账号、身份、动作的结构化操作日志
  reports/
    personal/        本人成绩 TXT
    students/        学生信息 TXT
    grades/          班级成绩 TXT
    rank/            排名 TXT
    stats/           统计 TXT 与图表统计快照
    charts/          内嵌 SVG 的 HTML
    logs/            按权限导出的操作日志 TXT
  backups/
    school_*.bak     数据库备份
    school_*.bak.log 对应操作日志副本
```

日志记录登录成功和失败、查阅、资料/成绩变更、批量操作、密码变更、导出、备份、恢复、拒绝和命令失败。密码参数从不写入日志。学生的 `history` 仅返回自己的登录记录；教师 `logs` 仅返回自身动作；管理员可按时间、账号、角色或动作文本检索全部日志。`logs-clear CONFIRM` 仅管理员可用，清空后仍保留该次清空记录。

备份：管理员执行 `backup`，控制台给出实际文件路径。

恢复：管理员执行 `restore "实际备份路径" CONFIRM`。程序先验证版本、完整校验和、全部字段、重复记录、引用关系和至少一个启用管理员，再保存恢复前安全副本，最后替换当前数据库。坏备份不会改变当前内存或数据库。成功恢复后控制台会退出当前登录，要求重新登录。**恢复数据库不会回滚当前日志**，以保留恢复行为的追溯链；备份的 `.log` 文件仅作为独立归档。

写数据库采用临时文件及同目录重命名方案；启动时若主文件缺失而 `.previous` 存在，会先验证后恢复该副本。若主库损坏，启动报错并保留文件，不会静默重建演示数据。

## 7. 架构与已知边界

类职责、接口与实际迭代问题见 `DESIGN.md`。

这是单机、单进程课程系统，没有数据库、第三方依赖、并发锁、网络服务或生产级权限隔离。任何拥有本机数据目录写权限的人仍可从系统外修改文件。校验和用于发现损坏，不提供密钥签名；日志不是不可篡改审计系统。

密码使用随机盐和 SHA-256 摘要保存，未保存明文；该快速哈希没有 Argon2/PBKDF2 等密码 KDF 的计算成本，**不宣称生产级密码安全**。控制台密码输入可见，示例文件也只用于虚构演示密码。

数据库提交与日志追加是两个文件操作；若数据已成功保存而日志写入失败，程序明确显示“数据已保存，但操作日志写入失败”，避免误报事务未提交。此故障路径已实际测试，但不具备跨文件事务或断电级持久化保证。

操作日志、报表与备份没有自动轮转；数据读取和排序在内存中完成，适合课程演示规模。后续可以增加持久事务、并发锁、密码 KDF、分页与真正的 GUI，但这些不作为本次已实现内容。

