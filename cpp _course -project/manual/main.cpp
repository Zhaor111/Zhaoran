// 方案 A：模拟学生手工设计的清晰直观实现；实际由 AI 辅助生成并验证。
// C++17，标准库，无数据库、无第三方依赖。详见 README.md。
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#ifdef _MSC_VER
#pragma comment(lib, "shell32.lib")
#endif
#include <shellapi.h>
#endif
namespace fs = std::filesystem;
using namespace std;

// 自包含 SHA-256，仅用于教学演示；生产环境应使用经过审计的 Argon2id 等实现。
static string sha256(const string &src) {
    static const uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4,
        0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe,
        0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f,
        0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc,
        0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
        0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116,
        0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7,
        0xc67178f2};
    auto r = [](uint32_t x, int n) { return (x >> n) | (x << (32 - n)); };
    vector<unsigned char> v(src.begin(), src.end());
    uint64_t bits = uint64_t(v.size()) * 8;
    v.push_back(0x80);
    while (v.size() % 64 != 56)
        v.push_back(0);
    for (int i = 7; i >= 0; --i)
        v.push_back(static_cast<unsigned char>(bits >> (8 * i)));
    uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                     0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    for (size_t z = 0; z < v.size(); z += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i)
            w[i] = (uint32_t(v[z + i * 4]) << 24) | (uint32_t(v[z + i * 4 + 1]) << 16) |
                   (uint32_t(v[z + i * 4 + 2]) << 8) | v[z + i * 4 + 3];
        for (int i = 16; i < 64; ++i) {
            uint32_t a = r(w[i - 15], 7) ^ r(w[i - 15], 18) ^ (w[i - 15] >> 3),
                     b = r(w[i - 2], 17) ^ r(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + a + w[i - 7] + b;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], q = h[7];
        for (int i = 0; i < 64; ++i) {
            uint32_t t = q + (r(e, 6) ^ r(e, 11) ^ r(e, 25)) + ((e & f) ^ (~e & g)) + k[i] + w[i],
                     u = (r(a, 2) ^ r(a, 13) ^ r(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
            q = g;
            g = f;
            f = e;
            e = d + t;
            d = c;
            c = b;
            b = a;
            a = t + u;
        }
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += q;
    }
    ostringstream o;
    for (auto x : h)
        o << hex << setfill('0') << setw(8) << x;
    return o.str();
}
static string salt() {
    random_device rd;
    ostringstream s;
    for (int i = 0; i < 4; ++i)
        s << hex << setfill('0') << setw(8) << rd();
    return s.str();
}
static string digest(const string &pass, const string &s) {
    string d = s + ":" + pass;
    for (int i = 0; i < 2000; ++i)
        d = sha256(d + s);
    return d;
}
static bool equalHash(const string &a, const string &b) {
    unsigned d = unsigned(a.size() ^ b.size());
    for (size_t i = 0; i < min(a.size(), b.size()); ++i)
        d |= unsigned(a[i] ^ b[i]);
    return d == 0;
}
static void need(bool yes, const string &message) {
    if (!yes)
        throw runtime_error(message);
}
static string now() {
    auto t = chrono::system_clock::to_time_t(chrono::system_clock::now());
    tm p{};
#ifdef _WIN32
    localtime_s(&p, &t);
#else
    localtime_r(&t, &p);
#endif
    ostringstream o;
    o << put_time(&p, "%Y-%m-%d %H:%M:%S");
    return o.str();
}
static string stamp() {
    string x = now();
    for (char &c : x)
        if (c == ' ' || c == ':' || c == '-')
            c = '_';
    return x;
}
static bool idok(const string &s) {
    return !s.empty() && s.size() <= 40 && all_of(s.begin(), s.end(), [](unsigned char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
               c == '_' || c == '-';
    });
}
static bool textok(const string &s) {
    return !s.empty() && s.size() <= 180 &&
           none_of(s.begin(), s.end(), [](unsigned char c) { return c < 32; });
}
static void passok(const string &s) {
    need(s.size() >= 6 && s.size() <= 128, "密码长度必须为 6~128 字节");
    need(none_of(s.begin(), s.end(), [](unsigned char c) { return c < 32; }), "密码不能含控制字符");
}
static double number(const string &s, double lo, double hi) {
    size_t p = 0;
    double v;
    try {
        v = stod(s, &p);
    } catch (...) {
        throw runtime_error("不是合法数字：" + s);
    }
    need(p == s.size() && isfinite(v) && v >= lo && v <= hi, "数值范围错误：" + s);
    return v;
}
static string f2(double x) {
    ostringstream o;
    o << fixed << setprecision(2) << x;
    return o.str();
}
static vector<string> words(const string &line) {
    vector<string> v;
    string token;
    bool quotedToken = false, started = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '\\' && quotedToken && i + 1 < line.size() && line[i + 1] == '\"') {
            token += '\"';
            ++i;
            started = true;
        } else if (c == '\"') {
            quotedToken = !quotedToken;
            started = true;
        } else if (!quotedToken && (c == ' ' || c == '\t' || c == '\r' || c == '\n')) {
            if (started) {
                v.push_back(token);
                token.clear();
                started = false;
            }
        } else {
            token += c;
            started = true;
        }
    }
    need(!quotedToken, "引号未闭合");
    if (started)
        v.push_back(token);
    return v;
}
static vector<string> split(const string &s) {
    vector<string> r;
    istringstream in(s);
    string x;
    while (getline(in, x, ',')) {
        need(idok(x), "班级编号格式错误");
        r.push_back(x);
    }
    need(!r.empty(), "至少填写一个班级");
    return r;
}
static string xml(string s) {
    string o;
    for (char c : s) {
        if (c == '&')
            o += "&amp;";
        else if (c == '<')
            o += "&lt;";
        else if (c == '>')
            o += "&gt;";
        else if (c == '\"')
            o += "&quot;";
        else
            o += c;
    }
    return o;
}

struct User {
    string name, role, ref, salt, hash;
    bool enabled = true;
};
struct Student {
    string id, name, sex, grade, cls, phone;
};
struct Teacher {
    string id, name, phone;
    vector<string> classes;
};
struct ClassRoom {
    string id, name, grade;
};
struct Subject {
    string id, name;
    double credit = 1;
};
struct Score {
    string sid, sub;
    double value = 0;
};
struct Log {
    string time, actor, role, action;
};
struct Data {
    vector<User> users;
    vector<Student> students;
    vector<Teacher> teachers;
    vector<ClassRoom> classes;
    vector<Subject> subjects;
    vector<Score> scores;
    vector<Log> logs;
};
template <class T> static T *byId(vector<T> &v, const string &id) {
    auto p = find_if(v.begin(), v.end(), [&](const T &x) { return x.id == id; });
    return p == v.end() ? nullptr : &*p;
}
template <class T> static const T *byId(const vector<T> &v, const string &id) {
    auto p = find_if(v.begin(), v.end(), [&](const T &x) { return x.id == id; });
    return p == v.end() ? nullptr : &*p;
}
static User *user(Data &d, const string &name) {
    auto p = find_if(d.users.begin(), d.users.end(), [&](const User &u) { return u.name == name; });
    return p == d.users.end() ? nullptr : &*p;
}
static const User *user(const Data &d, const string &name) {
    auto p = find_if(d.users.begin(), d.users.end(), [&](const User &u) { return u.name == name; });
    return p == d.users.end() ? nullptr : &*p;
}
static User makeUser(const string &name, const string &password, const string &role,
                     const string &ref) {
    need(idok(name), "账号只支持字母数字下划线短横线");
    passok(password);
    User u{name, role, ref, salt(), "", true};
    u.hash = digest(password, u.salt);
    return u;
}
static bool hexstr(const string &s, size_t n) {
    return s.size() == n && all_of(s.begin(), s.end(), [](unsigned char c) {
               return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
           });
}
static void validate(const Data &d) {
    auto uniqueIds = [](const auto &rows) {
        set<string> keys;
        for (const auto &x : rows)
            need(idok(x.id) && keys.insert(x.id).second, "编号非法或重复：" + x.id);
    };
    uniqueIds(d.students);
    uniqueIds(d.teachers);
    uniqueIds(d.classes);
    uniqueIds(d.subjects);
    for (const auto &x : d.classes)
        need(textok(x.name) && textok(x.grade), "班级信息非法");
    for (const auto &x : d.students)
        need(textok(x.name) && textok(x.sex) && textok(x.grade) && textok(x.phone) &&
                 byId(d.classes, x.cls) && byId(d.classes, x.cls)->grade == x.grade,
             "学生信息或年级班级关系非法：" + x.id);
    for (const auto &x : d.teachers) {
        need(textok(x.name) && textok(x.phone) && !x.classes.empty(), "教师信息非法");
        set<string> ids;
        for (const auto &c : x.classes)
            need(byId(d.classes, c) && ids.insert(c).second, "教师班级不存在或重复");
    }
    for (const auto &x : d.subjects)
        need(textok(x.name) && isfinite(x.credit) && x.credit > 0 && x.credit <= 20,
             "科目信息非法");
    set<string> ids, refs;
    int admins = 0;
    for (const auto &x : d.users) {
        need(idok(x.name) && ids.insert(x.name).second, "账号重复或非法");
        need(hexstr(x.salt, 32) && hexstr(x.hash, 64), "密码存储格式非法");
        need(x.role == "admin" || x.role == "teacher" || x.role == "student", "未知角色");
        if (x.role == "admin") {
            need(x.ref == "-", "管理员引用错误");
            if (x.enabled)
                ++admins;
        }
        if (x.role == "student")
            need(byId(d.students, x.ref) && refs.insert(x.role + ":" + x.ref).second,
                 "学生账号关系错误");
        if (x.role == "teacher")
            need(byId(d.teachers, x.ref) && refs.insert(x.role + ":" + x.ref).second,
                 "教师账号关系错误");
    }
    need(admins > 0, "必须保留至少一个启用的管理员");
    set<pair<string, string>> sk;
    for (const auto &x : d.scores)
        need(byId(d.students, x.sid) && byId(d.subjects, x.sub) && isfinite(x.value) &&
                 x.value >= 0 && x.value <= 100 && sk.insert({x.sid, x.sub}).second,
             "成绩存在非法值、引用或重复");
    for (const auto &x : d.logs)
        need(textok(x.time) && textok(x.actor) && textok(x.role) && !x.action.empty() &&
                 x.action.size() <= 4096 &&
                 none_of(x.action.begin(), x.action.end(), [](unsigned char c) { return c < 32; }),
             "日志格式非法");
}
static string serialize(const Data &d) {
    ostringstream o;
    o << setprecision(17) << "SCHOOL_A_V1\n";
    o << "USERS " << d.users.size() << '\n';
    for (auto &x : d.users)
        o << quoted(x.name) << ' ' << quoted(x.role) << ' ' << quoted(x.ref) << ' '
          << quoted(x.salt) << ' ' << quoted(x.hash) << ' ' << x.enabled << '\n';
    o << "STUDENTS " << d.students.size() << '\n';
    for (auto &x : d.students)
        o << quoted(x.id) << ' ' << quoted(x.name) << ' ' << quoted(x.sex) << ' ' << quoted(x.grade)
          << ' ' << quoted(x.cls) << ' ' << quoted(x.phone) << '\n';
    o << "TEACHERS " << d.teachers.size() << '\n';
    for (auto &x : d.teachers) {
        o << quoted(x.id) << ' ' << quoted(x.name) << ' ' << quoted(x.phone) << ' '
          << x.classes.size();
        for (auto &c : x.classes)
            o << ' ' << quoted(c);
        o << '\n';
    }
    o << "CLASSES " << d.classes.size() << '\n';
    for (auto &x : d.classes)
        o << quoted(x.id) << ' ' << quoted(x.name) << ' ' << quoted(x.grade) << '\n';
    o << "SUBJECTS " << d.subjects.size() << '\n';
    for (auto &x : d.subjects)
        o << quoted(x.id) << ' ' << quoted(x.name) << ' ' << x.credit << '\n';
    o << "SCORES " << d.scores.size() << '\n';
    for (auto &x : d.scores)
        o << quoted(x.sid) << ' ' << quoted(x.sub) << ' ' << x.value << '\n';
    o << "LOGS " << d.logs.size() << '\n';
    for (auto &x : d.logs)
        o << quoted(x.time) << ' ' << quoted(x.actor) << ' ' << quoted(x.role) << ' '
          << quoted(x.action) << '\n';
    o << "END\n";
    return o.str();
}
static void writeFile(const fs::path &path, const string &content) {
    fs::create_directories(path.parent_path());
    ofstream f(path, ios::binary | ios::trunc);
    need(bool(f), "无法写入文件：" + path.u8string());
    f << content;
    f.flush();
    need(bool(f), "文件写入失败");
}
static string readFile(const fs::path &path) {
    ifstream f(path, ios::binary);
    need(bool(f), "无法读取文件：" + path.u8string());
    ostringstream s;
    s << f.rdbuf();
    need(!f.bad(), "文件读取失败");
    return s.str();
}
static Data parseData(const string &content) {
    auto k = content.rfind("CHECKSUM ");
    need(k != string::npos, "数据缺少校验和");
    string body = content.substr(0, k);
    string check = content.substr(k + 9);
    if (!check.empty() && check.back() == '\n')
        check.pop_back();
    need(check == sha256(body), "数据校验和不匹配");
    istringstream in(body);
    string tag;
    in >> tag;
    need(tag == "SCHOOL_A_V1", "不支持的数据版本");
    Data d;
    auto count = [&](const string &expected) {
        size_t n = 0;
        in >> tag >> n;
        need(bool(in) && tag == expected && n <= 1000000, "数据分区错误：" + expected);
        return n;
    };
    for (size_t i = 0, n = count("USERS"); i < n; ++i) {
        User x;
        int e = -1;
        in >> quoted(x.name) >> quoted(x.role) >> quoted(x.ref) >> quoted(x.salt) >>
            quoted(x.hash) >> e;
        need(e == 0 || e == 1, "启用标志非法");
        x.enabled = e == 1;
        d.users.push_back(x);
    }
    for (size_t i = 0, n = count("STUDENTS"); i < n; ++i) {
        Student x;
        in >> quoted(x.id) >> quoted(x.name) >> quoted(x.sex) >> quoted(x.grade) >> quoted(x.cls) >>
            quoted(x.phone);
        d.students.push_back(x);
    }
    for (size_t i = 0, n = count("TEACHERS"); i < n; ++i) {
        Teacher x;
        size_t m = 0;
        in >> quoted(x.id) >> quoted(x.name) >> quoted(x.phone) >> m;
        need(m <= 10000, "教师班级数异常");
        for (size_t j = 0; j < m; ++j) {
            string c;
            in >> quoted(c);
            x.classes.push_back(c);
        }
        d.teachers.push_back(x);
    }
    for (size_t i = 0, n = count("CLASSES"); i < n; ++i) {
        ClassRoom x;
        in >> quoted(x.id) >> quoted(x.name) >> quoted(x.grade);
        d.classes.push_back(x);
    }
    for (size_t i = 0, n = count("SUBJECTS"); i < n; ++i) {
        Subject x;
        in >> quoted(x.id) >> quoted(x.name) >> x.credit;
        d.subjects.push_back(x);
    }
    for (size_t i = 0, n = count("SCORES"); i < n; ++i) {
        Score x;
        in >> quoted(x.sid) >> quoted(x.sub) >> x.value;
        d.scores.push_back(x);
    }
    for (size_t i = 0, n = count("LOGS"); i < n; ++i) {
        Log x;
        in >> quoted(x.time) >> quoted(x.actor) >> quoted(x.role) >> quoted(x.action);
        d.logs.push_back(x);
    }
    in >> tag;
    need(bool(in) && tag == "END", "数据不完整");
    in >> ws;
    need(in.eof(), "数据有多余内容");
    validate(d);
    return d;
}
static Data demo() {
    Data d;
    d.classes = {{"C1", "软件一班", "2025"}, {"C2", "软件二班", "2025"}};
    d.subjects = {{"MATH", "高等数学", 1}, {"CPP", "C++程序设计", 1}, {"ENG", "大学英语", 1}};
    d.students = {{"S001", "张三", "男", "2025", "C1", "13800000001"},
                  {"S002", "李四", "女", "2025", "C1", "13800000002"},
                  {"S003", "王五", "男", "2025", "C1", "13800000003"},
                  {"S004", "赵六", "女", "2025", "C2", "13800000004"}};
    d.teachers = {{"T001", "陈老师", "13900000001", {"C1"}},
                  {"T002", "刘老师", "13900000002", {"C2"}}};
    d.users = {makeUser("admin", "Admin@123", "admin", "-"),
               makeUser("teacher1", "Teach@123", "teacher", "T001"),
               makeUser("teacher2", "Teach@123", "teacher", "T002"),
               makeUser("stu1", "Stu@123", "student", "S001"),
               makeUser("stu2", "Stu@123", "student", "S002"),
               makeUser("stu3", "Stu@123", "student", "S003"),
               makeUser("stu4", "Stu@123", "student", "S004")};
    d.scores = {{"S001", "MATH", 90}, {"S001", "CPP", 80}, {"S001", "ENG", 70},
                {"S002", "MATH", 60}, {"S002", "CPP", 80}, {"S002", "ENG", 100},
                {"S003", "MATH", 50}, {"S003", "CPP", 60}, {"S004", "MATH", 88},
                {"S004", "CPP", 92},  {"S004", "ENG", 76}};
    return d;
}

class SchoolApp {
    Data d;
    fs::path root;
    string active;
    unsigned serial = 0;
    const User &current() const {
        auto p = user(d, active);
        need(p && p->enabled, "请先登录；账号可能已被禁用");
        return *p;
    }
    void admin() const { need(current().role == "admin", "权限不足：仅管理员可操作"); }
    void staff() const { need(current().role != "student", "权限不足：学生仅可查看本人数据"); }
    void classAccess(const string &cls) const {
        need(byId(d.classes, cls), "班级不存在");
        const auto &u = current();
        if (u.role == "admin")
            return;
        need(u.role == "teacher", "权限不足：不能查看班级明细");
        auto t = byId(d.teachers, u.ref);
        need(t && find(t->classes.begin(), t->classes.end(), cls) != t->classes.end(),
             "权限不足：该班级不在管辖范围");
    }
    const Student &studentAccess(const string &id) const {
        auto s = byId(d.students, id);
        need(s, "学生不存在");
        if (current().role == "student")
            need(current().ref == id, "权限不足：只能查看本人");
        else
            classAccess(s->cls);
        return *s;
    }
    static void sizeIs(const vector<string> &a, size_t n) {
        need(a.size() == n, "参数数量错误，请输入 help 查看格式");
    }
    static void confirm(const string &x) {
        need(x == "CONFIRM", "重要操作需要最后一个参数明确填写 CONFIRM");
    }
    fs::path outputPath(const string &category, const string &scope, const string &ext) {
        fs::path dir = root / "reports" / category;
        fs::create_directories(dir);
        fs::path p;
        do {
            p = dir / fs::u8path(category + "_" + scope + "_" + stamp() + "_" +
                                 to_string(++serial) + ext);
        } while (fs::exists(p));
        return p;
    }
    const Score *score(const string &id, const string &sub) const {
        auto p = find_if(d.scores.begin(), d.scores.end(),
                         [&](const Score &s) { return s.sid == id && s.sub == sub; });
        return p == d.scores.end() ? nullptr : &*p;
    }
    bool complete(const string &id) const {
        if (d.subjects.empty())
            return false;
        for (auto &s : d.subjects)
            if (!score(id, s.id))
                return false;
        return true;
    }
    double total(const string &id) const {
        double n = 0;
        for (auto &x : d.scores)
            if (x.sid == id)
                n += x.value;
        return n;
    }
    size_t recorded(const string &id) const {
        return count_if(d.scores.begin(), d.scores.end(),
                        [&](const Score &x) { return x.sid == id; });
    }
    double gpa(const string &id) const {
        double points = 0, credits = 0;
        for (auto &s : d.subjects)
            if (auto p = score(id, s.id)) {
                points += (p->value < 60 ? 0 : min(4.0, max(0.0, (p->value - 50) / 10))) * s.credit;
                credits += s.credit;
            }
        return credits ? points / credits : 0;
    }
    vector<pair<string, double>> ranking(const string &cls, const string &metric, bool asc) const {
        need(metric == "total" || metric == "average" || byId(d.subjects, metric),
             "排名科目不存在");
        vector<pair<string, double>> r;
        for (auto &s : d.students)
            if (s.cls == cls) {
                if (metric == "total" || metric == "average") {
                    if (complete(s.id))
                        r.push_back(
                            {s.id,
                             total(s.id) / (metric == "average" ? double(d.subjects.size()) : 1)});
                } else if (auto p = score(s.id, metric))
                    r.push_back({s.id, p->value});
            }
        sort(r.begin(), r.end(), [&](auto &a, auto &b) {
            if (a.second == b.second)
                return a.first < b.first;
            return asc ? a.second < b.second : a.second > b.second;
        });
        return r;
    }
    string rankOf(const string &sid, const string &metric) const {
        auto s = byId(d.students, sid);
        auto r = ranking(s->cls, metric, false);
        size_t rank = 0;
        double groupTop = 0;
        for (size_t i = 0; i < r.size(); ++i) {
            // Keep sorting exact, but absorb floating-point summation noise when
            // assigning ranks. Anchor each tie group to its highest value so
            // a chain of adjacent near-equalities cannot merge distinct groups.
            if (i == 0 || fabs(r[i].second - groupTop) > 1e-9) {
                rank = i + 1;
                groupTop = r[i].second;
            }
            if (r[i].first == sid)
                return to_string(rank);
        }
        return "待齐全/未录入";
    }
    string personal(const string &id, const string &only = "all") const {
        auto &s = studentAccess(id);
        need(only == "all" || byId(d.subjects, only), "科目不存在");
        ostringstream o;
        o << "学生成绩报表 | 生成时间 " << now() << "\n学号 " << s.id << " 姓名 " << s.name
          << " 性别 " << s.sex << " 年级 " << s.grade << " 班级 " << s.cls << " 联系方式 "
          << s.phone << '\n';
        for (auto &sub : d.subjects)
            if (only == "all" || sub.id == only) {
                auto p = score(id, sub.id);
                o << sub.id << ' ' << sub.name << " 成绩 " << (p ? f2(p->value) : "未录入")
                  << " 班级排名 " << rankOf(id, sub.id) << '\n';
            }
        if (only == "all") {
            size_t n = recorded(id);
            o << "已录入 " << n << '/' << d.subjects.size() << " 科，已录总分 " << f2(total(id))
              << " 已录平均分 " << (n ? f2(total(id) / double(n)) : "无") << '\n';
            if (complete(id))
                o << "全科 GPA " << f2(gpa(id)) << " 总分排名 " << rankOf(id, "total")
                  << " 平均分排名 " << rankOf(id, "average") << '\n';
            else
                o << "已录科目 GPA " << (n ? f2(gpa(id)) : "无")
                  << "（仅已录口径）\n成绩待齐全：全科 GPA、总分与平均分排名暂不计算\n";
        }
        return o.str();
    }
    string studentList(const string &cls, const string &query) const {
        staff();
        if (cls != "-")
            classAccess(cls);
        ostringstream o;
        o << "学号\t姓名\t性别\t年级\t班级\t联系方式\n";
        for (auto &s : d.students) {
            bool allowed = true;
            try {
                classAccess(s.cls);
            } catch (...) {
                allowed = false;
            }
            string all = s.id + " " + s.name + " " + s.phone;
            if (allowed && (cls == "-" || s.cls == cls) &&
                (query.empty() || all.find(query) != string::npos))
                o << s.id << '\t' << s.name << '\t' << s.sex << '\t' << s.grade << '\t' << s.cls
                  << '\t' << s.phone << '\n';
        }
        return o.str();
    }
    string classScores(const string &cls) const {
        classAccess(cls);
        ostringstream o;
        for (auto &s : d.students)
            if (s.cls == cls)
                o << personal(s.id) << '\n';
        return o.str();
    }
    string rankText(const string &cls, const string &metric, const string &order) const {
        classAccess(cls);
        need(order == "asc" || order == "desc", "排序必须为 asc 或 desc");
        auto r = ranking(cls, metric, order == "asc");
        ostringstream o;
        o << "班级 " << cls << " 指标 " << metric << " 顺序 " << order
          << "（同分竞争排名，名次始终高分在前）\n名次\t学号\t姓名\t数值\n";
        for (size_t i = 0; i < r.size(); ++i) {
            o << rankOf(r[i].first, metric) << '\t' << r[i].first << '\t'
              << byId(d.students, r[i].first)->name << '\t' << f2(r[i].second) << '\n';
        }
        for (auto &s : d.students)
            if (s.cls == cls && ((metric == "total" || metric == "average") ? !complete(s.id)
                                                                            : !score(s.id, metric)))
                o << "待齐全/未录入\t" << s.id << '\t' << s.name << '\n';
        return o.str();
    }
    struct Statistics {
        int enrolled = 0, n = 0, pass = 0;
        double sum = 0, high = 0, low = 100;
        array<int, 5> bands{};
    };
    Statistics stats(const string &cls, const string &sub) const {
        need(byId(d.subjects, sub), "科目不存在");
        Statistics s;
        for (auto &st : d.students)
            if (st.cls == cls) {
                ++s.enrolled;
                if (auto p = score(st.id, sub)) {
                    ++s.n;
                    s.sum += p->value;
                    s.high = max(s.high, p->value);
                    s.low = min(s.low, p->value);
                    if (p->value >= 60)
                        ++s.pass;
                    int b = p->value < 60   ? 0
                            : p->value < 70 ? 1
                            : p->value < 80 ? 2
                            : p->value < 90 ? 3
                                            : 4;
                    ++s.bands[b];
                }
            }
        return s;
    }
    string statsText(const string &cls, const string &sub = "all") const {
        classAccess(cls);
        need(sub == "all" || byId(d.subjects, sub), "科目不存在");
        ostringstream o;
        o << "班级 " << cls << " 学情统计 | " << now()
          << "\n及格率分母为已录入人数；缺考或未录入不按 0 分计算。\n";
        for (auto &x : d.subjects)
            if (sub == "all" || x.id == sub) {
                auto s = stats(cls, x.id);
                o << x.id << ' ' << x.name << " 在册=" << s.enrolled << " 已录=" << s.n
                  << " 未录=" << s.enrolled - s.n << " 平均=" << (s.n ? f2(s.sum / s.n) : "无")
                  << " 最高=" << (s.n ? f2(s.high) : "无") << " 最低=" << (s.n ? f2(s.low) : "无")
                  << " 及格=" << s.pass << " 不及格=" << s.n - s.pass
                  << " 及格率=" << (s.n ? f2(s.pass * 100.0 / s.n) + "%" : "无") << '\n';
            }
        return o.str();
    }
    string charts(const string &cls, const string &sub) {
        classAccess(cls);
        auto s = stats(cls, sub);
        string summary = statsText(cls, sub);
        fs::path svg = outputPath("charts", cls + "_" + sub, ".svg");
        fs::path txt = svg;
        txt.replace_extension(".txt");
        const char *colors[] = {"#e16b67", "#f4a261", "#e9c46a", "#48a9a6", "#4062bb"};
        const char *labels[] = {"[0,60)", "[60,70)", "[70,80)", "[80,90)", "[90,100]"};
        ostringstream o;
        o << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"1080\" height=\"650\" viewBox=\"0 "
             "0 1080 650\"><rect width=\"1080\" height=\"650\" fill=\"#f5f7fb\"/><g "
             "font-family=\"Microsoft YaHei,Arial,sans-serif\" fill=\"#22334d\"><text x=\"40\" "
             "y=\"50\" font-size=\"27\">"
          << xml(cls + " / " + sub + " 成绩分布")
          << "</text><text x=\"40\" y=\"82\" font-size=\"15\">仅统计已录入成绩；未录入 "
          << s.enrolled - s.n << " 人</text>";
        double start = -1.5707963267948966;
        const double pi = 3.14159265358979323846;
        if (s.n == 0)
            o << "<circle cx=\"240\" cy=\"280\" r=\"140\" fill=\"#dce3ee\"/><text x=\"198\" "
                 "y=\"286\" font-size=\"20\">暂无成绩</text>";
        for (int i = 0; i < 5; ++i) {
            double angle = s.n ? 2 * pi * s.bands[i] / s.n : 0, end = start + angle;
            if (s.bands[i] > 0) {
                if (s.bands[i] == s.n)
                    o << "<circle cx=\"240\" cy=\"280\" r=\"140\" fill=\"" << colors[i] << "\"/>";
                else
                    o << "<path d=\"M 240 280 L " << 240 + 140 * cos(start) << ' '
                      << 280 + 140 * sin(start) << " A 140 140 0 " << (angle > pi ? 1 : 0) << " 1 "
                      << 240 + 140 * cos(end) << ' ' << 280 + 140 * sin(end) << " Z\" fill=\""
                      << colors[i] << "\"/>";
            }
            start = end;
            double h =
                s.n ? 200.0 * s.bands[i] / max(1, *max_element(s.bands.begin(), s.bands.end())) : 0;
            int x = 520 + i * 98;
            o << "<rect x=\"" << x << "\" y=\"" << 400 - h << "\" width=\"65\" height=\"" << h
              << "\" fill=\"" << colors[i] << "\"/><text x=\"" << x + 26 << "\" y=\"" << 390 - h
              << "\" font-size=\"18\">" << s.bands[i] << "</text><text x=\"" << x - 3
              << "\" y=\"430\" font-size=\"13\">" << labels[i] << "</text><rect x=\""
              << 40 + i * 200 << "\" y=\"478\" width=\"16\" height=\"16\" fill=\"" << colors[i]
              << "\"/><text x=\"" << 62 + i * 200 << "\" y=\"492\" font-size=\"14\">" << labels[i]
              << "：" << s.bands[i] << " 人</text>";
        }
        o << "<text x=\"150\" y=\"450\" font-size=\"18\">分数段比例饼图</text><text x=\"666\" "
             "y=\"150\" font-size=\"18\">分数段人数柱状图</text><text x=\"40\" y=\"548\" "
             "font-size=\"17\">平均分 "
          << (s.n ? f2(s.sum / s.n) : "无") << "　最高分 " << (s.n ? f2(s.high) : "无")
          << "　最低分 " << (s.n ? f2(s.low) : "无")
          << "</text><text x=\"40\" y=\"581\" font-size=\"17\">及格率 "
          << (s.n ? f2(s.pass * 100.0 / s.n) + "%" : "无") << "　不及格 " << s.n - s.pass
          << " 人　已录入 " << s.n << " 人</text><text x=\"40\" y=\"621\" font-size=\"13\">"
          << xml(now()) << " | 学生成绩管理系统 A</text></g></svg>";
        writeFile(svg, o.str());
        writeFile(txt, summary);
        return "已生成图表：" + svg.u8string() + "\n已持久化统计：" + txt.u8string() + "\n";
    }
    void setScore(const string &sid, const string &sub, double value) {
        staff();
        studentAccess(sid);
        need(byId(d.subjects, sub), "科目不存在");
        auto p = find_if(d.scores.begin(), d.scores.end(),
                         [&](const Score &x) { return x.sid == sid && x.sub == sub; });
        if (p == d.scores.end())
            d.scores.push_back({sid, sub, value});
        else
            p->value = value;
    }
    void addAccount(const string &name, const string &password, const string &role,
                    const string &ref) {
        admin();
        need(role == "student" || role == "teacher", "仅可创建学生或教师账号");
        need(!user(d, name), "账号已存在");
        d.users.push_back(makeUser(name, password, role, ref));
        validate(d);
    }
    void deleteAccount(const string &name) {
        admin();
        auto p = user(d, name);
        need(p, "账号不存在");
        need(p->role != "admin", "此接口仅可删除学生或教师账号");
        d.users.erase(remove_if(d.users.begin(), d.users.end(),
                                [&](const User &u) { return u.name == name; }),
                      d.users.end());
    }
    void disableAccount(const string &name, const string &flag) {
        admin();
        need(flag == "true" || flag == "false", "禁用标志为 true 或 false");
        auto p = user(d, name);
        need(p, "账号不存在");
        need(p->role != "admin", "此接口仅可禁用学生或教师账号");
        p->enabled = flag != "true";
    }
    string logsText(const string &query, bool history = false) const {
        const auto &u = current();
        ostringstream o;
        o << "时间\t账号\t身份\t操作\n";
        for (auto &x : d.logs)
            if ((u.role == "admin" || x.actor == u.name) &&
                (!history || (x.actor == u.name && x.action.find("login") != string::npos)) &&
                (query.empty() ||
                 (x.time + " " + x.actor + " " + x.role + " " + x.action).find(query) !=
                     string::npos))
                o << x.time << '\t' << x.actor << '\t' << x.role << '\t' << x.action << '\n';
        return o.str();
    }
    string dispatch(const vector<string> &a) {
        const string &c = a[0];
        if (c == "help") {
            sizeIs(a, 1);
            return help();
        }
        if (c == "login") {
            sizeIs(a, 3);
            auto p = user(d, a[1]);
            need(p && p->enabled && equalHash(p->hash, digest(a[2], p->salt)),
                 "登录失败：账号、密码或启用状态不正确");
            active = p->name;
            return "登录成功：" + p->name + "（" + p->role + "）\n";
        }
        current();
        if (c == "logout") {
            sizeIs(a, 1);
            active.clear();
            return "已退出登录\n";
        }
        if (c == "me") {
            sizeIs(a, 1);
            auto u = current();
            if (u.role == "student")
                return personal(u.ref);
            if (u.role == "teacher") {
                auto p = byId(d.teachers, u.ref);
                string s = "教师 " + p->id + " " + p->name + " 联系方式 " + p->phone + " 管辖班级";
                for (auto &x : p->classes)
                    s += " " + x;
                return s + "\n";
            }
            return "管理员账号：" + u.name + "\n";
        }
        if (c == "password") {
            sizeIs(a, 3);
            auto p = user(d, active);
            need(equalHash(p->hash, digest(a[1], p->salt)), "原密码不正确");
            passok(a[2]);
            p->salt = salt();
            p->hash = digest(a[2], p->salt);
            return "密码已修改\n";
        }
        if (c == "history") {
            sizeIs(a, 1);
            return logsText("", true);
        }
        if (c == "classes") {
            sizeIs(a, 1);
            ostringstream o;
            for (auto &x : d.classes) {
                bool ok = current().role == "admin";
                if (current().role == "teacher") {
                    auto t = byId(d.teachers, current().ref);
                    ok = find(t->classes.begin(), t->classes.end(), x.id) != t->classes.end();
                }
                if (current().role == "student")
                    ok = byId(d.students, current().ref)->cls == x.id;
                if (ok)
                    o << x.id << ' ' << x.name << ' ' << x.grade << '\n';
            }
            return o.str();
        }
        if (c == "subjects") {
            sizeIs(a, 1);
            ostringstream o;
            for (auto &x : d.subjects)
                o << x.id << ' ' << x.name << " 学分 " << f2(x.credit) << '\n';
            return o.str();
        }
        if (c == "users") {
            admin();
            sizeIs(a, 1);
            ostringstream o;
            for (auto &x : d.users)
                o << x.name << ' ' << x.role << ' ' << x.ref << ' ' << (x.enabled ? "启用" : "禁用")
                  << '\n';
            return o.str();
        }
        if (c == "teachers") {
            admin();
            sizeIs(a, 1);
            ostringstream o;
            for (auto &x : d.teachers) {
                o << x.id << ' ' << x.name << ' ' << x.phone << " 班级";
                for (auto &y : x.classes)
                    o << ' ' << y;
                o << '\n';
            }
            return o.str();
        }
        if (c == "user-add") {
            sizeIs(a, 5);
            addAccount(a[1], a[2], a[3], a[4]);
            return "账号已创建\n";
        }
        if (c == "user-delete") {
            sizeIs(a, 3);
            admin();
            confirm(a[2]);
            deleteAccount(a[1]);
            return "账号已删除，基础资料保留\n";
        }
        if (c == "user-disable") {
            sizeIs(a, 3);
            disableAccount(a[1], a[2]);
            return "账号启用状态已更新\n";
        }
        if (c == "reset") {
            admin();
            sizeIs(a, 3);
            auto p = user(d, a[1]);
            need(p, "账号不存在");
            passok(a[2]);
            p->salt = salt();
            p->hash = digest(a[2], p->salt);
            return "指定用户密码已重置\n";
        }
        if (c == "reset-all") {
            admin();
            sizeIs(a, 3);
            confirm(a[2]);
            passok(a[1]);
            for (auto &x : d.users) {
                x.salt = salt();
                x.hash = digest(a[1], x.salt);
            }
            return "所有用户（含管理员）密码已重置\n";
        }
        if (c == "user-batch") {
            admin();
            sizeIs(a, 3);
            confirm(a[2]);
            ifstream in(fs::u8path(a[1]));
            need(bool(in), "批量账号文件无法打开");
            string line;
            size_t n = 0, ln = 0;
            while (getline(in, line)) {
                ++ln;
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();
                if (line.empty() || line[0] == '#')
                    continue;
                auto w = words(line);
                need(!w.empty(), "批量账号文件含空白记录");
                try {
                    if (w[0] == "ADD") {
                        sizeIs(w, 5);
                        addAccount(w[1], w[2], w[3], w[4]);
                    } else if (w[0] == "DELETE") {
                        sizeIs(w, 2);
                        deleteAccount(w[1]);
                    } else if (w[0] == "DISABLE") {
                        sizeIs(w, 3);
                        disableAccount(w[1], w[2]);
                    } else
                        throw runtime_error("仅支持 ADD/DELETE/DISABLE");
                } catch (const exception &e) {
                    throw runtime_error("第 " + to_string(ln) + " 行：" + e.what());
                }
                ++n;
            }
            need(!in.bad() && n > 0, "导入为空或读取失败");
            return "批量账号处理成功，共 " + to_string(n) + " 行\n";
        }
        if (c == "scores") {
            need(a.size() == 1 || a.size() == 2, "格式：scores [学号]");
            string id = a.size() == 2 ? a[1] : current().ref;
            need(current().role == "student" || a.size() == 2, "教师和管理员需指定学号");
            return personal(id);
        }
        throw runtime_error("未知命令，请输入 help");
    }

  public:
    explicit SchoolApp(fs::path path) : root(std::move(path)) {
        fs::create_directories(root);
        if (fs::exists(root / "school.db"))
            d = parseData(readFile(root / "school.db"));
        else if (fs::exists(root / "school.db.previous")) {
            d = parseData(readFile(root / "school.db.previous"));
            fs::rename(root / "school.db.previous", root / "school.db");
        } else {
            d = demo();
            validate(d);
            save();
        }
    }
    void save() {
        validate(d);
        string body = serialize(d);
        fs::path target = root / "school.db", temp = root / "school.db.tmp",
                 old = root / "school.db.previous";
        writeFile(temp, body + "CHECKSUM " + sha256(body) + "\n");
        if (fs::exists(target)) {
            if (fs::exists(old))
                fs::remove(old);
            fs::rename(target, old);
        }
        try {
            fs::rename(temp, target);
        } catch (...) {
            if (fs::exists(old) && !fs::exists(target))
                fs::rename(old, target);
            throw;
        }
        if (fs::exists(old))
            fs::remove(old);
    }
    struct Result {
        bool ok;
        string text;
    };
    Result execute(const string &line) {
        vector<string> a;
        try {
            a = words(line);
        } catch (const exception &e) {
            a = {"invalid-input"};
            return failure(a, e.what());
        }
        if (a.empty() || a[0][0] == '#')
            return {true, ""};
        Data before = d;
        string oldActive = active;
        string actor = active.empty() ? "anonymous" : active, role = "guest";
        if (auto p = user(d, active))
            role = p->role;
        if (a[0] == "login" && a.size() >= 2) {
            actor = idok(a[1]) ? a[1] : "invalid-account";
            if (auto p = user(d, a[1]))
                role = p->role;
        }
        string operation = a[0];
        static const set<string> secret = {"login",       "password",    "reset",   "reset-all",
                                           "student-add", "teacher-add", "user-add"};
        if (!secret.count(a[0]))
            for (size_t i = 1; i < a.size(); ++i)
                operation += " " + a[i];
        else if (a[0] == "login" && a.size() > 1)
            operation += " " + actor;
        else
            operation += " [敏感参数已隐藏]";
        if (operation.size() > 3500)
            operation = operation.substr(0, 3500);
        for (char &x : operation)
            if (static_cast<unsigned char>(x) < 32)
                x = ' ';
        try {
            string out = dispatch(a);
            validate(d);
            d.logs.push_back({now(), actor, role, "OK " + operation});
            save();
            return {true, out};
        } catch (const exception &e) {
            d = std::move(before);
            active = oldActive;
            string error = e.what();
            for (char &x : error)
                if (static_cast<unsigned char>(x) < 32)
                    x = ' ';
            if (error.size() > 300)
                error = error.substr(0, 300);
            d.logs.push_back({now(), actor, role, "FAIL " + operation + " | " + error});
            try {
                save();
            } catch (const exception &disk) {
                return {false, "错误：" + error + "；审计保存失败：" + disk.what() + "\n"};
            }
            return {false, "错误：" + error + "\n"};
        }
    }
    Result failure(const vector<string> &a, const string &message) {
        d.logs.push_back({now(), active.empty() ? "anonymous" : active,
                          active.empty() ? "guest" : current().role,
                          "FAIL " + a[0] + " | " + message});
        try {
            save();
        } catch (...) {
        }
        return {false, "错误：" + message + "\n"};
    }
    const Data &data() const { return d; }
    static string help() {
        return R"HELP(
学生成绩管理系统 A（模拟学生手工设计；实际 AI 辅助实现）
输入英文命令；含空格的字段/路径用英文双引号包围。退出程序：quit
公共：login 账号 密码 | logout | me | password 原密码 新密码 | history | help
查询：classes | subjects | scores
管理员：users | teachers
  user-add 账号 密码 student|teacher 学号或工号 | user-delete 账号 CONFIRM
  user-disable 账号 true|false（true 表示禁用）
  user-batch "文本路径" CONFIRM（ADD/DELETE/DISABLE；错误整批回滚）
  reset 账号 新密码 | reset-all 新密码 CONFIRM
示例：admin/Admin@123，teacher1/Teach@123，stu1/Stu@123
本版本为第 8 周阶段版本：资料维护、成绩录入、排名统计、图表、日志、备份恢复在第 12、16 周实现。
)HELP";
    }
};

static int selfTest() {
    fs::path dir = fs::temp_directory_path() / ("school_a_test_" + salt());
    int checks = 0;
    try {
        SchoolApp app(dir);
        auto run = [&](const string &c, bool expected = true) {
            auto r = app.execute(c);
            ++checks;
            need(r.ok == expected, "命令结果不符：" + c + " => " + r.text);
            return r.text;
        };
        auto check = [&](bool ok, const string &name) {
            ++checks;
            need(ok, name);
        };
        check(sha256("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
              "SHA-256 标准向量");
        run("grade S001 MATH 90", false);
        run("login stu1 Wrong@123", false);
        run("login stu1 Stu@123");
        auto personal = run("scores");
        check(personal.find("240.00") != string::npos && personal.find("3.00") != string::npos &&
                  personal.find("总分排名 1") != string::npos,
              "总分 GPA 竞争排名");
        run("scores S002", false);
        run("grade S001 MATH 100", false);
        run("student-edit S001 张三 男 2025 C1 13800000001", false);
        run("users", false);
        run("rank C1 total desc", false);
        run("logs-clear CONFIRM", false);
        run("backup", false);
        run("chart C1 MATH", false);
        run("export personal all");
        run("export personal MATH");
        run("password wrongpass New@12345", false);
        run("password Stu@123 New@12345");
        run("logout");
        run("login stu1 Stu@123", false);
        run("login stu1 New@12345");
        check(run("history").find("login stu1") != string::npos, "个人登录历史");
        run("logout");
        run("login teacher1 Teach@123");
        run("students C1 张");
        run("students C2", false);
        run("grade S004 MATH 100", false);
        run("student-edit S001 张三 男 2025 C2 13800000001", false);
        run("student-add S010 小七 男 2025 C1 13800000010 u10 Pass@123", false);
        run("grade S001 MATH nan", false);
        run("grade S001 MATH inf", false);
        run("grade S001 MATH -1", false);
        run("grade S001 MATH 101", false);
        run("grade S001 MATH 9oops", false);
        run("grade S001 MATH 0");
        run("grade S001 MATH 100");
        run("grade S001 MATH 90");
        auto rank = run("rank C1 total desc");
        check(rank.find("1\tS001") != string::npos && rank.find("1\tS002") != string::npos &&
                  rank.find("待齐全/未录入\tS003") != string::npos,
              "并列和不完整成绩");
        run("rank C1 MATH asc");
        auto st = run("stats C1 MATH");
        check(st.find("平均=66.67") != string::npos && st.find("及格=2") != string::npos &&
                  st.find("未录=0") != string::npos,
              "班级统计");
        writeFile(dir / "bad.txt", "S001 MATH 12\nS004 CPP 99\n");
        run("import-scores \"" + (dir / "bad.txt").u8string() + "\"", false);
        check(run("scores S001").find("成绩 90.00") != string::npos, "越权导入整批回滚");
        writeFile(dir / "bad2.txt", "S001 MATH 12\nS002 CPP nan\n");
        run("import-scores \"" + (dir / "bad2.txt").u8string() + "\"", false);
        check(run("scores S001").find("成绩 90.00") != string::npos, "非法导入整批回滚");
        writeFile(dir / "duplicate.txt", "S001 MATH 12\nS001 MATH 13\n");
        run("import-scores \"" + (dir / "duplicate.txt").u8string() + "\"", false);
        writeFile(dir / "valid.txt", "S003 ENG 70\nS001 MATH 90\n");
        run("import-scores \"" + (dir / "valid.txt").u8string() + "\"");
        check(run("rank C1 total desc").find("3\tS003") != string::npos, "竞争排名 1,1,3");
        auto ascending = run("rank C1 CPP asc");
        check(ascending.find("3\tS003") < ascending.find("1\tS001"),
              "升序改变显示顺序但仍按高分排名");
        run("chart C1 MATH");
        run("export students C1");
        run("export scores C1");
        run("export ranking C1 total desc");
        run("export statistics C1");
        run("export logs");
        check(run("logs").find("login stu1") == string::npos, "教师只能看自己日志");
        run("reset stu1 Pass@123", false);
        run("logout");
        run("login admin Admin@123");
        run("class-add C3 测试班 2026");
        run("class-edit C3 三班 2026");
        run("student-add S009 测试生 女 2026 C3 13800000009 - -");
        run("teacher-add T009 测试师 13900000009 C3 - -");
        writeFile(dir / "accounts.txt", "ADD teststu Temp@123 student S009\nADD testteacher "
                                        "Temp@123 teacher T009\nDISABLE teststu true\n");
        run("user-batch \"" + (dir / "accounts.txt").u8string() + "\" CONFIRM");
        run("login teststu Temp@123", false);
        run("user-disable teststu false");
        writeFile(dir / "accounts_bad.txt", "DISABLE teststu true\nDELETE absent\n");
        run("user-batch \"" + (dir / "accounts_bad.txt").u8string() + "\" CONFIRM", false);
        check(user(app.data(), "teststu")->enabled, "账号批量回滚");
        run("reset teststu Reset@123");
        run("subject-add PHY 物理 2");
        run("subject-edit PHY 大学物理 1.5");
        run("grade S009 PHY 66");
        run("grade-delete S009 PHY NO", false);
        run("grade-delete S009 PHY CONFIRM");
        run("subject-delete PHY CONFIRM");
        run("student-edit S009 测试生 女 2025 C1 13800000009");
        run("teacher-edit T009 测试师 13900000009 C1,C3");
        run("class-delete C3 CONFIRM", false);
        auto backup = run("backup");
        auto start = backup.find("：");
        string file = backup.substr(start + string("：").size());
        if (!file.empty() && file.back() == '\n')
            file.pop_back();
        run("grade S001 MATH 11");
        run("restore \"" + file + "\" NO", false);
        run("restore \"" + file + "\" CONFIRM");
        run("me", false);
        run("login admin Admin@123");
        check(run("scores S001").find("成绩 90.00") != string::npos, "有效备份恢复");
        writeFile(dir / "broken.db", "bad data");
        run("restore \"" + (dir / "broken.db").u8string() + "\" CONFIRM", false);
        check(run("scores S001").find("成绩 90.00") != string::npos, "坏备份不覆盖内存");
        Data invalid = app.data();
        invalid.scores[0].value = 101;
        string invalidBody = serialize(invalid);
        writeFile(dir / "bad_business.db", invalidBody + "CHECKSUM " + sha256(invalidBody) + "\n");
        run("restore \"" + (dir / "bad_business.db").u8string() + "\" CONFIRM", false);
        check(run("scores S001").find("成绩 90.00") != string::npos,
              "有效校验和但业务非法的备份被拒绝");
        run("user-delete teststu CONFIRM");
        run("student-delete S009 CONFIRM");
        run("teacher-delete T009 CONFIRM");
        run("class-delete C3 CONFIRM");
        run("reset-all Finish@123 CONFIRM");
        run("logout");
        run("login admin Finish@123");
        run("logs-clear CONFIRM");
        check(app.data().logs.size() == 1 &&
                  app.data().logs[0].action.find("logs-clear") != string::npos,
              "清空日志仍留审计记录");
        auto raw = readFile(dir / "school.db");
        check(raw.find("Finish@123") == string::npos && raw.find("New@12345") == string::npos &&
                  raw.find("Admin@123") == string::npos,
              "持久化不含明文密码");
        SchoolApp reload(dir);
        check(reload.execute("login admin Finish@123").ok, "持久化重启登录");
        fs::path interrupted = dir / "interrupted";
        fs::create_directories(interrupted);
        writeFile(interrupted / "school.db.previous", raw);
        SchoolApp recovered(interrupted);
        check(recovered.execute("login admin Finish@123").ok &&
                  fs::exists(interrupted / "school.db"),
              "主文件缺失时从已校验previous恢复");
        fs::path corrupt = dir / "corrupt";
        fs::create_directories(corrupt);
        writeFile(corrupt / "school.db", "corrupted");
        bool rejected = false;
        try {
            SchoolApp broken(corrupt);
        } catch (...) {
            rejected = true;
        }
        check(rejected && readFile(corrupt / "school.db") == "corrupted",
              "启动拒绝损坏数据且保留现场");
        SchoolApp decimals(dir / "decimal_ranks");
        auto decimalRun = [&](const string &command) {
            auto result = decimals.execute(command);
            ++checks;
            need(result.ok, "小数排名回归命令失败：" + command + " => " + result.text);
            return result.text;
        };
        decimalRun("login teacher1 Teach@123");
        decimalRun("grade S001 MATH 0.1");
        decimalRun("grade S001 CPP 0.2");
        decimalRun("grade S001 ENG 0.3");
        decimalRun("grade S002 MATH 0.3");
        decimalRun("grade S002 CPP 0.2");
        decimalRun("grade S002 ENG 0.1");
        auto decimalTotal = decimalRun("rank C1 total desc");
        check(decimalTotal.find("1\tS001") != string::npos &&
                  decimalTotal.find("1\tS002") != string::npos,
              "0.1/0.2/0.3 小数置换的总分并列");
        auto decimalAverage = decimalRun("rank C1 average asc");
        check(decimalAverage.find("1\tS001") != string::npos &&
                  decimalAverage.find("1\tS002") != string::npos,
              "0.1/0.2/0.3 小数置换的均分并列");
        decimalRun("grade S001 MATH 50");
        decimalRun("grade S002 MATH 50.0000000008");
        decimalRun("grade S003 MATH 50.0000000016");
        auto anchored = decimalRun("rank C1 MATH desc");
        check(anchored.find("1\tS003") != string::npos &&
                  anchored.find("1\tS002") != string::npos &&
                  anchored.find("3\tS001") != string::npos,
              "并列容差锚定组最高值，不作相邻链式合并");
        cout << "SELF-TEST PASS: " << checks << " checks; isolated temporary data.\n";
        fs::remove_all(dir);
        return 0;
    } catch (const exception &e) {
        cerr << "SELF-TEST FAIL after " << checks << " checks: " << e.what()
             << "\nTemporary evidence: " << dir.u8string() << '\n';
        return 1;
    }
}

int main(int argc, char **argv) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    try {
        vector<string> arguments;
#ifdef _WIN32
        // Narrow argv can use the active ANSI code page. Decode the actual Unicode
        // command line first so Chinese --data/--script paths remain intact.
        (void)argc;
        (void)argv;
        int wideCount = 0;
        LPWSTR *wideArguments = CommandLineToArgvW(GetCommandLineW(), &wideCount);
        need(wideArguments != nullptr, "无法读取 Unicode 启动参数");
        try {
            for (int i = 0; i < wideCount; ++i) {
                int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wideArguments[i], -1,
                                                nullptr, 0, nullptr, nullptr);
                need(bytes > 0, "启动参数包含非法 Unicode 字符");
                string value(static_cast<size_t>(bytes), '\0');
                need(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wideArguments[i], -1,
                                         &value[0], bytes, nullptr, nullptr) == bytes,
                     "启动参数转 UTF-8 失败");
                value.pop_back();
                arguments.push_back(std::move(value));
            }
        } catch (...) {
            LocalFree(wideArguments);
            throw;
        }
        LocalFree(wideArguments);
#else
        for (int i = 0; i < argc; ++i)
            arguments.push_back(argv[i]);
#endif
        fs::path path = "data", script;
        bool tests = false;
        for (size_t i = 1; i < arguments.size(); ++i) {
            string a = arguments[i];
            if (a == "--self-test")
                tests = true;
            else if (a == "--data" && i + 1 < arguments.size())
                path = fs::u8path(arguments[++i]);
            else if (a == "--script" && i + 1 < arguments.size())
                script = fs::u8path(arguments[++i]);
            else if (a == "--help") {
                cout << "school_manual [--data PATH] [--script FILE] [--self-test]\n";
                return 0;
            } else
                throw runtime_error("未知启动参数或缺少值：" + a);
        }
        if (tests)
            return selfTest();
        SchoolApp app(path);
        cout << "学生成绩管理系统 A\n输入 help 查看命令，quit 退出。数据目录："
             << fs::absolute(path).u8string() << "\n";
        if (!script.empty()) {
            ifstream in(script);
            need(bool(in), "脚本无法打开");
            string line;
            int failures = 0;
            while (getline(in, line)) {
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();
                if (line == "quit")
                    break;
                auto r = app.execute(line);
                cout << r.text;
                if (!r.ok)
                    ++failures;
            }
            return failures ? 2 : 0;
        }
        string line;
        while (cout << "> " && getline(cin, line)) {
            if (line == "quit")
                break;
            cout << app.execute(line).text;
        }
        return 0;
    } catch (const exception &e) {
        cerr << "启动失败：" << e.what() << "\n已有损坏数据不会被自动覆盖，请从有效备份恢复。\n";
        return 1;
    }
}
