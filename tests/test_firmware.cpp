// Unit test for Utils::formatFirmwareBCD (regression test for the
// "%02d" bug where the firmware string rendered as "1.%02d").
#include "core/utils.h"

#include <QCoreApplication>
#include <QString>
#include <cstdio>

static int failures = 0;

static void expectEq(const QString& input, const QString& expected) {
    const QString actual = Utils::formatFirmwareBCD(input);
    if (actual != expected) {
        std::fprintf(stderr, "FAIL: formatFirmwareBCD(\"%s\") = \"%s\", expected \"%s\"\n",
                     input.toUtf8().constData(),
                     actual.toUtf8().constData(),
                     expected.toUtf8().constData());
        failures++;
    } else {
        std::printf("ok:   \"%s\" -> \"%s\"\n", input.toUtf8().constData(),
                    actual.toUtf8().constData());
    }
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    // bcdDevice "0100" (hi=0x01, lo=0x00) -> "1.00"
    expectEq("0100", "1.00");
    // "0111" -> "1.11" (zero-padded low byte)
    expectEq("0111", "1.11");
    // "0200" -> "2.00"
    expectEq("0200", "2.00");
    // non-BCD string is returned verbatim
    expectEq("abc", "abc");
    // empty string -> "-"
    expectEq("", "-");

    if (failures > 0) {
        std::fprintf(stderr, "%d test(s) failed\n", failures);
        return 1;
    }
    std::printf("All formatFirmwareBCD tests passed\n");
    return 0;
}
