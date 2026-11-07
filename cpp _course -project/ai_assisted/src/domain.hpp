#pragma once
#include <algorithm>
#include <array>
#include <cctype>
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
#include <map>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace sg {
namespace fs = std::filesystem;
using std::size_t;
using std::string;
inline void require(bool ok, const string &msg) {
    if (!ok)
        throw std::runtime_error(msg);
}
inline string now() {
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream s;
    s << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return s.str();
}
inline string stamp() {
    string s = now();
    for (auto &c : s)
        if (c == ' ' || c == ':' || c == '-')
            c = '_';
    return s;
}
inline bool identifier(const string &s) {
    return !s.empty() && s.size() <= 40 && std::all_of(s.begin(), s.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '_' || c == '-';
    });
}
inline bool clean(const string &s) {
    return s.size() <= 200 && !s.empty() && s.find_first_of("\r\n\t") == string::npos;
}
inline std::vector<string> split(const string &s, char delimiter = ',') {
    std::vector<string> v;
    std::istringstream in(s);
    string t;
    while (std::getline(in, t, delimiter))
        v.push_back(t);
    return v;
}
inline double scoreNumber(const string &s) {
    size_t used = 0;
    double n = 0;
    try {
        n = std::stod(s, &used);
    } catch (...) {
        throw std::runtime_error("成绩必须为 0—100 的有限数字");
    }
    require(used == s.size() && std::isfinite(n) && n >= 0 && n <= 100,
            "成绩必须为 0—100 的有限数字");
    return n;
}
inline string fixed(double d) {
    std::ostringstream s;
    s << std::fixed << std::setprecision(2) << d;
    return s.str();
}
enum class Role { Student, Teacher, Admin };
inline string roleName(Role r) {
    return r == Role::Student ? "student" : r == Role::Teacher ? "teacher" : "admin";
}
inline Role parseRole(const string &s) {
    if (s == "student")
        return Role::Student;
    if (s == "teacher")
        return Role::Teacher;
    if (s == "admin")
        return Role::Admin;
    throw std::runtime_error("未知角色");
}
struct Account {
    string login, salt, hash, ref;
    Role role = Role::Student;
    bool enabled = true;
};
struct Student {
    string id, account, name, gender, year, classId, contact;
};
struct Teacher {
    string id, account, name;
    std::set<string> classes;
};
struct Subject {
    string id, name;
};
struct SchoolClass {
    string id, name;
};
struct State {
    std::map<string, Account> accounts;
    std::map<string, Student> students;
    std::map<string, Teacher> teachers;
    std::map<string, Subject> subjects;
    std::map<string, SchoolClass> classes;
    std::map<std::pair<string, string>, double> grades;
};
struct Session {
    string login;
};
struct Summary {
    double total = 0, mean = 0, gpa = 0;
    size_t recorded = 0, expected = 0;
    bool complete() const {
        return expected > 0 && recorded == expected;
    }
};
struct Stat {
    size_t enrolled = 0, n = 0, passed = 0;
    double sum = 0, lo = 100, hi = 0;
    std::array<int, 5> bands{};
};
struct RankRow {
    string student;
    double value = 0;
    int rank = 0;
};
inline double gpa(double n) {
    return n < 60 ? 0 : std::min(4.0, (n - 50) / 10);
}
class RolePolicy {
  public:
    virtual ~RolePolicy() = default;
    virtual bool administer() const = 0;
    virtual bool teach(const State &, const Account &, const string &) const = 0;
    virtual bool seeStudent(const State &, const Account &, const Student &) const = 0;
};
class AdminPolicy final : public RolePolicy {
  public:
    bool administer() const override {
        return true;
    }
    bool teach(const State &, const Account &, const string &) const override {
        return true;
    }
    bool seeStudent(const State &, const Account &, const Student &) const override {
        return true;
    }
};
class TeacherPolicy final : public RolePolicy {
  public:
    bool administer() const override {
        return false;
    }
    bool teach(const State &s, const Account &a, const string &c) const override {
        auto i = s.teachers.find(a.ref);
        return i != s.teachers.end() && i->second.classes.count(c) > 0;
    }
    bool seeStudent(const State &s, const Account &a, const Student &t) const override {
        return teach(s, a, t.classId);
    }
};
class StudentPolicy final : public RolePolicy {
  public:
    bool administer() const override {
        return false;
    }
    bool teach(const State &, const Account &, const string &) const override {
        return false;
    }
    bool seeStudent(const State &, const Account &a, const Student &t) const override {
        return a.ref == t.id;
    }
};
inline const RolePolicy &policy(Role r) {
    static AdminPolicy a;
    static TeacherPolicy t;
    static StudentPolicy s;
    return r == Role::Admin     ? static_cast<const RolePolicy &>(a)
           : r == Role::Teacher ? static_cast<const RolePolicy &>(t)
                                : static_cast<const RolePolicy &>(s);
}
} // namespace sg
