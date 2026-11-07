#pragma once
#include "service.hpp"
namespace sg {
inline int selfTest() {
    const fs::path root = fs::temp_directory_path() / ("sgm_b_test_" + randomSalt());
    int total = 0, passed = 0;
    auto test = [&](const string &name, const auto &fn) {
        ++total;
        try {
            fn();
            ++passed;
            std::cout << "PASS " << name << '\n';
        } catch (const std::exception &e) {
            std::cout << "FAIL " << name << ": " << e.what() << '\n';
        }
    };
    auto expect = [](bool condition) { require(condition, "assertion failed"); };
    auto rejects = [&](const auto &fn) {
        bool rejected = false;
        try {
            fn();
        } catch (const std::exception &) {
            rejected = true;
        }
        expect(rejected);
    };
    try {
        SchoolService svc(root);
        Repository repo(root);
        Session a = svc.login("admin", "Admin@123"), t = svc.login("teacher1", "Teach@123"),
                st = svc.login("stu1", "Stu@123");
        test("SHA256 empty vector", [&] {
            expect(sha256("") ==
                   "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
        });
        test("SHA256 abc vector", [&] {
            expect(sha256("abc") ==
                   "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
        });
        test("SHA256 multi block vector", [&] {
            expect(sha256(string(1000, 'a')) ==
                   "41edece42d63e8d9bf515a9ba6932e1c20cbc9f5a5d134645adb5db1b9737ea3");
        });
        test("wrong login rejected", [&] { rejects([&] { svc.login("stu1", "wrong"); }); });
        test("student own profile", [&] { expect(svc.me(st).find("张三") != string::npos); });
        test("student no peer grades", [&] { rejects([&] { svc.grades(st, "S002"); }); });
        test("student cannot edit own grade",
             [&] { rejects([&] { svc.setGrade(st, "S001", "CPP", 99); }); });
        test("student cannot edit own profile", [&] {
            rejects([&] { svc.editStudent(st, "S001", "改名", "男", "2025", "C1", "x"); });
        });
        test("student cannot list class", [&] { rejects([&] { svc.students(st, "C1"); }); });
        test("student cannot see class ranks",
             [&] { rejects([&] { svc.rank(st, "C1", "CPP"); }); });
        test("student cannot see class stats", [&] { rejects([&] { svc.stats(st, "C1"); }); });
        test("teacher scope read denied", [&] { rejects([&] { svc.grades(t, "S004"); }); });
        test("teacher scope write denied",
             [&] { rejects([&] { svc.setGrade(t, "S004", "CPP", 99); }); });
        test("teacher cannot administer", [&] { rejects([&] { svc.accounts(t); }); });
        test("teacher cannot create accounts", [&] {
            rejects([&] { svc.addUsers(t, "student test Pass@123 S009 测试 C1 男 2025 x"); });
        });
        test("teacher cannot transfer class", [&] {
            rejects([&] { svc.editStudent(t, "S001", "张三", "男", "2025", "C2", "x"); });
        });
        test("teacher can edit scoped profile", [&] {
            svc.editStudent(t, "S001", "张三", "男", "2025", "C1", "13800000000");
            expect(repo.load().students.at("S001").contact == "13800000000");
        });
        test("fuzzy search", [&] {
            auto s = svc.students(t, "C1", "张");
            expect(s.find("S001") != string::npos && s.find("S002") == string::npos);
        });
        test("demo total mean GPA", [&] {
            auto s = svc.ownGrades(st);
            expect(s.find("240.00") != string::npos && s.find("80.00") != string::npos &&
                   s.find("3.00") != string::npos);
        });
        test("missing scores excluded total rank", [&] {
            auto s = svc.rank(t, "C1", "total");
            expect(s.find("1\tS001") != string::npos && s.find("1\tS002") != string::npos &&
                   s.find("不参与\tS003") != string::npos);
        });
        test("competition ranks 1 1 3", [&] {
            auto s = svc.rank(t, "C1", "CPP");
            expect(s.find("1\tS001") != string::npos && s.find("1\tS002") != string::npos &&
                   s.find("3\tS003") != string::npos);
        });
        test("ascending preserves academic rank", [&] {
            auto s = svc.rank(t, "C1", "CPP", true);
            expect(s.find("3\tS003") < s.find("1\tS001"));
        });
        auto decimalRankTest = [&](const string &metric) {
            SchoolService decimals(root / ("decimal_" + metric));
            auto administrator = decimals.login("admin", "Admin@123");
            decimals.setGrade(administrator, "S001", "CPP", 0.1);
            decimals.setGrade(administrator, "S001", "ENG", 0.2);
            decimals.setGrade(administrator, "S001", "MATH", 0.3);
            decimals.setGrade(administrator, "S002", "CPP", 0.3);
            decimals.setGrade(administrator, "S002", "ENG", 0.2);
            decimals.setGrade(administrator, "S002", "MATH", 0.1);
            auto ranking = decimals.rank(administrator, "C1", metric);
            expect(ranking.find("1\tS001") != string::npos &&
                   ranking.find("1\tS002") != string::npos &&
                   ranking.find("2\tS002") == string::npos);
        };
        test("decimal sum reversed order tie", [&] { decimalRankTest("total"); });
        test("decimal mean reversed order tie", [&] { decimalRankTest("mean"); });
        test("individual rank privacy", [&] {
            auto s = svc.ownRank(st);
            expect(s.find("S002") == string::npos && s.find("李四") == string::npos &&
                   s.find("total：1") != string::npos);
        });
        test("subject statistics denominator", [&] {
            auto s = svc.stats(t, "C1");
            expect(s.find("MATH\t3\t3\t0\t66.67\t90.00\t50.00\t2\t1\t66.67%") != string::npos &&
                   s.find("ENG\t3\t2\t1\t85.00") != string::npos);
        });
        test("NaN numeric rejected",
             [&] { rejects([&] { svc.setGrade(t, "S001", "CPP", std::nan("")); }); });
        test("infinity rejected", [&] { rejects([&] { scoreNumber("inf"); }); });
        test("above upper bound rejected",
             [&] { rejects([&] { svc.setGrade(t, "S001", "CPP", 101); }); });
        test("below lower bound rejected",
             [&] { rejects([&] { svc.setGrade(t, "S001", "CPP", -1); }); });
        test("numeric trailing junk rejected", [&] { rejects([&] { scoreNumber("88x"); }); });
        auto importFile = root / "import.txt";
        test("atomic duplicate import", [&] {
            auto before = encode(repo.load());
            atomicWrite(importFile, "S001 CPP 99\nS001 CPP 77\n");
            rejects([&] { svc.importGrades(t, importFile); });
            expect(encode(repo.load()) == before);
        });
        test("atomic scope import", [&] {
            auto before = encode(repo.load());
            atomicWrite(importFile, "S001 CPP 99\nS004 CPP 77\n");
            rejects([&] { svc.importGrades(t, importFile); });
            expect(encode(repo.load()) == before);
        });
        test("atomic invalid import", [&] {
            auto before = encode(repo.load());
            atomicWrite(importFile, "S001 CPP 99\nS002 CPP nan\n");
            rejects([&] { svc.importGrades(t, importFile); });
            expect(encode(repo.load()) == before);
        });
        test("valid BOM CRLF import", [&] {
            atomicWrite(importFile, "\xEF\xBB\xBF# import\r\nS001 CPP 88\r\nS002 CPP 78\r\n");
            svc.importGrades(t, importFile);
            expect(repo.load().grades.at({"S001", "CPP"}) == 88);
        });
        test("grade 0 and 100 accepted", [&] {
            svc.setGrade(t, "S001", "CPP", 0);
            svc.setGrade(t, "S001", "CPP", 100);
            expect(repo.load().grades.at({"S001", "CPP"}) == 100);
        });
        test("grade delete requires confirmation",
             [&] { rejects([&] { svc.deleteGrade(t, "S001", "CPP", false); }); });
        test("grade delete and reenter", [&] {
            svc.deleteGrade(t, "S001", "CPP", true);
            expect(!repo.load().grades.count({"S001", "CPP"}));
            svc.setGrade(t, "S001", "CPP", 80);
        });
        test("wrong old password rejected",
             [&] { rejects([&] { svc.changePassword(st, "wrong", "New@123"); }); });
        test("password change and re-login", [&] {
            svc.changePassword(st, "Stu@123", "New@123");
            rejects([&] { svc.login("stu1", "Stu@123"); });
            svc.login("stu1", "New@123");
        });
        test("admin resets all roles", [&] {
            svc.resetPassword(a, "stu1", "Stu@123");
            svc.resetPassword(a, "teacher1", "Teach@123");
            svc.resetPassword(a, "admin", "Admin@123");
            svc.login("stu1", "Stu@123");
        });
        test("password stored salted not plaintext", [&] {
            auto data = readFile(root / "school.db");
            expect(data.find("Stu@123") == string::npos &&
                   repo.load().accounts.at("stu1").salt != repo.load().accounts.at("stu2").salt);
        });
        test("atomic user batch duplicates", [&] {
            auto before = encode(repo.load());
            rejects([&] {
                svc.addUsers(a, "student test Pass@123 S009 新人 C1 女 2025 x\nstudent stu1 "
                                "Pass@123 S010 新人 C1 男 2025 x");
            });
            expect(encode(repo.load()) == before);
        });
        test("atomic user batch missing class", [&] {
            auto before = encode(repo.load());
            rejects([&] { svc.addUsers(a, "student test Pass@123 S009 新人 C9 女 2025 x"); });
            expect(encode(repo.load()) == before);
        });
        test("create student teacher batch", [&] {
            svc.addUsers(a, "student test Pass@123 S009 新人 C1 女 2025 x\nteacher newt Pass@123 "
                            "T009 新老师 C1,C2");
            expect(repo.load().accounts.count("test") &&
                   repo.load().teachers.at("T009").classes.size() == 2);
        });
        test("disabled stale session rejected", [&] {
            svc.usersAction(a, "disable", {"stu1", "teacher1"}, false);
            rejects([&] { svc.me(st); });
            rejects([&] { svc.login("teacher1", "Teach@123"); });
            svc.usersAction(a, "enable", {"stu1", "teacher1"}, false);
        });
        test("atomic disable unknown account", [&] {
            rejects([&] { svc.usersAction(a, "disable", {"stu1", "missing"}, false); });
            expect(repo.load().accounts.at("stu1").enabled);
        });
        test("cannot delete admin",
             [&] { rejects([&] { svc.usersAction(a, "delete", {"admin"}, true); }); });
        test("user delete requires confirmation",
             [&] { rejects([&] { svc.usersAction(a, "delete", {"test"}, false); }); });
        test("delete student cascades grades", [&] {
            svc.setGrade(a, "S009", "CPP", 70);
            svc.usersAction(a, "delete", {"test", "newt"}, true);
            auto n = repo.load();
            expect(!n.students.count("S009") && !n.accounts.count("test") &&
                   !n.grades.count({"S009", "CPP"}) && !n.teachers.count("T009"));
        });
        test("class create edit", [&] {
            svc.catalogChange(a, "class", "add", "C9", "测试班");
            svc.catalogChange(a, "class", "edit", "C9", "新测试班");
            expect(repo.load().classes.at("C9").name == "新测试班");
        });
        test("class delete rejects linked records",
             [&] { rejects([&] { svc.catalogChange(a, "class", "delete", "C1", "", true); }); });
        test("class delete unused", [&] {
            svc.catalogChange(a, "class", "delete", "C9", "", true);
            expect(!repo.load().classes.count("C9"));
        });
        test("subject CRUD and cascade", [&] {
            svc.catalogChange(a, "subject", "add", "PHY", "物理");
            svc.catalogChange(a, "subject", "edit", "PHY", "大学物理");
            svc.setGrade(t, "S001", "PHY", 88);
            svc.catalogChange(a, "subject", "delete", "PHY", "", true);
            expect(!repo.load().subjects.count("PHY") &&
                   !repo.load().grades.count({"S001", "PHY"}));
        });
        test("teacher edit class authorization refresh", [&] {
            svc.editTeacher(a, "T001", "陈老师", "C2");
            rejects([&] { svc.students(t, "C1"); });
            svc.students(t, "C2");
            svc.editTeacher(a, "T001", "陈老师", "C1");
        });
        test("student filtered export", [&] {
            auto p = svc.exportOwn(st, "CPP");
            auto content = readFile(p);
            expect(content.find("CPP\t") != string::npos &&
                   content.find("MATH\t") == string::npos && content.find("S002") == string::npos);
        });
        test("class TXT export types", [&] {
            for (auto kind : {"students", "grades", "rank", "stats"})
                expect(fs::exists(svc.exportClass(t, kind, "C1")));
        });
        test("teacher export scope denied",
             [&] { rejects([&] { svc.exportClass(t, "grades", "C2"); }); });
        test("chart and stats persistence", [&] {
            auto p = svc.chart(t, "C1", "MATH");
            auto html = readFile(p);
            expect(html.find("<svg") != string::npos && html.find("分数段柱状图") != string::npos &&
                   html.find("66.67%") != string::npos);
            expect(fs::exists(root / "reports" / "stats"));
        });
        test("teacher log privacy", [&] {
            auto l = svc.logs(t);
            expect(l.find("\tadmin\t") == string::npos && l.find("\tteacher2\t") == string::npos);
        });
        test("student login history privacy", [&] {
            auto l = svc.logs(st, "", true);
            expect(l.find("LOGIN success") != string::npos && l.find("\tadmin\t") == string::npos &&
                   l.find("UPDATE") == string::npos);
        });
        test("student cannot read operation log", [&] { rejects([&] { svc.logs(st); }); });
        test("teacher cannot clear logs", [&] { rejects([&] { svc.clearLogs(t, true); }); });
        test("logs never contain passwords", [&] {
            auto l = readFile(root / "audit.log");
            expect(l.find("Admin@123") == string::npos && l.find("Stu@123") == string::npos &&
                   l.find("New@123") == string::npos);
        });
        test("admin log query and export", [&] {
            auto l = svc.logs(a, "DENIED");
            expect(l.find("DENIED") != string::npos);
            expect(fs::exists(svc.exportLogs(a, "LOGIN")));
        });
        test("teacher backup denied", [&] { rejects([&] { svc.backup(t); }); });
        fs::path backup = svc.backup(a);
        test("backup has database and log", [&] {
            expect(fs::exists(backup) && fs::exists(fs::path(backup.u8string() + ".log")));
        });
        test("restore requires confirmation",
             [&] { rejects([&] { svc.restore(a, backup, false); }); });
        test("corrupt restore leaves state intact", [&] {
            auto before = encode(repo.load());
            auto bad = root / "bad.bak";
            atomicWrite(bad, readFile(backup) + "junk");
            rejects([&] { svc.restore(a, bad, true); });
            expect(encode(repo.load()) == before);
        });
        test("valid restore and audit retention", [&] {
            svc.setGrade(t, "S001", "CPP", 19);
            svc.restore(a, backup, true);
            expect(repo.load().grades.at({"S001", "CPP"}) == 80 &&
                   readFile(root / "audit.log").find("RESTORE") != string::npos);
        });
        test("restart persists state", [&] {
            SchoolService again(root);
            auto s = again.login("stu1", "Stu@123");
            expect(again.ownGrades(s).find("80.00") != string::npos);
        });
        test("empty subject statistics safe", [&] {
            svc.catalogChange(a, "subject", "add", "EMPTY", "空科目");
            expect(svc.stats(t, "C1").find("EMPTY\t3\t0\t3\tN/A") != string::npos);
            svc.chart(t, "C1", "EMPTY");
            svc.catalogChange(a, "subject", "delete", "EMPTY", "", true);
        });
        test("audit write failure states committed outcome", [&] {
            auto log = root / "audit.log", saved = root / "audit_saved.log";
            fs::rename(log, saved);
            fs::create_directory(log);
            string message;
            try {
                svc.setGrade(t, "S001", "CPP", 81);
            } catch (const std::exception &e) {
                message = e.what();
            }
            double persisted = repo.load().grades.at({"S001", "CPP"});
            fs::remove(log);
            fs::rename(saved, log);
            expect(persisted == 81 &&
                   message.find("数据已保存，但操作日志写入失败") != string::npos);
            svc.setGrade(t, "S001", "CPP", 80);
        });
        test("log clear confirmed remains auditable", [&] {
            rejects([&] { svc.clearLogs(a, false); });
            svc.clearLogs(a, true);
            auto logs = repo.logs();
            expect(logs.size() == 1 && logs[0].action.find("CLEAR logs") != string::npos);
        });
    } catch (const std::exception &e) {
        ++total;
        std::cout << "FAIL setup: " << e.what() << '\n';
    }
    std::cout << "RESULT " << passed << '/' << total << " passed\n";
    std::error_code pathError;
    // macOS temporary paths may have a trailing slash or a /private symlink alias.
    const bool inTemporaryDirectory =
        fs::equivalent(root.parent_path(), fs::temp_directory_path(), pathError);
    if (!pathError && inTemporaryDirectory &&
        root.filename().u8string().rfind("sgm_b_test_", 0) == 0) {
        std::error_code ec;
        fs::remove_all(root, ec);
        if (ec)
            std::cout << "Temporary directory retained: " << root.u8string() << '\n';
    }
    return total == passed ? 0 : 1;
}
} // namespace sg
