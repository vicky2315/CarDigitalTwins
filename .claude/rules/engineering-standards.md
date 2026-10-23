---
paths:
  - "Source/**/*.h"
  - "Source/**/*.cpp"
  - "Source/**/*.cs"
  - "Config/**/*.ini"
  - "Tools/**/*.py"
---
# Engineering Standards

All C++ and Python here (human or Claude); goal = no reviewer can tell generated from hand-written. In doubt → match a neighbouring
class in the same folder.

## C++ house style (read from the code, 2026-10-23)
- Tabs, Allman braces, UE naming prefixes. No copyright header.
- **File header comment** at the top of every `.h`: what the class is for, who feeds it, SPEC section if any.
- **Includes:** own header first, blank line, then the rest sorted alphabetically (engine and project mixed).
- **One type per file**, except small enums/structs that only serve that class.
- **Comment per member:** every `UPROPERTY`, private field and non-obvious method gets a one-line purpose; defaults explained
  (`0 = hidden, 1 = shown`).
- **Order:** `.cpp` functions follow the `.h` declaration order.
- **Descriptive names:** say what they hold (`CalloutOpacities`, `TelemetryViewModel`); never close to engine names (`Tags`, `Mesh`,
  `Color`). Bound widgets end in their type (`RingImage`, `CalloutBoxBorder`).
- **Final newline** in every file.
- **Tunables** in `UTwinDashboardSettings` / `UTelemetrySettings` or a named `constexpr` in a file-local namespace
  (`CalloutLayerTiming::FadeSeconds`), never a bare number repeated across files. Shared colours/formatting: `TwinHmiColors`,
  `TwinHmiFormat`.
- **Loud misses:** missing config, anchor, asset or ViewModel → `UE_LOG` warning once, naming the thing and the consequence.
- **Unity builds:** file-scope helpers go in a uniquely named namespace; no generic `static` names that can clash.

## MVVM + UI (SPEC §5–§6)
- Dashboard widgets derive from `UTwinViewModelWidget`, subscribe in `NativeConstruct`; subscriptions end in `NativeDestruct`.
- ViewModels hold plain numbers and enums; widgets format. Setters game-thread only.
- Widgets never hold actor pointers; scene questions go through `ATwinOpsPlayerController`.
- `NativePreConstruct` shows a designer preview; Play state is set from data, not left at designer values.

## Modifying existing code
- Read the full header + implementation first.
- Smallest safe change; no reformatting, reordering or renaming of untouched code.
- New code goes at its logical position, not the end of the file.
- Never remove functionality unless asked (Blueprint/reflection may use it).

## Python (`Tools/`)
- Run with `python -I`. Editor scripts are idempotent and check every save; tests in `tests/` next to the tool (pytest).
