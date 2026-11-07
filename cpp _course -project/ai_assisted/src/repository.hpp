#pragma once
#include "sha256.hpp"
namespace sg {
inline void validate(const State &s) {
    size_t admins = 0;
    for (const auto &[id, c] : s.classes)
        require(identifier(id) && c.id == id && clean(c.name), "班级数据非法");
    for (const auto &[id, c] : s.subjects)
        require(identifier(id) && c.id == id && clean(c.name), "科目数据非法");
    for (const auto &[id, v] : s.students) {
        require(identifier(id) && v.id == id && identifier(v.account) && clean(v.name) &&
                    clean(v.gender) && clean(v.year) && clean(v.contact) &&
                    s.classes.count(v.classId),
                "学生字段或班级非法");
        auto a = s.accounts.find(v.account);
        require(a != s.accounts.end() && a->second.role == Role::Student && a->second.ref == id,
                "学生账号关联错误");
    }
    for (const auto &[id, v] : s.teachers) {
        require(identifier(id) && v.id == id && identifier(v.account) && clean(v.name),
                "教师字段非法");
        auto a = s.accounts.find(v.account);
        require(a != s.accounts.end() && a->second.role == Role::Teacher && a->second.ref == id,
                "教师账号关联错误");
        for (const auto &c : v.classes)
            require(s.classes.count(c), "教师关联班级不存在");
    }
    auto hex = [](const string &v) {
        return std::all_of(v.begin(), v.end(), [](unsigned char c) { return std::isxdigit(c); });
    };
    for (const auto &[id, a] : s.accounts) {
        require(identifier(id) && id == a.login && a.salt.size() >= 16 && a.salt.size() <= 128 &&
                    hex(a.salt) && a.hash.size() == 64 && hex(a.hash),
                "账号或密码摘要非法");
        if (a.role == Role::Admin) {
            require(a.ref == "-", "管理员关联非法");
            if (a.enabled)
                ++admins;
        } else if (a.role == Role::Student)
            require(s.students.count(a.ref) && s.students.at(a.ref).account == id, "孤立学生账号");
        else
            require(s.teachers.count(a.ref) && s.teachers.at(a.ref).account == id, "孤立教师账号");
    }
    require(admins > 0, "必须保留至少一个启用的管理员");
    for (const auto &[key, n] : s.grades)
        require(s.students.count(key.first) && s.subjects.count(key.second) && std::isfinite(n) &&
                    n >= 0 && n <= 100,
                "成绩引用或数值非法");
}
inline string encode(const State &s) {
    validate(s);
    std::ostringstream o;
    o << "SGM_B_1\n" << std::setprecision(17);
    for (const auto &[id, c] : s.classes)
        o << "C " << std::quoted(id) << ' ' << std::quoted(c.name) << '\n';
    for (const auto &[id, c] : s.subjects)
        o << "U " << std::quoted(id) << ' ' << std::quoted(c.name) << '\n';
    for (const auto &[id, v] : s.students)
        o << "S " << std::quoted(id) << ' ' << std::quoted(v.account) << ' ' << std::quoted(v.name)
          << ' ' << std::quoted(v.gender) << ' ' << std::quoted(v.year) << ' '
          << std::quoted(v.classId) << ' ' << std::quoted(v.contact) << '\n';
    for (const auto &[id, v] : s.teachers) {
        o << "T " << std::quoted(id) << ' ' << std::quoted(v.account) << ' ' << std::quoted(v.name)
          << ' ' << v.classes.size();
        for (const auto &c : v.classes)
            o << ' ' << std::quoted(c);
        o << '\n';
    }
    for (const auto &[id, a] : s.accounts)
        o << "A " << std::quoted(id) << ' ' << roleName(a.role) << ' ' << a.enabled << ' '
          << std::quoted(a.ref) << ' ' << std::quoted(a.salt) << ' ' << std::quoted(a.hash) << '\n';
    for (const auto &[k, n] : s.grades)
        o << "G " << std::quoted(k.first) << ' ' << std::quoted(k.second) << ' ' << n << '\n';
    string b = o.str();
    return b + "END " + sha256(b) + "\n";
}
inline State decode(const string &text) {
    require(text.size() < 20 * 1024 * 1024, "数据文件超过 20 MB 限制");
    auto pos = text.rfind("END ");
    require(pos != string::npos && pos > 0 && text.back() == '\n', "文件缺失校验尾部");
    string body = text.substr(0, pos), tail = text.substr(pos + 4);
    require(tail == sha256(body) + "\n", "数据校验失败，文件损坏或被改写");
    State s;
    std::istringstream in(body);
    string line;
    std::getline(in, line);
    require(line == "SGM_B_1", "数据版本不兼容");
    while (std::getline(in, line)) {
        std::istringstream r(line);
        string tag;
        r >> tag;
        if (tag == "C") {
            SchoolClass v;
            r >> std::quoted(v.id) >> std::quoted(v.name);
            require(s.classes.emplace(v.id, v).second, "重复班级");
        } else if (tag == "U") {
            Subject v;
            r >> std::quoted(v.id) >> std::quoted(v.name);
            require(s.subjects.emplace(v.id, v).second, "重复科目");
        } else if (tag == "S") {
            Student v;
            r >> std::quoted(v.id) >> std::quoted(v.account) >> std::quoted(v.name) >>
                std::quoted(v.gender) >> std::quoted(v.year) >> std::quoted(v.classId) >>
                std::quoted(v.contact);
            require(s.students.emplace(v.id, v).second, "重复学生");
        } else if (tag == "T") {
            Teacher v;
            size_t n = 0;
            r >> std::quoted(v.id) >> std::quoted(v.account) >> std::quoted(v.name) >> n;
            require(n < 10000, "教师班级数非法");
            for (size_t i = 0; i < n; ++i) {
                string c;
                r >> std::quoted(c);
                require(v.classes.insert(c).second, "重复任教班级");
            }
            require(s.teachers.emplace(v.id, v).second, "重复教师");
        } else if (tag == "A") {
            Account a;
            string role;
            int enabled = -1;
            r >> std::quoted(a.login) >> role >> enabled >> std::quoted(a.ref) >>
                std::quoted(a.salt) >> std::quoted(a.hash);
            a.role = parseRole(role);
            require(enabled == 0 || enabled == 1, "账号状态非法");
            a.enabled = enabled == 1;
            require(s.accounts.emplace(a.login, a).second, "重复账号");
        } else if (tag == "G") {
            string st, su, n;
            r >> std::quoted(st) >> std::quoted(su) >> n;
            double v = scoreNumber(n);
            require(s.grades.emplace(std::make_pair(st, su), v).second, "重复成绩");
        } else
            throw std::runtime_error("未知数据行");
        require(!r.fail(), "数据字段缺失");
        string extra;
        require(!(r >> extra), "数据字段过多");
    }
    validate(s);
    return s;
}
inline string readFile(const fs::path &p) {
    std::ifstream f(p, std::ios::binary);
    require(bool(f), "无法读取文件：" + p.u8string());
    std::ostringstream o;
    o << f.rdbuf();
    require(!f.bad(), "读取失败");
    return o.str();
}
inline void atomicWrite(const fs::path &p, const string &content) {
    fs::create_directories(p.parent_path());
    auto tmp = p;
    tmp += ".tmp";
    auto previous = p;
    previous += ".previous";
    {
        std::ofstream o(tmp, std::ios::binary | std::ios::trunc);
        require(bool(o), "无法写入文件");
        o << content;
        o.flush();
        require(bool(o), "写入失败，可能磁盘空间不足");
    }
    bool existed = fs::exists(p);
    if (fs::exists(previous))
        fs::remove(previous);
    if (existed)
        fs::rename(p, previous);
    try {
        fs::rename(tmp, p);
    } catch (...) {
        if (existed && fs::exists(previous))
            fs::rename(previous, p);
        throw;
    }
    if (fs::exists(previous))
        fs::remove(previous);
}
struct Log {
    string time, user, role, action;
};
class Repository {
    fs::path root_;

  public:
    explicit Repository(fs::path root) : root_(std::move(root)) {
        fs::create_directories(root_);
        auto db = root_ / "school.db", prev = root_ / "school.db.previous";
        if (!fs::exists(db) && fs::exists(prev)) {
            decode(readFile(prev));
            fs::rename(prev, db);
        }
    }
    const fs::path &root() const {
        return root_;
    }
    bool exists() const {
        return fs::exists(root_ / "school.db");
    }
    State load() const {
        return decode(readFile(root_ / "school.db"));
    }
    void save(const State &s) {
        atomicWrite(root_ / "school.db", encode(s));
    }
    void audit(const string &user, const string &role, const string &action) {
        std::ofstream o(root_ / "audit.log", std::ios::binary | std::ios::app);
        require(bool(o), "无法写入操作日志");
        o << std::quoted(now()) << ' ' << std::quoted(user) << ' ' << std::quoted(role) << ' '
          << std::quoted(action) << '\n';
        o.flush();
        require(bool(o), "操作日志写入失败");
    }
    std::vector<Log> logs() const {
        std::vector<Log> out;
        std::ifstream f(root_ / "audit.log", std::ios::binary);
        string line;
        while (std::getline(f, line)) {
            Log l;
            std::istringstream s(line);
            s >> std::quoted(l.time) >> std::quoted(l.user) >> std::quoted(l.role) >>
                std::quoted(l.action);
            require(!s.fail(), "日志文件格式损坏");
            out.push_back(l);
        }
        return out;
    }
    void clearLogs() {
        atomicWrite(root_ / "audit.log", "");
    }
    fs::path artifact(const string &kind, const string &scope, const string &ext,
                      const string &text) {
        require(identifier(kind) && identifier(scope) && identifier(ext), "报表标识非法");
        auto dir = root_ / "reports" / kind;
        fs::create_directories(dir);
        auto p = dir / (scope + "_" + stamp() + "." + ext);
        for (int n = 1; fs::exists(p); ++n)
            p = dir / (scope + "_" + stamp() + "_" + std::to_string(n) + "." + ext);
        atomicWrite(p, text);
        return p;
    }
    fs::path backup(const State &s) {
        auto dir = root_ / "backups";
        fs::create_directories(dir);
        auto p = dir / ("school_" + stamp() + ".bak");
        for (int n = 1; fs::exists(p); ++n)
            p = dir / ("school_" + stamp() + "_" + std::to_string(n) + ".bak");
        atomicWrite(p, encode(s));
        if (fs::exists(root_ / "audit.log"))
            atomicWrite(fs::path(p.u8string() + ".log"), readFile(root_ / "audit.log"));
        return p;
    }
};
inline State demoState() {
    State s;
    s.classes = {{"C1", {"C1", "计算机一班"}}, {"C2", {"C2", "计算机二班"}}};
    s.subjects = {
        {"CPP", {"CPP", "C++程序设计"}}, {"ENG", {"ENG", "英语"}}, {"MATH", {"MATH", "高等数学"}}};
    auto account = [&](string login, Role role, string ref, string pwd) {
        Account a;
        a.login = login;
        a.role = role;
        a.ref = ref;
        setPassword(a, pwd);
        s.accounts.emplace(login, a);
    };
    account("admin", Role::Admin, "-", "Admin@123");
    account("teacher1", Role::Teacher, "T001", "Teach@123");
    account("teacher2", Role::Teacher, "T002", "Teach@123");
    s.teachers = {{"T001", {"T001", "teacher1", "陈老师", {"C1"}}},
                  {"T002", {"T002", "teacher2", "周老师", {"C2"}}}};
    std::vector<string> names = {"张三", "李四", "王五", "赵六"};
    for (int i = 1; i <= 4; ++i) {
        string id = "S00" + std::to_string(i), login = "stu" + std::to_string(i);
        s.students[id] = {
            id, login, names[i - 1], i == 2 ? "女" : "男", "2025", i == 4 ? "C2" : "C1", "未填写"};
        account(login, Role::Student, id, "Stu@123");
    }
    s.grades = {{{"S001", "MATH"}, 90}, {{"S001", "CPP"}, 80}, {{"S001", "ENG"}, 70},
                {{"S002", "MATH"}, 60}, {{"S002", "CPP"}, 80}, {{"S002", "ENG"}, 100},
                {{"S003", "MATH"}, 50}, {{"S003", "CPP"}, 60}, {{"S004", "MATH"}, 85},
                {{"S004", "CPP"}, 75},  {{"S004", "ENG"}, 95}};
    return s;
}
} // namespace sg
