#!/bin/sh
set -eu

repo=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
work=$(mktemp -d "${TMPDIR:-/tmp}/gpac-ut-sources.XXXXXX")
trap 'rm -rf "$work"' EXIT HUP INT TERM
source_dir="$work/source"
build_dir="$work/build"
cc=${CC:-cc}
make=${MAKE:-make}

mkdir -p "$source_dir/src/component/unittests" "$source_dir/unittests" "$source_dir/include" \
    "$build_dir/src" "$build_dir/bin/gcc"
cp "$repo/unittests/build.sh" "$source_dir/unittests/build.sh"
cat > "$source_dir/include/tests.h" <<'EOF'
#define unittest(name) int test_##name(void)
EOF
cat > "$source_dir/src/component/unittests/ut_source.c" <<'EOF'
#include "tests.h"
unittest(component_source)
{
    return 0;
}
EOF
cat > "$source_dir/unittests/tests.c" <<'EOF'
#include <string.h>
static int count;
int register_test(const char *name, int (*test)(void))
{
    if (strcmp(name, "component_source")) return 1;
    count++;
    return test();
}
int run_tests(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    return count == 1 ? 0 : 1;
}
EOF
cat > "$build_dir/config.mak" <<EOF
SRC_PATH=$source_dir
UNIT_TESTS=yes
EOF
: > "$build_dir/static.mak"
printf 'Libs.private:\n' > "$build_dir/gpac.pc"
printf 'void gpac_source_test_stub(void) {}\n' > "$work/stub.c"
$cc -c "$work/stub.c" -o "$work/stub.o"
${AR:-ar} cr "$build_dir/bin/gcc/libgpac.a" "$work/stub.o"

check_build()
{
    bash "$source_dir/unittests/build.sh" > "$build_dir/bin/gcc/unittests.c"
    if ! $make -s -C "$build_dir/src" -f "$repo/src/Makefile" unit_tests \
        "CC=$cc" "CFLAGS=-I$source_dir/include" > "$work/build.log" 2>&1; then
        cat "$work/build.log"
        exit 1
    fi
    "$build_dir/bin/gcc/unittests"
}

check_build

# Branch switches can leave a second generated main and old test sources.
mkdir -p "$source_dir/unittests/build" "$source_dir/build/old/src/component/unittests"
cat > "$source_dir/unittests/build/runner.c" <<'EOF'
int main(void) { return 1; }
EOF
cat > "$source_dir/build/old/src/component/unittests/ut_stale.c" <<'EOF'
#include "tests.h"
unittest(stale_build)
{
    return 1;
}
EOF
cat > "$source_dir/src/component/unittests/helper.c" <<'EOF'
#include "tests.h"
unittest(unrelated_helper)
{
    return 1;
}
int main(void) { return 1; }
EOF

check_build
printf 'Unit-test source selection checks passed.\n'
