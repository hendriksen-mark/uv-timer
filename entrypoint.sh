#!/bin/sh

mkdir -p dist

set -eu

if [ -n "${CC:-}" ]; then
	cc="$CC"
elif [ -x "/opt/homebrew/opt/llvm/bin/clang" ]; then
	cc="/opt/homebrew/opt/llvm/bin/clang"
else
	cc="clang"
fi

if ! "$cc" --print-targets 2>/dev/null | grep -qi 'wasm32'; then
	echo "Compiler '$cc' does not support wasm32 targets." >&2
	echo "Install a wasm-capable clang (for example: brew install llvm lld wasi-runtimes)" >&2
	exit 1
fi

if [ -d "/opt/homebrew/opt/lld/bin" ]; then
	PATH="/opt/homebrew/opt/lld/bin:$PATH"
fi

if [ -d "wasi-libc/sysroot" ]; then
	sysroot="wasi-libc/sysroot"
elif [ -d "wasi-libc/install" ]; then
	sysroot="wasi-libc/install"
else
	echo "Could not find WASI sysroot (expected wasi-libc/sysroot or wasi-libc/install)" >&2
	exit 1
fi

include_dir="$sysroot/include/wasm32-wasi"
if [ ! -d "$include_dir" ]; then
	echo "Missing WASI include directory: $include_dir" >&2
	exit 1
fi

resource_dir=""
if [ -n "${WASI_RUNTIME_RESOURCE_DIR:-}" ] && [ -f "$WASI_RUNTIME_RESOURCE_DIR/lib/wasm32-unknown-wasi/libclang_rt.builtins.a" ]; then
	resource_dir="$WASI_RUNTIME_RESOURCE_DIR"
elif [ -f "/opt/homebrew/opt/wasi-runtimes/share/wasi-runtimes/lib/wasm32-unknown-wasi/libclang_rt.builtins.a" ]; then
	resource_dir="/opt/homebrew/opt/wasi-runtimes/share/wasi-runtimes"
else
	for d in /opt/homebrew/Cellar/wasi-runtimes/*/share/wasi-runtimes; do
		if [ -f "$d/lib/wasm32-unknown-wasi/libclang_rt.builtins.a" ]; then
			resource_dir="$d"
			break
		fi
	done
fi

found=0
for source in chips/*.chip.c; do
	if [ ! -f "$source" ]; then
		continue
	fi

	found=1
	name="$(basename "$source" .chip.c)"
	json="chips/$name.chip.json"

	if [ -n "$resource_dir" ]; then
		"$cc" --target=wasm32-unknown-wasi --sysroot "$sysroot" -resource-dir "$resource_dir" -isystem "$include_dir" -nostartfiles -Wl,--export-table -Wl,--no-entry -Werror -Wno-error=deprecated -Wno-deprecated -o "dist/$name.chip.wasm" "$source"
	else
		"$cc" --target=wasm32-unknown-wasi --sysroot "$sysroot" -isystem "$include_dir" -nostartfiles -Wl,--export-table -Wl,--no-entry -Werror -Wno-error=deprecated -Wno-deprecated -o "dist/$name.chip.wasm" "$source"
	fi

	if [ ! -f "$json" ]; then
		echo "Missing descriptor for $source: $json" >&2
		exit 1
	fi

	cp "$json" "dist/$name.chip.json"
done

if [ "$found" -eq 0 ]; then
	echo "No chips found in chips/*.chip.c" >&2
	exit 1
fi
