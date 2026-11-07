#pragma once
#include "repository.hpp"
namespace sg {
class SchoolService {
    Repository repo_;
    State state_;
    const Account &actor(const Session &session) {
        auto i = state_.accounts.find(session.login);
        if (i == state_.accounts.end() || !i->second.enabled) {
            repo_.audit(session.login, "unknown", "DENIED invalid or disabled session");
            throw std::runtime_error("请重新登录；账号不存在或已禁用");
        }
        return i->second;
    }
    void check(bool condition, const Account &a, const string &reason) {
        if (!condition) {
            repo_.audit(a.login, roleName(a.role), "DENIED " + reason);
            throw std::runtime_error("权限或参数错误：" + reason);
        }
    }
    const Account &admin(const Session &s) {
        auto &a = actor(s);
        check(policy(a.role).administer(), a, "仅管理员可操作");
        return a;
    }
    void classAccess(const Account &a, const string &c) {
        check(state_.classes.count(c) > 0, a, "班级不存在");
        check(policy(a.role).teach(state_, a, c), a, "不允许访问此班级");
    }
    const Student &studentAccess(const Account &a, const string &id, bool write = false) {
        auto it = state_.students.find(id);
        check(it != state_.students.end(), a, "学生不存在");
        check(write ? policy(a.role).teach(state_, a, it->second.classId)
                    : policy(a.role).seeStudent(state_, a, it->second),
              a, "不允许访问此学生");
        return it->second;
    }
    void audit(const Account &a, const string &what) {
        repo_.audit(a.login, roleName(a.role), what);
    }
    void commit(State &&next, const Account &a, const string &what) {
        string login = a.login, role = roleName(a.role);
        validate(next);
        repo_.save(next);
        state_ = std::move(next);
        try {
            repo_.audit(login, role, what);
        } catch (const std::exception &e) {
            throw std::runtime_error(string("数据已保存，但操作日志写入失败：") + e.what());
        }
    }
    Summary summarize(const string &id) const {
        Summary s;
        s.expected = state_.subjects.size();
        for (const auto &[key, value] : state_.grades)
            if (key.first == id) {
                ++s.recorded;
                s.total += value;
                s.gpa += sg::gpa(value);
            }
        if (s.recorded) {
            s.mean = s.total / s.recorded;
            s.gpa /= s.recorded;
        }
        return s;
    }
    std::vector<RankRow> rankings(const string &c, const string &metric, bool ascending) const {
        require(metric == "total" || metric == "mean" || state_.subjects.count(metric),
                "排名指标须为 total、mean 或科目代码");
        std::vector<RankRow> v;
        for (const auto &[id, student] : state_.students)
            if (student.classId == c) {
                if (metric == "total" || metric == "mean") {
                    auto t = summarize(id);
                    if (t.complete())
                        v.push_back({id, metric == "total" ? t.total : t.mean, 0});
                } else {
                    auto i = state_.grades.find({id, metric});
                    if (i != state_.grades.end())
                        v.push_back({id, i->second, 0});
                }
            }
        std::sort(v.begin(), v.end(), [&](const auto &a, const auto &b) {
            return a.value == b.value ? a.student < b.student : a.value > b.value;
        });
        double groupTop = v.empty() ? 0 : v.front().value;
        int groupRank = 1;
        for (size_t i = 0; i < v.size(); ++i) {
            // Compare against the group head, avoiding both binary-decimal rounding
            // artifacts and tolerance chaining across a sequence of nearby values.
            if (std::abs(v[i].value - groupTop) > 1e-9) {
                groupTop = v[i].value;
                groupRank = static_cast<int>(i + 1);
            }
            v[i].rank = groupRank;
        }
        if (ascending)
            std::stable_sort(v.begin(), v.end(),
                             [](const auto &a, const auto &b) { return a.value < b.value; });
        return v;
    }
    Stat statistics(const string &c, const string &sub) const {
        Stat t;
        for (const auto &[id, s] : state_.students)
            if (s.classId == c) {
                ++t.enrolled;
                auto i = state_.grades.find({id, sub});
                if (i != state_.grades.end()) {
                    double n = i->second;
                    ++t.n;
                    t.sum += n;
                    t.lo = std::min(t.lo, n);
                    t.hi = std::max(t.hi, n);
                    if (n >= 60)
                        ++t.passed;
                    ++t.bands[n < 60 ? 0 : n < 70 ? 1 : n < 80 ? 2 : n < 90 ? 3 : 4];
                }
            }
        return t;
    }
    string studentLine(const Student &s) const {
        std::ostringstream o;
        o << s.id << '\t' << s.account << '\t' << s.name << '\t' << s.gender << '\t' << s.year
          << '\t' << s.classId << '\t' << s.contact << '\n';
        return o.str();
    }
    string summaryText(const string &id) const {
        auto s = summarize(id);
        std::ostringstream o;
        o << "已录/应录：" << s.recorded << '/' << s.expected << "；状态："
          << (s.complete() ? "完整" : "缺成绩") << "\n已录总分：" << fixed(s.total)
          << "；已录均分：" << (s.recorded ? fixed(s.mean) : "N/A") << "；已录科目平均 GPA："
          << (s.recorded ? fixed(s.gpa) : "N/A") << '\n';
        if (!s.complete())
            o << "总分/均分班级排名不参与；缺成绩不计作 0 分。\n";
        return o.str();
    }
    static string xml(string s) {
        string r;
        for (char c : s) {
            if (c == '&')
                r += "&amp;";
            else if (c == '<')
                r += "&lt;";
            else if (c == '>')
                r += "&gt;";
            else if (c == '\"')
                r += "&quot;";
            else
                r += c;
        }
        return r;
    }

  public:
    explicit SchoolService(fs::path root) : repo_(std::move(root)) {
        if (repo_.exists())
            state_ = repo_.load();
        else {
            state_ = demoState();
            repo_.save(state_);
            repo_.audit("system", "system", "INITIALIZE demo data");
        }
    }
    Session login(const string &name, const string &password) {
        require(identifier(name), "账号只允许字母、数字、下划线、连字符");
        auto i = state_.accounts.find(name);
        bool ok =
            i != state_.accounts.end() && i->second.enabled && passwordIs(i->second, password);
        repo_.audit(name, i == state_.accounts.end() ? "unknown" : roleName(i->second.role),
                    ok ? "LOGIN success" : "LOGIN failed");
        require(ok, "账号、密码错误或账号已禁用");
        return {name};
    }
    void logout(const Session &s) {
        auto &a = actor(s);
        audit(a, "LOGOUT");
    }
    void failure(const Session &s, const string &command) {
        string role = "anonymous";
        auto i = state_.accounts.find(s.login);
        if (i != state_.accounts.end())
            role = roleName(i->second.role);
        repo_.audit(s.login.empty() ? "anonymous" : s.login, role, "COMMAND_FAILED " + command);
    }
    void changePassword(const Session &s, const string &oldp, const string &newp) {
        auto &a = actor(s);
        require(passwordIs(a, oldp), "旧密码错误");
        State n = state_;
        setPassword(n.accounts.at(a.login), newp);
        commit(std::move(n), a, "PASSWORD change own");
    }
    void resetPassword(const Session &s, const string &who, const string &newp) {
        auto &a = admin(s);
        require(state_.accounts.count(who), "账号不存在");
        State n = state_;
        setPassword(n.accounts.at(who), newp);
        commit(std::move(n), a, "PASSWORD reset " + who);
    }
    string me(const Session &s) {
        auto &a = actor(s);
        std::ostringstream o;
        o << "账号：" << a.login << "；角色：" << roleName(a.role) << '\n';
        if (a.role == Role::Student)
            o << "学号\t账号\t姓名\t性别\t年级\t班级\t联系方式\n"
              << studentLine(state_.students.at(a.ref));
        else if (a.role == Role::Teacher) {
            const auto &t = state_.teachers.at(a.ref);
            o << "教师：" << t.id << ' ' << t.name << "；负责班级：";
            for (auto &c : t.classes)
                o << c << ' ';
            o << '\n';
        }
        audit(a, "READ own profile");
        return o.str();
    }
    string catalog(const Session &s) {
        auto &a = actor(s);
        std::ostringstream o;
        o << "科目（统一选修、每科 1 学分）：\n";
        for (const auto &[id, v] : state_.subjects)
            o << id << '\t' << v.name << '\n';
        o << "可访问班级：\n";
        for (const auto &[id, v] : state_.classes)
            if (policy(a.role).teach(state_, a, id) ||
                (a.role == Role::Student && state_.students.at(a.ref).classId == id))
                o << id << '\t' << v.name << '\n';
        audit(a, "READ catalog");
        return o.str();
    }
    string students(const Session &s, const string &c, const string &q = "") {
        auto &a = actor(s);
        classAccess(a, c);
        std::ostringstream o;
        o << "学号\t账号\t姓名\t性别\t年级\t班级\t联系方式\n";
        for (const auto &[id, v] : state_.students)
            if (v.classId == c &&
                (id.find(q) != string::npos || v.name.find(q) != string::npos ||
                 v.account.find(q) != string::npos || v.contact.find(q) != string::npos))
                o << studentLine(v);
        audit(a, "READ students class=" + c + " query=" + q);
        return o.str();
    }
    void editStudent(const Session &s, const string &id, const string &name, const string &gender,
                     const string &year, const string &c, const string &contact) {
        auto &a = actor(s);
        auto &st = studentAccess(a, id, true);
        check(a.role == Role::Admin || st.classId == c, a, "教师不能更改班级");
        State n = state_;
        auto &v = n.students.at(id);
        v.name = name;
        v.gender = gender;
        v.year = year;
        v.classId = c;
        v.contact = contact;
        commit(std::move(n), a, "UPDATE student=" + id);
    }
    string grades(const Session &s, const string &id, const string &sub = "all") {
        auto &a = actor(s);
        auto &v = studentAccess(a, id);
        require(sub == "all" || state_.subjects.count(sub), "科目不存在");
        std::ostringstream o;
        o << "学生：" << id << ' ' << v.name << "；班级：" << v.classId << "\n科目\t名称\t成绩\n";
        for (const auto &[sid, c] : state_.subjects)
            if (sub == "all" || sub == sid) {
                auto i = state_.grades.find({id, sid});
                o << sid << '\t' << c.name << '\t'
                  << (i == state_.grades.end() ? "未录入" : fixed(i->second)) << '\n';
            }
        o << summaryText(id);
        audit(a, "READ grades student=" + id + " subject=" + sub);
        return o.str();
    }
    string ownGrades(const Session &s, const string &sub = "all") {
        auto &a = actor(s);
        check(a.role == Role::Student, a, "此命令仅限学生");
        return grades(s, a.ref, sub);
    }
    void setGrade(const Session &s, const string &id, const string &sub, double value) {
        auto &a = actor(s);
        studentAccess(a, id, true);
        require(state_.subjects.count(sub), "科目不存在");
        require(std::isfinite(value) && value >= 0 && value <= 100, "成绩范围为 0—100 且必须有限");
        State n = state_;
        bool edit = n.grades.count({id, sub});
        n.grades[{id, sub}] = value;
        commit(std::move(n), a,
               string(edit ? "UPDATE" : "CREATE") + " grade student=" + id + " subject=" + sub +
                   " score=" + fixed(value));
    }
    void deleteGrade(const Session &s, const string &id, const string &sub, bool confirmed) {
        auto &a = actor(s);
        studentAccess(a, id, true);
        require(confirmed, "删除成绩必须输入 CONFIRM");
        State n = state_;
        require(n.grades.erase({id, sub}) > 0, "成绩不存在");
        commit(std::move(n), a, "DELETE grade student=" + id + " subject=" + sub);
    }
    void importGrades(const Session &s, const fs::path &file) {
        auto &a = actor(s);
        check(a.role != Role::Student, a, "学生不可导入成绩");
        string data = readFile(file);
        require(data.size() < 5 * 1024 * 1024, "导入文件过大");
        if (data.rfind("\xEF\xBB\xBF", 0) == 0)
            data.erase(0, 3);
        std::istringstream f(data);
        string line;
        State n = state_;
        std::set<std::pair<string, string>> seen;
        int count = 0, lineNo = 0;
        while (std::getline(f, line)) {
            ++lineNo;
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (line.empty() || line[0] == '#')
                continue;
            std::istringstream in(line);
            string id, sub, value, extra;
            in >> id >> sub >> value;
            require(!in.fail() && !(in >> extra),
                    "导入第 " + std::to_string(lineNo) + " 行必须有 学号 科目 成绩 三列");
            studentAccess(a, id, true);
            require(state_.subjects.count(sub), "导入科目不存在");
            require(seen.emplace(id, sub).second, "导入包含重复 学号/科目，整批拒绝");
            n.grades[{id, sub}] = scoreNumber(value);
            ++count;
        }
        require(count > 0, "没有可导入记录");
        commit(std::move(n), a, "BATCH_IMPORT grades count=" + std::to_string(count));
    }
    string classGrades(const Session &s, const string &c) {
        auto &a = actor(s);
        classAccess(a, c);
        std::ostringstream o;
        o << "学号\t姓名";
        for (auto &[id, v] : state_.subjects)
            o << '\t' << id;
        o << "\t已录总分\t已录均分\t已录 GPA\t状态\n";
        for (auto &[id, v] : state_.students)
            if (v.classId == c) {
                o << id << '\t' << v.name;
                for (auto &[sub, x] : state_.subjects) {
                    auto it = state_.grades.find({id, sub});
                    o << '\t' << (it == state_.grades.end() ? "未录入" : fixed(it->second));
                }
                auto t = summarize(id);
                o << '\t' << fixed(t.total) << '\t' << (t.recorded ? fixed(t.mean) : "N/A") << '\t'
                  << (t.recorded ? fixed(t.gpa) : "N/A") << '\t'
                  << (t.complete() ? "完整" : "缺成绩") << '\n';
            }
        audit(a, "READ class grades=" + c);
        return o.str();
    }
    string rank(const Session &s, const string &c, const string &metric, bool ascending = false) {
        auto &a = actor(s);
        classAccess(a, c);
        auto rows = rankings(c, metric, ascending);
        std::ostringstream o;
        o << "班级：" << c << "；指标：" << metric << "；"
          << (ascending ? "升序显示（低分在前，保留降序学业名次）" : "降序显示（高分在前）")
          << "；同分竞争排名 1,1,3\n名次\t学号\t姓名\t数值\n";
        std::set<string> included;
        for (auto &r : rows) {
            included.insert(r.student);
            o << r.rank << '\t' << r.student << '\t' << state_.students.at(r.student).name << '\t'
              << fixed(r.value) << '\n';
        }
        for (auto &[id, v] : state_.students)
            if (v.classId == c && !included.count(id))
                o << "不参与\t" << id << '\t' << v.name << "\t缺成绩\n";
        audit(a, "READ ranking class=" + c + " metric=" + metric + (ascending ? " asc" : " desc"));
        return o.str();
    }
    string ownRank(const Session &s) {
        auto &a = actor(s);
        check(a.role == Role::Student, a, "此命令仅限学生");
        auto &st = state_.students.at(a.ref);
        std::vector<string> metrics = {"total", "mean"};
        for (auto &[id, sub] : state_.subjects)
            metrics.push_back(id);
        std::ostringstream o;
        o << summaryText(st.id) << "班内降序竞争排名：\n";
        for (auto &m : metrics) {
            auto rows = rankings(st.classId, m, false);
            auto i = std::find_if(rows.begin(), rows.end(),
                                  [&](const auto &r) { return r.student == st.id; });
            o << m << "："
              << (i == rows.end() ? "缺成绩，不参与"
                                  : std::to_string(i->rank) + " / " + std::to_string(rows.size()) +
                                        " 名参排学生")
              << '\n';
        }
        audit(a, "READ own ranking");
        return o.str();
    }
    string stats(const Session &s, const string &c) {
        auto &a = actor(s);
        classAccess(a, c);
        std::ostringstream o;
        o << "班级：" << c
          << "；及格线 "
             "60；缺成绩排除出均分、极值和及格率分母\n科目\t班级人数\t已录人数\t缺成绩\t均分\t最高"
             "\t最低\t及格\t不及格\t已录及格率\n";
        for (auto &[id, sub] : state_.subjects) {
            auto t = statistics(c, id);
            o << id << '\t' << t.enrolled << '\t' << t.n << '\t' << t.enrolled - t.n << '\t'
              << (t.n ? fixed(t.sum / t.n) : "N/A") << '\t' << (t.n ? fixed(t.hi) : "N/A") << '\t'
              << (t.n ? fixed(t.lo) : "N/A") << '\t' << t.passed << '\t' << t.n - t.passed << '\t'
              << (t.n ? fixed(100.0 * t.passed / t.n) + "%" : "N/A") << '\n';
        }
        audit(a, "READ statistics class=" + c);
        return o.str();
    }
    string accounts(const Session &s) {
        auto &a = admin(s);
        std::ostringstream o;
        o << "账号\t角色\t状态\t关联编号\n";
        for (auto &[id, v] : state_.accounts)
            o << id << '\t' << roleName(v.role) << '\t' << (v.enabled ? "启用" : "禁用") << '\t'
              << v.ref << '\n';
        o << "教师信息：\n";
        for (auto &[id, t] : state_.teachers) {
            o << id << '\t' << t.account << '\t' << t.name << '\t';
            for (auto &c : t.classes)
                o << c << ',';
            o << '\n';
        }
        audit(a, "READ accounts teachers");
        return o.str();
    }
    void addUsers(const Session &s, const string &data) {
        auto &a = admin(s);
        State n = state_;
        std::istringstream f(data);
        string line;
        int count = 0;
        while (std::getline(f, line)) {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (line.empty() || line[0] == '#')
                continue;
            std::istringstream in(line);
            string role, login, pass, id, name, classes, extra;
            in >> role >> std::quoted(login) >> std::quoted(pass) >> std::quoted(id) >>
                std::quoted(name) >> std::quoted(classes);
            require(!in.fail() && (role == "student" || role == "teacher"), "账号行格式错误");
            require(identifier(login) && identifier(id) && !n.accounts.count(login),
                    "账号重复或编号非法");
            Account ac;
            ac.login = login;
            ac.role = parseRole(role);
            ac.ref = id;
            setPassword(ac, pass);
            if (role == "student") {
                Student v;
                v.id = id;
                v.account = login;
                v.name = name;
                v.classId = classes;
                in >> std::quoted(v.gender) >> std::quoted(v.year) >> std::quoted(v.contact);
                require(!in.fail() && n.students.emplace(id, v).second, "学生编号重复或字段不足");
            } else {
                Teacher t;
                t.id = id;
                t.account = login;
                t.name = name;
                for (auto &c : split(classes)) {
                    require(n.classes.count(c), "教师班级不存在");
                    require(t.classes.insert(c).second, "重复班级");
                }
                require(n.teachers.emplace(id, t).second, "教师编号重复");
            }
            require(!(in >> extra), "账号行字段过多");
            n.accounts.emplace(login, ac);
            ++count;
        }
        require(count > 0, "没有可创建账号");
        commit(std::move(n), a, "BATCH_CREATE users count=" + std::to_string(count));
    }
    void importUsers(const Session &s, const fs::path &file) {
        admin(s);
        string d = readFile(file);
        if (d.rfind("\xEF\xBB\xBF", 0) == 0)
            d.erase(0, 3);
        addUsers(s, d);
    }
    void usersAction(const Session &s, const string &action, const std::vector<string> &users,
                     bool confirmed) {
        auto &a = admin(s);
        require(action == "enable" || action == "disable" || action == "delete", "未知账号动作");
        require(!users.empty(), "账号列表为空");
        require(action != "delete" || confirmed, "批量删除必须输入 CONFIRM");
        State n = state_;
        std::set<string> seen;
        for (auto &u : users) {
            require(seen.insert(u).second, "账号列表包含重复项");
            auto i = n.accounts.find(u);
            require(i != n.accounts.end() && i->second.role != Role::Admin,
                    "只能处理已有学生/教师账号");
            if (action == "delete") {
                auto ref = i->second.ref;
                if (i->second.role == Role::Student) {
                    n.students.erase(ref);
                    for (auto g = n.grades.begin(); g != n.grades.end();)
                        if (g->first.first == ref)
                            g = n.grades.erase(g);
                        else
                            ++g;
                } else
                    n.teachers.erase(ref);
                n.accounts.erase(i);
            } else
                i->second.enabled = action == "enable";
        }
        commit(
            std::move(n), a, "BATCH_" + action + " users=" + [&]() {
                string r;
                for (auto &u : users)
                    r += u + ",";
                return r;
            }());
    }
    void editTeacher(const Session &s, const string &id, const string &name,
                     const string &classes) {
        auto &a = admin(s);
        State n = state_;
        require(n.teachers.count(id), "教师不存在");
        auto &t = n.teachers.at(id);
        t.name = name;
        t.classes.clear();
        for (auto &c : split(classes))
            require(t.classes.insert(c).second, "重复班级");
        commit(std::move(n), a, "UPDATE teacher=" + id);
    }
    void catalogChange(const Session &s, const string &type, const string &action, const string &id,
                       const string &name, bool confirmed = false) {
        auto &a = admin(s);
        require(type == "class" || type == "subject", "未知目录类型");
        require(action == "add" || action == "edit" || action == "delete", "未知目录动作");
        require(identifier(id), "编号非法");
        State n = state_;
        bool exists = type == "class" ? n.classes.count(id) > 0 : n.subjects.count(id) > 0;
        require(action == "add" ? !exists : exists, "记录不存在或已存在");
        if (action == "delete") {
            require(confirmed, "删除必须输入 CONFIRM");
            if (type == "class") {
                for (auto &[sid, v] : n.students)
                    require(v.classId != id, "班级仍有学生，先转班或删除账号");
                for (auto &[tid, v] : n.teachers)
                    require(!v.classes.count(id), "班级仍被教师负责，请先编辑教师");
                n.classes.erase(id);
            } else {
                n.subjects.erase(id);
                for (auto it = n.grades.begin(); it != n.grades.end();)
                    if (it->first.second == id)
                        it = n.grades.erase(it);
                    else
                        ++it;
            }
        } else if (type == "class")
            n.classes[id] = {id, name};
        else
            n.subjects[id] = {id, name};
        commit(std::move(n), a, action + " " + type + "=" + id);
    }
    string logs(const Session &s, const string &query = "", bool history = false) {
        auto &a = actor(s);
        check(a.role != Role::Student || history, a, "学生仅可查看个人登录记录");
        std::ostringstream o;
        o << "时间\t账号\t身份\t操作\n";
        for (auto &l : repo_.logs()) {
            bool scope = a.role == Role::Admin || l.user == a.login;
            if (history)
                scope = l.user == a.login && l.action.rfind("LOGIN ", 0) == 0;
            if (scope &&
                (l.time + " " + l.user + " " + l.role + " " + l.action).find(query) != string::npos)
                o << l.time << '\t' << l.user << '\t' << l.role << '\t' << l.action << '\n';
        }
        audit(a, history ? "READ own login history" : "READ logs query=" + query);
        return o.str();
    }
    void clearLogs(const Session &s, bool confirmed) {
        auto &a = admin(s);
        require(confirmed, "清空日志必须输入 CONFIRM");
        repo_.clearLogs();
        audit(a, "CLEAR logs (previous records removed)");
    }
    fs::path exportOwn(const Session &s, const string &subject = "all") {
        auto &a = actor(s);
        check(a.role == Role::Student, a, "学生导出入口");
        string id = a.ref, content = ownGrades(s, subject);
        auto p = repo_.artifact("personal", id + "_" + subject, "txt", content);
        audit(a, "EXPORT personal grades=" + subject);
        return p;
    }
    fs::path exportClass(const Session &s, const string &kind, const string &c,
                         const string &metric = "total", bool ascending = false) {
        auto &a = actor(s);
        classAccess(a, c);
        string content;
        if (kind == "students")
            content = students(s, c);
        else if (kind == "grades")
            content = classGrades(s, c);
        else if (kind == "rank")
            content = rank(s, c, metric, ascending);
        else if (kind == "stats")
            content = stats(s, c);
        else
            throw std::runtime_error("导出类型须为 students/grades/rank/stats");
        auto p = repo_.artifact(
            kind, c + (kind == "rank" ? "_" + metric + (ascending ? "_asc" : "_desc") : ""), "txt",
            content);
        audit(a, "EXPORT " + kind + " class=" + c);
        return p;
    }
    fs::path exportLogs(const Session &s, const string &q = "") {
        auto &a = actor(s);
        check(a.role != Role::Student, a, "学生不能导出操作日志");
        auto content = logs(s, q);
        auto p = repo_.artifact("logs", a.login, "txt", content);
        audit(a, "EXPORT logs query=" + q);
        return p;
    }
    fs::path backup(const Session &s) {
        auto &a = admin(s);
        auto p = repo_.backup(state_);
        audit(a, "BACKUP file=" + p.filename().u8string());
        return p;
    }
    void restore(const Session &s, const fs::path &p, bool confirmed) {
        auto &a = admin(s);
        require(confirmed, "恢复将覆盖全库，必须输入 CONFIRM");
        State candidate = decode(readFile(p));
        auto safety = repo_.backup(state_);
        commit(std::move(candidate), a,
               "RESTORE file=" + p.filename().u8string() +
                   " safety=" + safety.filename().u8string());
    }
    fs::path chart(const Session &s, const string &c, const string &sub) {
        auto &a = actor(s);
        classAccess(a, c);
        require(state_.subjects.count(sub), "科目不存在");
        Stat t = statistics(c, sub);
        std::ostringstream o;
        o << "<!doctype html><html lang='zh-CN'><meta "
             "charset='utf-8'><title>班级成绩统计</title><style>body{font:16px "
             "system-ui;background:#f4f7fb;color:#172b4d;max-width:1100px;margin:40px "
             "auto}section{background:white;padding:24px;border-radius:12px;margin:20px "
             "0}table{border-collapse:collapse}td,th{padding:10px;border:1px solid "
             "#ddd}svg{max-width:100%;height:auto}</style><h1>"
          << xml(c + " / " + sub + " " + state_.subjects.at(sub).name) << "</h1><p>生成时间："
          << now() << "；仅统计已录成绩；缺成绩不算 0 分。</p><section><h2>学情概览</h2><p>班级 "
          << t.enrolled << " 人 · 已录 " << t.n << " 人 · 缺成绩 " << t.enrolled - t.n
          << " 人</p><p>均分 " << (t.n ? fixed(t.sum / t.n) : "N/A") << " · 最高 "
          << (t.n ? fixed(t.hi) : "N/A") << " · 最低 " << (t.n ? fixed(t.lo) : "N/A")
          << " · 及格率 " << (t.n ? fixed(100.0 * t.passed / t.n) + "%" : "N/A") << " · 不及格 "
          << t.n - t.passed
          << " 人</p></section><section><h2>分数段饼图</h2><svg xmlns='http://www.w3.org/2000/svg' "
             "viewBox='0 0 760 300' role='img' aria-label='分数段饼图'>";
        const char *colors[] = {"#df6262", "#e69c42", "#d7bb39", "#4d9bba", "#5b6bc0"};
        const char *labels[] = {"[0,60)", "[60,70)", "[70,80)", "[80,90)", "[90,100]"};
        double angle = -1.5707963267948966;
        const double pi = 3.14159265358979323846;
        if (t.n == 0)
            o << "<circle cx='160' cy='150' r='110' fill='#ddd'/><text x='120' "
                 "y='155'>暂无成绩</text>";
        for (int i = 0; i < 5; ++i) {
            if (t.bands[i]) {
                double next = angle + 2 * pi * t.bands[i] / t.n;
                if (size_t(t.bands[i]) == t.n)
                    o << "<circle cx='160' cy='150' r='110' fill='" << colors[i] << "'/>";
                else
                    o << "<path d='M160 150 L" << 160 + 110 * std::cos(angle) << ' '
                      << 150 + 110 * std::sin(angle) << " A110 110 0 "
                      << (next - angle > pi ? 1 : 0) << " 1 " << 160 + 110 * std::cos(next) << ' '
                      << 150 + 110 * std::sin(next) << " Z' fill='" << colors[i] << "'/>";
                angle = next;
            }
            o << "<rect x='350' y='" << 30 + i * 48 << "' width='18' height='18' fill='"
              << colors[i] << "'/><text x='385' y='" << 45 + i * 48 << "'>" << labels[i] << " 分："
              << t.bands[i] << " 人（" << (t.n ? fixed(100.0 * t.bands[i] / t.n) : "0.00")
              << "%）</text>";
        }
        o << "</svg></section><section><h2>分数段柱状图</h2><svg "
             "xmlns='http://www.w3.org/2000/svg' viewBox='0 0 760 320' role='img' "
             "aria-label='分数段人数柱状图'><path d='M55 25 V260 H720' fill='none' "
             "stroke='#333'/><text x='10' y='20'>人数</text>";
        int maximum = std::max(1, *std::max_element(t.bands.begin(), t.bands.end()));
        for (int i = 0; i < 5; ++i) {
            double h = 200.0 * t.bands[i] / maximum;
            int x = 90 + i * 125;
            o << "<rect x='" << x << "' y='" << 260 - h << "' width='70' height='" << h
              << "' fill='" << colors[i] << "'/><text x='" << x + 28 << "' y='" << 248 - h << "'>"
              << t.bands[i] << "</text><text x='" << x << "' y='285'>" << labels[i] << "</text>";
        }
        o << "</svg><p>人数为整数；每柱上方标明人数。分数段左闭右开，最后一段 [90,100] 包含 "
             "100。</p></section></html>";
        auto p = repo_.artifact("charts", c + "_" + sub, "html", o.str());
        repo_.artifact("stats", c + "_chart_snapshot", "txt", stats(s, c));
        audit(a, "EXPORT chart class=" + c + " subject=" + sub);
        return p;
    }
};
} // namespace sg
