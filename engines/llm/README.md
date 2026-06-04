# Built-in LLM (no Ollama)

Etemenanki can run a local **GGUF** model via `llama-cpp-python` on port **11435**.

| File | Role |
|------|------|
| `manifest.json` | Model id, Hugging Face download, server port |
| `models/*.gguf` | Downloaded weights (not in git) |

**Setup:** In the wizard choose **Built-in model** → Finish (downloads ~1.7 GB).

**CLI:**

```bash
python tools/embedded_llm.py download
python tools/embedded_llm.py serve
python tools/embedded_llm.py status
```
