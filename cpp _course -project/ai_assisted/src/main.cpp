#include "self_test.hpp"
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#ifdef _MSC_VER
#pragma comment(lib, "shell32.lib")
#endif
#endif
using namespace sg;
static const char *HELP = R"HELP(
学生成绩管理系统 B（模拟 AI 为主 + 人工辅助；控制台 UTF-8）
所有含空格的参数用英文双引号包围；代码区分大小写；路径也可用双引号。
默认账号：admin / Admin@123；teacher1 / Teach@123；stu1 / Stu@123
第二班：teacher2 / Teach@123；stu4 / Stu@123。其余 stu2、stu3 密码 Stu@123。

【通用】
  login 账号 密码                  登录
  logout                          退出当前账号
  me                              当前账号和个人备案信息
  catalog                         选修科目、可访问班级
  passwd 旧密码 新密码            自主改密（6—100 字符）
  history                         本人登录成功/失败记录
  help / quit                     帮助 / 退出程序
【学生：本人只读】
  grades [科目代码]                本人全部或单科成绩，及已录总分/均分
【仅管理员】
  users                           查询全部账号与教师资料
  add-student 账号 初始密码 学号 姓名 班级 性别 年级 联系方式
  add-teacher 账号 初始密码 工号 姓名 班级列表
  import-users 文件                每行：student + add-student 参数，或
                                  teacher + add-teacher 参数；整批原子校验
  user-enable 账号1,账号2          批量启用学生/教师
  user-disable 账号1,账号2         批量禁用学生/教师（旧会话随即失效）
  user-delete 账号1,账号2 CONFIRM  批量删除
  reset-password 账号 新密码      重置任意角色密码

缺成绩不按0分。重要删除操作必须在命令尾输入 CONFIRM；日志不记录密码。
本版本为第 8 周阶段版本：资料维护、成绩录入、排名统计、图表、日志、备份恢复在第 12、16 周实现。
)HELP";
// CLI quotes group words; backslashes remain literal for Windows paths.
static std::vector<string> tokenize(const string &line) {
    std::vector<string> out;
    string token;
    bool quoted = false, started = false;
    for (unsigned char c : line) {
        if (c == '\"') {
            quoted = !quoted;
            started = true;
        } else if (std::isspace(c) && !quoted) {
            if (started) {
                out.push_back(token);
                token.clear();
                started = false;
            }
        } else {
            token += char(c);
            started = true;
        }
    }
    require(!quoted, "双引号未闭合");
    if (started)
        out.push_back(token);
    return out;
}
static string rejoin(const std::vector<string> &args, size_t start) {
    std::ostringstream o;
    for (size_t i = start; i < args.size(); ++i)
        o << std::quoted(args[i]) << ' ';
    return o.str();
}
static std::vector<string> commandArguments(int argc, char **argv) {
    std::vector<string> out;
#ifdef _WIN32
    (void)argc;
    (void)argv;
    int wideCount = 0;
    wchar_t **wideArgs = CommandLineToArgvW(GetCommandLineW(), &wideCount);
    require(wideArgs != nullptr, "无法读取 Unicode 启动参数");
    struct WideArgumentsOwner {
        wchar_t **value;
        ~WideArgumentsOwner() {
            LocalFree(value);
        }
    } owner{wideArgs};
    out.reserve(static_cast<size_t>(wideCount));
    for (int i = 0; i < wideCount; ++i) {
        int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wideArgs[i], -1, nullptr, 0,
                                       nullptr, nullptr);
        require(size > 0, "启动参数不是有效 Unicode");
        string arg(static_cast<size_t>(size), '\0');
        require(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wideArgs[i], -1, arg.data(),
                                    size, nullptr, nullptr) == size,
                "启动参数 UTF-8 转换失败");
        arg.pop_back();
        out.push_back(std::move(arg));
    }
#else
    for (int i = 0; i < argc; ++i)
        out.emplace_back(argv[i]);
#endif
    return out;
}
int main(int argc, char **argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    try {
        const auto arguments = commandArguments(argc, argv);
        fs::path data = "data";
        for (size_t i = 1; i < arguments.size(); ++i) {
            string arg = arguments[i];
            if (arg == "--self-test")
                return selfTest();
            if (arg == "--data") {
                require(i + 1 < arguments.size(), "--data 缺少路径");
                data = fs::u8path(arguments[++i]);
            } else if (arg == "--help") {
                std::cout << HELP;
                return 0;
            } else
                throw std::runtime_error("未知启动参数：" + arg);
        }
        SchoolService service(data);
        Session session;
        std::cout << "学生成绩管理系统 B | 模拟 AI 为主 + 人工辅助\n输入 help "
                     "查看完整中文指令；数据目录："
                  << fs::absolute(data).u8string() << '\n';
        string line;
        while (std::cout << '[' << (session.login.empty() ? "未登录" : session.login) << "]> " &&
               std::getline(std::cin, line)) {
            std::vector<string> v;
            string cmd = "parse";
            try {
                v = tokenize(line);
                if (v.empty())
                    continue;
                cmd = v[0];
                auto arity = [&](size_t lo, size_t hi) {
                    require(v.size() >= lo && v.size() <= hi, "参数数量不正确，请输入 help");
                };
                auto sortAsc = [&](size_t i) {
                    if (v.size() <= i)
                        return false;
                    require(v[i] == "asc" || v[i] == "desc", "排序必须为 asc 或 desc");
                    return v[i] == "asc";
                };
                auto showPath = [](const fs::path &p) {
                    std::cout << "已保存：" << fs::absolute(p).u8string() << '\n';
                };
                if (cmd == "help") {
                    arity(1, 1);
                    std::cout << HELP;
                } else if (cmd == "quit") {
                    arity(1, 1);
                    if (!session.login.empty())
                        service.logout(session);
                    break;
                } else if (cmd == "login") {
                    arity(3, 3);
                    session = service.login(v[1], v[2]);
                    std::cout << "登录成功。\n";
                } else if (cmd == "logout") {
                    arity(1, 1);
                    service.logout(session);
                    session = {};
                    std::cout << "已退出登录。\n";
                } else if (cmd == "me") {
                    arity(1, 1);
                    std::cout << service.me(session);
                } else if (cmd == "catalog") {
                    arity(1, 1);
                    std::cout << service.catalog(session);
                } else if (cmd == "passwd") {
                    arity(3, 3);
                    service.changePassword(session, v[1], v[2]);
                    std::cout << "密码已修改。\n";
                } else if (cmd == "history") {
                    arity(1, 1);
                    std::cout << service.logs(session, "", true);
                } else if (cmd == "grades") {
                    arity(1, 2);
                    std::cout << service.ownGrades(session, v.size() == 2 ? v[1] : "all");
                } else if (cmd == "users") {
                    arity(1, 1);
                    std::cout << service.accounts(session);
                } else if (cmd == "add-student") {
                    arity(9, 9);
                    service.addUsers(session, "student " + rejoin(v, 1));
                    std::cout << "学生账号已创建。\n";
                } else if (cmd == "add-teacher") {
                    arity(6, 6);
                    service.addUsers(session, "teacher " + rejoin(v, 1));
                    std::cout << "教师账号已创建。\n";
                } else if (cmd == "import-users") {
                    arity(2, 2);
                    service.importUsers(session, fs::u8path(v[1]));
                    std::cout << "账号整批导入成功。\n";
                } else if (cmd == "user-enable" || cmd == "user-disable" || cmd == "user-delete") {
                    arity(cmd == "user-delete" ? 3 : 2, cmd == "user-delete" ? 3 : 2);
                    service.usersAction(session, cmd.substr(5), split(v[1]),
                                        v.size() == 3 && v[2] == "CONFIRM");
                    std::cout << "批量账号操作成功。\n";
                } else if (cmd == "reset-password") {
                    arity(3, 3);
                    service.resetPassword(session, v[1], v[2]);
                    std::cout << "密码已重置。\n";
                } else
                    throw std::runtime_error("未知命令，请输入 help");
            } catch (const std::exception &e) {
                std::cerr << "错误：" << e.what() << '\n';
                try {
                    service.failure(session, identifier(cmd) ? cmd : "invalid-command");
                } catch (const std::exception &logError) {
                    std::cerr << "日志写入失败：" << logError.what() << '\n';
                }
            }
        }
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "启动失败：" << e.what() << '\n';
        return 1;
    }
}
