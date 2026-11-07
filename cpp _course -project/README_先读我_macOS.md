# 学生成绩管理系统 · 第 8 周提交（macOS）

## 本阶段做了什么

第 8 周交付「账号 + 查询」链路：三角色登录、密码加盐存储、登录记录、学生个人信息与成绩查询、班级科目目录、管理员全局账号管理（查询/新增/批量/禁用/删除/重置密码）。

第 12 周再补「维护 + 录入」：学生信息维护、成绩录入、批量导入、班级与科目维护、报表导出。

## 快速开始

```sh
sh build_all.sh
sh run_manual.sh
```

B 版：

```sh
sh run_ai.sh
```

## 本阶段可用命令

A 版：`login`、`logout`、`me`、`password`、`history`、`classes`、`subjects`、`scores`、`users`、`teachers`、`user-add`、`user-delete`、`user-disable`、`user-batch`、`reset`、`reset-all`、`help`。

B 版：`login`、`logout`、`me`、`catalog`、`passwd`、`history`、`grades`、`users`、`add-student`、`add-teacher`、`import-users`、`user-enable`、`user-disable`、`user-delete`、`reset-password`、`help`。

`students`、`grade`、`score`、`import-scores`、`rank`、`stats`、`chart`、`export`、`backup` 等属于第 12、16 周内容，本阶段执行会提示“未知命令”。

## 演示账号

| 角色 | 账号 | 密码 |
|---|---|---|
| 管理员 | admin | Admin@123 |
| 教师 | teacher1 | Teach@123 |
| 学生 | stu1 | Stu@123 |

## 目录说明

- `manual/`：A 版，单文件手工实现，详见该目录 README。
- `ai_assisted/`：B 版，domain / repository / service 分层实现。
- `build_all.sh`、`run_manual.sh`、`run_ai.sh`：编译与启动入口。
- `小组作业规划表.xlsx`：小组分工与进度计划。

## 演示脚本

`manual/examples/demo_commands.txt` 与 `ai_assisted/examples/demo_commands.txt` 可直接粘贴运行，只做查询，不修改数据。

## 环境

macOS + clang++（C++17）。终端内即可完成编译与运行。
