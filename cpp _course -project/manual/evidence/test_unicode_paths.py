"""External startup test: real Windows argv with Chinese names and spaces."""
from pathlib import Path
import json
import subprocess
import tempfile

app = Path(__file__).resolve().parents[1] / "bin" / "manual_grade.exe"
with tempfile.TemporaryDirectory(prefix="课程 中文路径_") as temporary:
    root = Path(temporary)
    data = root / "数据 文件"
    script = root / "脚本 命令.txt"
    script.write_text(
        "login teacher1 Teach@123\n"
        "stats C1 ENG\n"
        "chart C1 MATH\n"
        "logout\nquit\n",
        encoding="utf-8",
    )
    result = subprocess.run(
        [str(app), "--data", str(data), "--script", str(script)],
        capture_output=True,
        encoding="utf-8",
        check=False,
    )
    assert result.returncode == 0, (result.returncode, result.stdout, result.stderr)
    assert (data / "school.db").is_file()
    assert "数据 文件" in result.stdout
    assert "平均=85.00" in result.stdout
    assert list((data / "reports" / "charts").glob("*.svg"))
    assert list((data / "reports" / "charts").glob("*.txt"))
    print(json.dumps({
        "result": "PASS",
        "checks": 6,
        "scenario": "Windows Unicode argv: Chinese names and spaces for both --data and --script",
        "return_code": result.returncode,
        "output": result.stdout,
    }, ensure_ascii=False, indent=2))
