# 学生成绩管理系统 A · macOS 版

本方案是“学生手工设计”的实现风格：少量实体结构、一个业务控制类、`vector` 顺序存储、直接清晰的控制台命令。** 本目录包含独立完整系统，可以独立编译运行。

## macOS 运行与编译

适用于 macOS 10.15 或更高版本，使用 Apple Command Line Tools 自带的 `clang++` 和 C++17 标准库，不需要 Homebrew、CMake、Qt 或数据库。可在 Apple Silicon 或 Intel Mac 上从源码编译成本机架构的程序。

先打开“终端”。尚未安装开发工具时，手动执行一次并完成系统安装提示：

```sh
xcode-select --install
```

进入本目录后运行：

```sh
sh ./run.sh
```

也可以在任意工作目录中通过完整路径启动，中文和空格路径需要引号：

```sh
sh "/完整路径/课程项目/manual/run.sh"
```

`run.sh` 每次启动都会重新编译当前源码，成功后进入本目录并运行 `bin/manual_grade`，默认使用本目录的 `data/`。首次解压无需手动设置脚本执行权限，也不会误用其他电脑或其他架构的旧二进制。编译失败时停止启动，并保留上次成功的程序文件。

只编译、执行自测或重放演示：

```sh
sh ./build.sh
./bin/manual_grade --self-test
sh ./run.sh --data "/完整路径/个人演示数据" --script examples/demo_commands.txt
```

`build.sh` 默认运行 `clang++ -std=c++17 -O2 -Wall -Wextra -Wpedantic`。它先编译到 `bin/` 中的临时文件，全部成功才替换 `bin/manual_grade`；不覆盖旧的有效程序来尝试失败的编译。脚本使用 POSIX `sh`、LF 换行且不带 BOM。

如需指定其他已安装的 C++17 编译器，可设置 `CXX`，它只能是一个可执行文件名或完整路径，不能填写带参数的命令：

```sh
CXX="/完整路径/clang++" sh ./build.sh
```

macOS 下脚本会检查开发工具配置。缺失时给出 `xcode-select --install` 的手动安装说明，不会自动安装或更改系统配置。所有启动参数都传给程序；显式 `--data` 覆盖默认数据目录。`run.sh` 切换到本目录后，`examples/...` 相对路径可直接使用；外部自定义数据、脚本路径建议填写带引号的绝对路径。

可选 CMake 配置仍保留，明确要求 C++17 并禁用编译器语言扩展；它不是 macOS 启动的前置依赖：

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

本次移植没有在实际 Mac 上执行 Apple Clang 编译或运行；随包 Windows 构建、自测及 CLI 验收记录是历史业务验证，不能视为 macOS 实机验证。可在目标 Mac 上运行 `sh ./run.sh --self-test` 检验编译和 128 项业务检查。源码中 Windows 专用 API 均由 `_WIN32` 条件编译隔离，macOS 使用标准库和 POSIX 时间转换。

程序参数：

| 参数 | 用途 |
|---|---|
| `--data PATH` | 独立数据目录；直接运行程序默认当前工作目录的 `data`，`run.sh` 默认本目录 `data` |
| `--script FILE` | 按行执行 UTF-8 命令脚本；全部成功返回 0，有命令失败返回 2 |
| `--self-test` | 在随机临时目录运行业务集成测试，失败返回非 0，不改动演示数据 |
| `--help` | 显示启动参数 |

交互输入 `help` 查看全部命令，`quit` 退出。命令名使用英文，提示使用中文。带空格的字段和路径用英文双引号包围；macOS 文件路径使用 `/`，例如 `"/Users/你的用户名/课程资料/成绩.txt"`。

## 演示账号与数据

| 角色 | 账号 | 密码 | 数据范围 |
|---|---|---|---|
| 管理员 | `admin` | `Admin@123` | 全部数据 |
| 教师 | `teacher1` | `Teach@123` | C1 |
| 教师 | `teacher2` | `Teach@123` | C2 |
| 学生 | `stu1` / `stu2` / `stu3` / `stu4` | `Stu@123` | S001 / S002 / S003 / S004 本人 |

示例姓名与联系方式均为演示数据。全新数据目录会自动初始化两个班、两位教师、四名学生和三个科目；已有数据文件不会自动重置。

| 学生 | 班级 | MATH | CPP | ENG | 核验结果 |
|---|---|---:|---:|---:|---|
| S001 张三 | C1 | 90 | 80 | 70 | 总分 240，均分 80，全科 GPA 3.00，班级总分第 1 |
| S002 李四 | C1 | 60 | 80 | 100 | 总分 240，均分 80，全科 GPA 2.67，班级总分并列第 1 |
| S003 王五 | C1 | 50 | 60 | 未录入 | 已录总分 110，已录均分 55，已录科目 GPA 0.50；全科统计待齐全 |
| S004 赵六 | C2 | 88 | 92 | 76 | 用于隔班权限检查 |

## 三角色操作示例

学生可查本人资料、成绩、单科和总均分名次，修改本人密码，查看本人登录历史，导出本人全科或单科 TXT。无学生或成绩写权限。

```text
login stu1 Stu@123
me
scores
history
export personal all
export personal MATH
password Stu@123 NewPass@123
logout
```

教师只能查看和操作分配给自己的班级。`student-edit` 可编辑姓名、性别、电话等资料，但教师不能转班；不能新增、删除学生或管理账号。

```text
login teacher1 Teach@123
students C1
students C1 张
student-get S001
student-edit S001 张三 男 2025 C1 13800000001
grade S001 MATH 90
import-scores "examples/scores_valid.txt"
rank C1 total desc
rank C1 average asc
rank C1 CPP asc
stats C1
chart C1 MATH
export students C1
export scores C1
export ranking C1 total desc
export statistics C1
logs
export logs
logout
```

管理员维护账号、学生、教师、班级、科目、成绩、日志与备份。删除和恢复需要明确填写 `CONFIRM`；少填或填错不会执行。

```text
login admin Admin@123
users
teachers
classes
subjects
class-add C3 测试班 2026
student-add S009 测试生 女 2026 C3 13800000009 - -
user-add teststu Temp@123 student S009
teacher-add T009 测试师 13900000009 C3 testteacher Temp@123
teacher-edit T009 测试师 13900000009 C1,C3
user-disable teststu true
user-disable teststu false
reset teststu Reset@123
subject-add PHY 大学物理 2
grade S009 PHY 66
grade-delete S009 PHY CONFIRM
subject-delete PHY CONFIRM
student-delete S009 CONFIRM
teacher-delete T009 CONFIRM
class-delete C3 CONFIRM
backup
export logs
```

`class-edit 编号 名称 年级` 更新班级，联动同班学生年级；`subject-edit 编号 名称 学分` 更新科目。删除有学生或教师引用的班级会被拒绝。删除学生会连带删除其账号与成绩；删除教师会删除其账号；删除科目会连带删除该科成绩。`user-delete 账号 CONFIRM` 只删除账号，保留基础资料。系统保留至少一个启用管理员，账号增删禁用接口只处理学生和教师。

`reset-all 新密码 CONFIRM` 重置所有用户，包括当前管理员。`restore "备份文件路径" CONFIRM` 校验整个备份后恢复业务资料及凭据，保留当前审计记录，生成恢复前安全副本并退出登录。`logs-clear CONFIRM` 仅管理员可执行，清空后仍写入本次清空审计。

## 批量文件与原子性

成绩文件使用 UTF-8，每行三个字段：`学号 科目编号 成绩`；支持空格或制表符，`#` 起始行作为注释。示例在 `examples/scores_valid.txt`。同一文件出现重复学生科目、非有限数、越界成绩、未知引用或越权记录，整批操作回滚；在拒绝导入后仍记录失败审计。

管理员账号批量文件支持：

```text
ADD 账号 密码 student|teacher 学号或工号
DELETE 账号
DISABLE 账号 true|false
```

对应命令 `user-batch "examples/accounts_batch.txt" CONFIRM`。示例在初始演示资料上批量删除并重新创建学生和教师账号，再批量禁用、启用。每个学生或教师最多对应一个账号，基础资料需先存在。任一行失败，整批不生效。

## 统计口径

- 所有学生选修系统内全部科目。未录入用“未录入”表示，不转换为 0。
- 已录总分、已录均分可随时查看；全部科目齐全后才参与全科总分、平均分排名及全科 GPA。缺录时显示已录科目 GPA 并明确说明口径。
- 单科绩点：成绩低于 60 时为 0；其他成绩为 `min(4, max(0, (成绩-50)/10))`。GPA 按科目学分加权；初始三个科目均为 1 学分。
- 并列采用竞争排名 `1,1,3`；原始排序保持严格大小比较，分组时以组内最高分为锚，允许 `1e-9` 浮点误差。升降序只改变显示顺序，名次始终以高分为先。单科排名只纳入该科已录入者。
- 单科平均分、最高、最低、及格人数、不及格人数仅计算已录入者；及格率为 `及格人数/已录入人数`，另报未录入人数。空数据用“无”表示。
- 图表分段为 `[0,60)`、`[60,70)`、`[70,80)`、`[80,90)`、`[90,100]`，覆盖小数成绩。一次 `chart` 生成包含饼图、柱状图的 SVG，并持久化同名 TXT 统计。

## 存储、日志与备份

`data/school.db` 是自定义带版本的 UTF-8 文本格式，使用带引号字段序列化，不需要数据库。文件尾保存 SHA-256 校验和；加载时先校验整个文件，再检查账号、外键、重复编号、成绩范围及管理员可用性，验证完成才替换内存。

写入先产生临时文件，再保留旧文件并替换；若程序中断导致主文件缺失但 `.previous` 存在，下次启动会先校验 `.previous` 再恢复。备份位于 `data/backups/`。损坏主文件会导致启动明确失败且保留现场；可将有效备份复制为新的独立数据目录的 `school.db` 后启动，或在正常运行实例中使用 `restore`。

密码采用随机盐 + 2,000 轮 SHA-256，持久化文件和日志不记录明文密码。默认口令是公开教学演示口令。普通学生、教师的日志查询和导出仅显示本人；管理员可检索、导出全部日志。登录成功/失败、查询、修改、拒绝越权、批量、导出、修改密码、备份恢复均记载时间、账号、身份和操作结果。涉密命令隐藏敏感参数。

报表自动分类到 `data/reports/personal|students|scores|ranking|statistics|logs|charts/`，文件名包含类型、范围、时间和递增编号，避免覆盖。SVG 可用浏览器直接打开。

## 历史验证与当前局限

历史 Windows 版本执行 `--self-test`：**128 项检查通过**。覆盖 SHA-256 已知向量、三角色授权、NaN/Inf/非法数字/0/100、总均分与 GPA、并列和缺失、升序名次、批量回滚、导出、图表、账号 CRUD、确认保护、备份恢复、正确校验和但业务非法的备份、重启持久化和中断恢复等。`evidence/self_test.txt` 和 `evidence/build.txt` 属于此前 Windows 构建的记录；本次 macOS 版需要在目标 Mac 重新编译和执行自测。

此前独立于内置自测的 Windows 外部 CLI 验收还通过 29 项检查（由另一验收任务执行，涵盖 11 个进程场景）；统一项目验收目录保存其证据。`evidence/demo_run.txt` 是当时的实际演示输出，`examples/demo_commands.txt` 可在 macOS 重放。测试数据与演示数据隔离。

本系统是单机教学项目，不支持多进程并发访问同一目录。自定义密码散列不等同于生产级密码系统；校验和用于损坏检测，不能抵抗能重写本地文件的恶意人员。密码在命令行交互中可见；数据、备份及导出文件没有加密，文件安全依赖操作系统权限。全部数据和日志放入内存并在每次命令后持久化，适合课程规模，数据量大时应改用数据库、索引和分段日志。控制台不提供窗口弹窗，重要操作通过 `CONFIRM` 明确确认。

