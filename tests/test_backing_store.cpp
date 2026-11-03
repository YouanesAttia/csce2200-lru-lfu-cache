#include "../src/storage/backing_store.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

using namespace storage;

static int checks = 0;
static int failures = 0;

#define CHECK(cond)                                                      \
    do {                                                                 \
        checks++;                                                        \
        if (!(cond)) {                                                   \
            failures++;                                                  \
            printf("  FAIL line %d: %s\n", __LINE__, #cond);             \
        }                                                                \
    } while (0)

static const char* TEST_FILE = "test_store.bin";
static const char* SAMPLE_FILE = "tests/sample_data.txt";

static long sizeOfFile(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fclose(f);
    return n;
}

static void writeRaw(const char* path, const char* bytes, size_t n) {
    FILE* f = fopen(path, "wb");
    fwrite(bytes, 1, n, f);
    fclose(f);
}

static void chopFile(const char* path, long newSize) {
    FILE* f = fopen(path, "rb");
    char* buf = new char[newSize];
    size_t got = fread(buf, 1, newSize, f);
    (void)got;
    fclose(f);
    writeRaw(path, buf, newSize);
    delete[] buf;
}

static void flipByte(const char* path, long offset) {
    FILE* f = fopen(path, "r+b");
    fseek(f, offset, SEEK_SET);
    int c = fgetc(f);
    fseek(f, offset, SEEK_SET);
    fputc(c ^ 0xFF, f);
    fclose(f);
}

static bool valueIs(BackingStore& s, const std::string& key,
                    const std::string& expected) {
    std::string v;
    return s.get(key, v) == Status::Ok && v == expected;
}

static void testRoundTrip() {
    printf("round trip\n");
    remove(TEST_FILE);
    BackingStore s;
    CHECK(s.open(TEST_FILE) == Status::Ok);
    CHECK(s.put("a", "alpha") == Status::Ok);
    CHECK(s.put("b", "beta") == Status::Ok);
    CHECK(s.put("c", "gamma") == Status::Ok);
    CHECK(s.count() == 3);
    CHECK(valueIs(s, "a", "alpha"));
    CHECK(valueIs(s, "b", "beta"));
    CHECK(valueIs(s, "c", "gamma"));
    s.close();
}

static void testMissingKeys() {
    printf("missing keys\n");
    remove(TEST_FILE);
    BackingStore s;
    s.open(TEST_FILE);
    std::string v = "untouched";
    CHECK(s.get("nope", v) == Status::NotFound);
    CHECK(v == "untouched");
    CHECK(!s.contains("nope"));
    CHECK(s.remove("nope") == Status::NotFound);
    s.put("x", "1");
    CHECK(s.get("y", v) == Status::NotFound);
    CHECK(s.get("X", v) == Status::NotFound);
    CHECK(s.get("x ", v) == Status::NotFound);
    s.close();
}

static void testUpdateAndRemove() {
    printf("update and remove\n");
    remove(TEST_FILE);
    BackingStore s;
    s.open(TEST_FILE);
    s.put("k", "old");
    s.put("k", "newer value");
    CHECK(s.count() == 1);
    CHECK(valueIs(s, "k", "newer value"));
    CHECK(s.contains("k"));
    CHECK(s.remove("k") == Status::Ok);
    CHECK(!s.contains("k"));
    std::string v;
    CHECK(s.get("k", v) == Status::NotFound);
    CHECK(s.count() == 0);
    CHECK(s.put("k", "back") == Status::Ok);
    CHECK(valueIs(s, "k", "back"));
    s.close();
}

static void testEmptyAndBinaryValues() {
    printf("empty and binary values\n");
    remove(TEST_FILE);
    BackingStore s;
    s.open(TEST_FILE);
    CHECK(s.put("empty", "") == Status::Ok);
    std::string v = "stale";
    CHECK(s.get("empty", v) == Status::Ok);
    CHECK(v.empty());
    const char bin[] = {0, 1, (char)0xFF, 0, (char)0x80, 10, 13, 0};
    std::string binary(bin, sizeof(bin));
    CHECK(s.put("bin", binary) == Status::Ok);
    CHECK(s.get("bin", v) == Status::Ok);
    CHECK(v == binary);
    CHECK(v.size() == sizeof(bin));
    s.close();
    CHECK(s.open(TEST_FILE) == Status::Ok);
    CHECK(s.get("bin", v) == Status::Ok);
    CHECK(v == binary);
    s.close();
}

static void testKeyVariety() {
    printf("key variety\n");
    remove(TEST_FILE);
    BackingStore s;
    s.open(TEST_FILE);
    std::string withSpace = "has space";
    std::string withNewline = "line1\nline2";
    std::string longKey(MAX_KEY_LEN, 'k');
    std::string unicode = "caf\xc3\xa9";
    CHECK(s.put(withSpace, "1") == Status::Ok);
    CHECK(s.put(withNewline, "2") == Status::Ok);
    CHECK(s.put(longKey, "3") == Status::Ok);
    CHECK(s.put(unicode, "4") == Status::Ok);
    CHECK(s.put("1", "5") == Status::Ok);
    CHECK(s.put("01", "6") == Status::Ok);
    s.close();
    CHECK(s.open(TEST_FILE) == Status::Ok);
    CHECK(s.count() == 6);
    CHECK(valueIs(s, withSpace, "1"));
    CHECK(valueIs(s, withNewline, "2"));
    CHECK(valueIs(s, longKey, "3"));
    CHECK(valueIs(s, unicode, "4"));
    CHECK(valueIs(s, "1", "5"));
    CHECK(valueIs(s, "01", "6"));
    s.close();
}

static void testPersistence() {
    printf("persistence across reopen\n");
    remove(TEST_FILE);
    {
        BackingStore s;
        s.open(TEST_FILE);
        s.put("one", "1");
        s.put("two", "2");
        s.put("three", "3");
        s.put("two", "TWO");
        s.remove("three");
        s.close();
    }
    BackingStore s;
    CHECK(s.open(TEST_FILE) == Status::Ok);
    CHECK(s.discardedBytes() == 0);
    CHECK(s.count() == 2);
    CHECK(valueIs(s, "one", "1"));
    CHECK(valueIs(s, "two", "TWO"));
    std::string v;
    CHECK(s.get("three", v) == Status::NotFound);
    CHECK((uint64_t)sizeOfFile(TEST_FILE) == s.fileSize());
    s.put("four", "4");
    s.close();
    CHECK(s.open(TEST_FILE) == Status::Ok);
    CHECK(s.count() == 3);
    CHECK(valueIs(s, "four", "4"));
    CHECK(valueIs(s, "one", "1"));
    s.close();
}

static void testSampleData() {
    printf("sample data file\n");
    remove(TEST_FILE);
    FILE* in = fopen(SAMPLE_FILE, "r");
    CHECK(in != nullptr);
    if (!in) return;
    BackingStore s;
    s.open(TEST_FILE);
    char line[512];
    int lines = 0;
    while (fgets(line, sizeof(line), in)) {
        size_t n = strlen(line);
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = 0;
        char* space = strchr(line, ' ');
        if (!space) continue;
        *space = 0;
        CHECK(s.put(line, space + 1) == Status::Ok);
        lines++;
    }
    fclose(in);
    CHECK(lines > 0);
    s.close();

    CHECK(s.open(TEST_FILE) == Status::Ok);
    CHECK((int)s.count() == lines);
    in = fopen(SAMPLE_FILE, "r");
    int verified = 0;
    while (fgets(line, sizeof(line), in)) {
        size_t n = strlen(line);
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = 0;
        char* space = strchr(line, ' ');
        if (!space) continue;
        *space = 0;
        CHECK(valueIs(s, line, space + 1));
        verified++;
    }
    fclose(in);
    CHECK(verified == lines);
    s.close();
}

static void testLargeVolume() {
    printf("large volume\n");
    remove(TEST_FILE);
    const int N = 20000;
    BackingStore s;
    s.open(TEST_FILE);
    for (int i = 0; i < N; i++)
        s.put("key-" + std::to_string(i), "value-" + std::to_string(i));
    CHECK((int)s.count() == N);
    s.close();
    CHECK(s.open(TEST_FILE) == Status::Ok);
    CHECK((int)s.count() == N);
    bool all = true;
    for (int i = 0; i < N; i += 37) {
        if (!valueIs(s, "key-" + std::to_string(i), "value-" + std::to_string(i)))
            all = false;
    }
    CHECK(all);
    s.close();
}

static void testLimitsAndArguments() {
    printf("limits and arguments\n");
    remove(TEST_FILE);
    BackingStore s;
    std::string v;
    CHECK(s.put("a", "a") == Status::ErrNotOpen);
    CHECK(s.get("a", v) == Status::ErrNotOpen);
    CHECK(s.remove("a") == Status::ErrNotOpen);
    CHECK(s.flush() == Status::ErrNotOpen);
    CHECK(!s.contains("a"));
    CHECK(s.open(nullptr) == Status::ErrInvalidArg);
    s.open(TEST_FILE);
    CHECK(s.put("", "value") == Status::ErrInvalidArg);
    CHECK(s.put(std::string(MAX_KEY_LEN + 1, 'k'), "v") == Status::ErrTooLarge);
    std::string big(MAX_VALUE_LEN + 1, 'z');
    CHECK(s.put("k", big) == Status::ErrTooLarge);
    big.resize(MAX_VALUE_LEN);
    CHECK(s.put("k", big) == Status::Ok);
    CHECK(s.get("k", v) == Status::Ok);
    CHECK(v.size() == MAX_VALUE_LEN);
    s.close();
}

static void testOpenFailures() {
    printf("open failures\n");
    BackingStore s;
    CHECK(s.open("no_such_directory_xyz/store.bin") == Status::ErrOpenFailed);
    CHECK(!s.isOpen());
    remove(TEST_FILE);
    CHECK(s.open(TEST_FILE, false) == Status::ErrOpenFailed);
    CHECK(sizeOfFile(TEST_FILE) == -1);
}

static void testBadHeader() {
    printf("bad header\n");
    const char junk[] = "this is not a store file at all";
    writeRaw(TEST_FILE, junk, sizeof(junk));
    BackingStore s;
    CHECK(s.open(TEST_FILE) == Status::ErrBadHeader);
    CHECK(!s.isOpen());
    CHECK(sizeOfFile(TEST_FILE) == (long)sizeof(junk));
    writeRaw(TEST_FILE, "BS", 2);
    CHECK(s.open(TEST_FILE) == Status::ErrBadHeader);
    CHECK(sizeOfFile(TEST_FILE) == 2);
    const char oldVersion[8] = {'B', 'S', 'T', 'R', 1, 0, 0, 0};
    writeRaw(TEST_FILE, oldVersion, 8);
    CHECK(s.open(TEST_FILE) == Status::ErrBadHeader);
    writeRaw(TEST_FILE, "", 0);
    CHECK(s.open(TEST_FILE) == Status::Ok);
    CHECK(s.count() == 0);
    s.close();
}

// Record size is 13 + keyLen + valueLen. File header is 8 bytes.
static void testTruncatedTail() {
    printf("truncated tail recovery\n");
    remove(TEST_FILE);
    {
        BackingStore s;
        s.open(TEST_FILE);
        s.put("k1", "first");
        s.put("k2", "second");
        s.put("k3", "third");
        s.close();
    }
    long full = sizeOfFile(TEST_FILE);
    chopFile(TEST_FILE, full - 5);
    BackingStore s;
    CHECK(s.open(TEST_FILE) == Status::Recovered);
    CHECK(s.discardedBytes() == (uint64_t)(13 + 2 + 5 - 5));
    CHECK(s.count() == 2);
    CHECK(valueIs(s, "k1", "first"));
    CHECK(valueIs(s, "k2", "second"));
    CHECK(!s.contains("k3"));
    CHECK((uint64_t)sizeOfFile(TEST_FILE) == s.fileSize());
    CHECK(s.put("k3", "third again") == Status::Ok);
    s.close();
    CHECK(s.open(TEST_FILE) == Status::Ok);
    CHECK(s.count() == 3);
    CHECK(valueIs(s, "k3", "third again"));
    s.close();
}

static void testCorruptedRecordOnOpen() {
    printf("corrupted record found on open\n");
    remove(TEST_FILE);
    {
        BackingStore s;
        s.open(TEST_FILE);
        s.put("k1", "first");
        s.put("k2", "second");
        s.put("k3", "third");
        s.close();
    }
    long secondRecord = 8 + (13 + 2 + 5);
    flipByte(TEST_FILE, secondRecord + 9 + 2 + 2);
    BackingStore s;
    CHECK(s.open(TEST_FILE) == Status::Recovered);
    CHECK(s.count() == 1);
    CHECK(valueIs(s, "k1", "first"));
    CHECK(!s.contains("k2"));
    CHECK(!s.contains("k3"));
    CHECK(s.discardedBytes() == (uint64_t)((13 + 2 + 6) + (13 + 2 + 5)));
    CHECK(sizeOfFile(TEST_FILE) == 8 + 13 + 2 + 5);
    s.close();
}

static void testCorruptedKeyOnOpen() {
    printf("corrupted key found on open\n");
    remove(TEST_FILE);
    {
        BackingStore s;
        s.open(TEST_FILE);
        s.put("k1", "first");
        s.put("k2", "second");
        s.close();
    }
    flipByte(TEST_FILE, 8 + (13 + 2 + 5) + 9);
    BackingStore s;
    CHECK(s.open(TEST_FILE) == Status::Recovered);
    CHECK(s.count() == 1);
    CHECK(valueIs(s, "k1", "first"));
    s.close();
}

static void testCorruptionDetectedOnGet() {
    printf("corruption detected on get\n");
    remove(TEST_FILE);
    BackingStore s;
    s.open(TEST_FILE);
    s.put("k1", "payload");
    s.put("k2", "intact");
    flipByte(TEST_FILE, 8 + 9 + 2 + 3);
    std::string v = "untouched";
    CHECK(s.get("k1", v) == Status::ErrCorrupt);
    CHECK(v == "untouched");
    CHECK(valueIs(s, "k2", "intact"));
    s.close();
}

static void testCounters() {
    printf("disk counters\n");
    remove(TEST_FILE);
    BackingStore s;
    s.open(TEST_FILE);
    CHECK(s.diskWrites() == 0 && s.diskReads() == 0);
    s.put("a", "1");
    s.put("b", "2");
    std::string v;
    s.get("a", v);
    s.get("missing", v);
    CHECK(s.diskWrites() == 2);
    CHECK(s.diskReads() == 1);
    s.close();
}

int main(int argc, char** argv) {
    if (argc > 1) SAMPLE_FILE = argv[1];
    testRoundTrip();
    testMissingKeys();
    testUpdateAndRemove();
    testEmptyAndBinaryValues();
    testKeyVariety();
    testPersistence();
    testSampleData();
    testLargeVolume();
    testLimitsAndArguments();
    testOpenFailures();
    testBadHeader();
    testTruncatedTail();
    testCorruptedRecordOnOpen();
    testCorruptedKeyOnOpen();
    testCorruptionDetectedOnGet();
    testCounters();
    remove(TEST_FILE);
    printf("\n%d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
