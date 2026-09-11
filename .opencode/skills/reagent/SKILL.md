---
name: reagent
description: Drive the ReAgent reverse-engineering pipeline (re-agent CLI + Ghidra + Codex CLI). Use when the user asks to reverse, decompile, reconstruct, validate or do parity on C/C++ functions from a binary, or mentions re-agent, ghidra-bridge, addresses, reversed hooks, or the reports/re-agent output.
---

# ReAgent (auto-re-agent)

ReAgent reconstruye y valida funciones C/C++ a partir de binarios, usando Ghidra (análisis) + LLMs (Codex CLI). NO modifica el árbol de fuentes original: escribe en `reports/re-agent/` y en overlays de candidatos; la validación con `copy_project` corre en copia temporal.

## Entorno instalado (rutas reales)

- Binarios (venv): `~/.venvs/re-agent/bin/re-agent` y `~/.venvs/re-agent/bin/ghidra-bridge`
- Ghidra: `/Applications/Ghidra` (headless: `/Applications/Ghidra/support/analyzeHeadless`)
- LLM: provider `codex` (CLI autenticado con ChatGPT)

### Proyecto 1 — Source Engine macOS (64-bit arm64, Mach-O)
- Bit target: `/Users/jesus/Desktop/Source-Engine-macos-port/OpenS/build/OpenS.app/Contents/MacOS/OpenS` (arm64, con símbolos, incluye Jolt)
- Proyecto: `/Users/jesus/Desktop/Source-Engine-macos-port/OpenS`
  - `re-agent.yaml` (provider codex, `source_root: engine`, `copy_project: true`, `require_verified: false`) + `ghidra-bridge.yaml`
  - Evidencia: `ghidra-export/` (5.936 funciones), Ghidra project `ghidra-project/`
  - Diferencia clave: este binario se compila NATIVO en macOS → los build gates SÍ son viables (aún no activados)
- Smoke test hecho: `re-agent plan --address 0x100097740` → OK (0 gaps)

### Proyecto 2 — GTA San Andreas 1.0 US (32-bit Windows PE)
- Binary: `gta_sa.exe` dentro de `/Users/jesus/Library/Application Support/Steam/steamapps/common/grand theft auto - san andreas`
- Proyecto: mismo dir; fuente gta-reversed clonada ahí (`source/game_sa`, `docs/hooks.csv`)
- Evidencia: `ghidra-export/` (11.777 funciones), address map (6.146 conocidos, 137 stubs)
- LLM: provider `codex`. Validación SIN build gates (`require_verified: false`) porque el build genera un DLL de Windows (no nativo en macOS).

Cubre 64-bit (Mach-O arm64) y 32-bit (PE) sin otra modificación: Ghidra/PyGhidra manejan ambos.

## Flujo estándar

1. `~/.venvs/re-agent/bin/re-agent doctor` — diagnóstico siempre primero
2. Para una función/dir nuevo: plan gratis → `re-agent plan --address 0xADDR --max-depth 2 --max-functions N --output out.json`
3. Corrida real acotada: `re-agent reverse --manifest out.json --dry-run` luego `--max-functions 1` (o `--address`)
4. Reporte: `re-agent status --manifest out.json --format text`; artefactos en `reports/re-agent/`

## Comandos clave

| Comando | Para qué |
|---|---|
| `re-agent doctor` | Diagnóstico (siempre primero) |
| `re-agent plan` | Manifiesto acotado SIN llamadas LLM |
| `re-agent reverse --address A` | Reconstruir una función |
| `re-agent reverse --manifest g.json --max-functions N` | Lote acotado |
| `re-agent status --manifest g.json` | Progreso/cobertura |
| `re-agent parity --address A --skip-ghidra` | Parity de fuente existente |
| `ghidra-bridge decompile/search/unimplemented ...` | Consultas de evidencia directas |

## Reglas operativas

- Empezar SIEMPRE con `re-agent doctor` y UNA función pequeña elegida por el usuario.
- Usar `plan` + `--dry-run` antes de gastar LLM; acotar `--max-functions`.
- El usuario debe confirmar costo LLM y el objetivo antes de correr reverse.
- Nunca dejar que re-agent sobrescriba fuentes; con `copy_project: false` los candidatos van a `reports/re-agent/candidates/`.
- `trust_configured_commands: true` es afirmar que los comandos validan de verdad; dejarlo en `false` para no confiables.
- Resultados = evidencia conservadora, NO equivalencia binaria probada.