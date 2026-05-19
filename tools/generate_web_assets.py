from pathlib import Path

try:
    Import("env")  # type: ignore[name-defined]
    ROOT = Path(env["PROJECT_DIR"])  # type: ignore[name-defined]
except NameError:
    ROOT = Path(__file__).resolve().parents[1]

WEB_DIR = ROOT / "web"
OUTPUT = ROOT / "src" / "WiFiManagerWebAssets.h"


def raw_literal(name: str, delimiter: str, content: str) -> str:
    marker = f"){delimiter}\""
    if marker in content:
        raise ValueError(f"{name} contains raw string delimiter {marker}")
    return f'static const char {name}[] = R"{delimiter}({content}){delimiter}";'


def main() -> None:
    setup = (WEB_DIR / "setup.html").read_text(encoding="utf-8")
    saved = (WEB_DIR / "saved.html").read_text(encoding="utf-8")
    content = "\n".join(
        [
            "#ifndef WIFI_MANAGER_WEB_ASSETS_H",
            "#define WIFI_MANAGER_WEB_ASSETS_H",
            "",
            "// Generated from web/setup.html and web/saved.html.",
            "// Run tools/generate_web_assets.py after editing those files.",
            "namespace WiFiManagerWebAssets {",
            raw_literal("kSetupPage", "WMSETUP", setup),
            "",
            raw_literal("kSavedPage", "WMSAVED", saved),
            "} // namespace WiFiManagerWebAssets",
            "",
            "#endif // WIFI_MANAGER_WEB_ASSETS_H",
            "",
        ]
    )

    if OUTPUT.exists() and OUTPUT.read_text(encoding="utf-8") == content:
        return
    OUTPUT.write_text(content, encoding="utf-8", newline="\n")


main()
