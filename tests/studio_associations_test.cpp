#include "studio_associations.h"
#include "check.h"

int main() {
    const QString id = QStringLiteral("io.github.catpieleaf.Stud");
    const QString input = QStringLiteral(
        "# keep comments\n[Default Applications]\n"
        "x-scheme-handler/roblox-studio=io.github.catpieleaf.Stud.desktop;other.desktop;\n"
        "x-scheme-handler/roblox=io.github.catpieleaf.Stud.desktop;\n"
        "application/x-roblox-rbxl=io.github.catpieleaf.Stud.Studio.desktop;\n"
        "[Added Associations]\n"
        "x-scheme-handler/roblox-studio-auth=other.desktop;stud.desktop;\n"
        "application/x-roblox-rbxlx=other.desktop;\n"
        "[Other]\nx-scheme-handler/roblox-studio=stud.desktop;\n");
    const auto clean = stud::ui::without_studio_associations(input.split('\n'), id);
    const QString output = clean.join('\n');
    expect(output.contains("x-scheme-handler/roblox-studio=other.desktop;"), "preserves another Studio handler");
    expect(output.contains("x-scheme-handler/roblox=io.github.catpieleaf.Stud.desktop;"), "preserves Player associations");
    expect(!output.contains("application/x-roblox-rbxl="), "removes an empty Stud-only file association");
    expect(output.contains("x-scheme-handler/roblox-studio-auth=other.desktop;"), "removes only Stud from added associations");
    expect(output.contains("[Other]\nx-scheme-handler/roblox-studio=stud.desktop;"), "preserves unrelated sections");
    expect(stud::ui::without_studio_associations(clean, id) == clean, "cleanup is idempotent");
    return finish("Studio association migration");
}
