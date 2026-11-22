#include "cli.h"
#include "../../include/metrics/stats.hpp"

#include <iomanip>
#include <iostream>
using namespace std;

// The longest valid command is SET <key> <value> TTL <seconds>, which is five
// tokens. Eight leaves room to notice a line with too many and report it
// instead of quietly ignoring the extra ones.
static const int MAX_TOKENS = 8;

// ---------------------------------------------------------------------------
// small helpers
// ---------------------------------------------------------------------------

// Uppercase a copy, so "get" and "GET" are the same command. Lowercase letters
// sit 32 above their uppercase versions in ASCII, so subtracting 32 converts.
static string upper(const string &text)
{
    string out = text;
    for (size_t i = 0; i < out.size(); ++i)
        if (out[i] >= 'a' && out[i] <= 'z')
            out[i] = (char)(out[i] - 32);
    return out;
}

static string lower(const string &text)
{
    string out = text;
    for (size_t i = 0; i < out.size(); ++i)
        if (out[i] >= 'A' && out[i] <= 'Z')
            out[i] = (char)(out[i] + 32);
    return out;
}

// Read a whole number. Written by hand rather than with stoi because stoi
// throws on bad input, and a typed command line is expected to contain bad
// input sometimes. Returns false instead.
static bool parseNumber(const string &text, uint64_t &out)
{
    if (text.empty())
        return false;

    uint64_t value = 0;
    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] < '0' || text[i] > '9')
            return false;
        value = value * 10 + (uint64_t)(text[i] - '0'); // '0' is 48 in ASCII
    }
    out = value;
    return true;
}

/*
    Split a line into tokens on spaces and tabs.

    A token wrapped in double quotes keeps its spaces, which is how a value
    with spaces gets through. There is no escape character, so a value cannot
    contain a double quote itself.
*/
static bool tokenize(const string &line, string *tokens, int &count, string &errorName)
{
    count = 0;
    size_t i = 0;

    while (i < line.size())
    {
        while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
            ++i;
        if (i >= line.size())
            break;

        if (count == MAX_TOKENS)
        {
            errorName = "TooManyArguments";
            return false;
        }

        string piece;
        if (line[i] == '"')
        {
            ++i; // step past the opening quote
            while (i < line.size() && line[i] != '"')
            {
                piece += line[i];
                ++i;
            }
            if (i >= line.size())
            {
                errorName = "UnclosedQuote";
                return false;
            }
            ++i; // step past the closing quote
        }
        else
        {
            while (i < line.size() && line[i] != ' ' && line[i] != '\t')
            {
                piece += line[i];
                ++i;
            }
        }

        tokens[count] = piece;
        ++count;
    }
    return true;
}

static Command invalid(const string &errorName)
{
    Command cmd;
    cmd.type = CommandType::Invalid;
    cmd.errorName = errorName;
    return cmd;
}

// ---------------------------------------------------------------------------
// parsing
// ---------------------------------------------------------------------------

Command parseLine(const string &line)
{
    Command cmd;

    // A blank line, or one that starts with #, is skipped. Scripts given to
    // RUN are easier to read with comments in them.
    //
    // This is checked before the line is split up, not after. A comment can be
    // far longer than the five tokens a real command ever needs, so splitting
    // first would reject a long comment as TooManyArguments.
    size_t firstCharacter = 0;
    while (firstCharacter < line.size() &&
           (line[firstCharacter] == ' ' || line[firstCharacter] == '\t'))
        ++firstCharacter;

    if (firstCharacter >= line.size() || line[firstCharacter] == '#')
    {
        cmd.type = CommandType::Empty;
        return cmd;
    }

    string tokens[MAX_TOKENS];
    int count = 0;
    string tokenError;

    if (!tokenize(line, tokens, count, tokenError))
        return invalid(tokenError);

    const string word = upper(tokens[0]);

    if (word == "SET")
    {
        // SET <key> <value>  or  SET <key> <value> TTL <seconds>
        if (count < 3)
            return invalid("MissingArgument");
        if (count != 3 && count != 5)
            return invalid("TooManyArguments");

        cmd.type = CommandType::Set;
        cmd.key = tokens[1];
        cmd.value = tokens[2];

        if (count == 5)
        {
            if (upper(tokens[3]) != "TTL")
                return invalid("UnknownCommand");
            if (!parseNumber(tokens[4], cmd.ttl))
                return invalid("BadNumber");
            cmd.hasTtl = true;
        }
        return cmd;
    }

    if (word == "GET" || word == "DEL")
    {
        if (count < 2)
            return invalid("MissingArgument");
        if (count > 2)
            return invalid("TooManyArguments");

        cmd.type = (word == "GET") ? CommandType::Get : CommandType::Del;
        cmd.key = tokens[1];
        return cmd;
    }

    if (word == "POLICY")
    {
        if (count < 2)
            return invalid("MissingArgument");
        if (count > 2)
            return invalid("TooManyArguments");

        const string name = lower(tokens[1]);
        if (name != "lru" && name != "lfu")
            return invalid("BadPolicy");

        cmd.type = CommandType::Policy;
        cmd.policy = name;
        return cmd;
    }

    if (word == "ADVANCE")
    {
        if (count < 2)
            return invalid("MissingArgument");
        if (count > 2)
            return invalid("TooManyArguments");
        if (!parseNumber(tokens[1], cmd.seconds))
            return invalid("BadNumber");

        cmd.type = CommandType::Advance;
        return cmd;
    }

    if (word == "RUN")
    {
        if (count < 2)
            return invalid("MissingArgument");
        if (count > 2)
            return invalid("TooManyArguments");

        cmd.type = CommandType::Run;
        cmd.file = tokens[1];
        return cmd;
    }

    if (word == "STATS" || word == "SWEEP" || word == "HELP" || word == "EXIT")
    {
        if (count > 1)
            return invalid("TooManyArguments");

        if (word == "STATS")
            cmd.type = CommandType::Stats;
        else if (word == "SWEEP")
            cmd.type = CommandType::Sweep;
        else if (word == "HELP")
            cmd.type = CommandType::Help;
        else
            cmd.type = CommandType::Exit;
        return cmd;
    }

    return invalid("UnknownCommand");
}

Options parseArgs(int argc, char **argv)
{
    Options options;

    for (int i = 1; i < argc; ++i)
    {
        const string flag = argv[i];

        if (flag == "--fake-clock")
        {
            options.fakeClock = true;
            continue;
        }

        // Everything below needs a value after it.
        if (flag == "--policy" || flag == "--capacity" || flag == "--db")
        {
            if (i + 1 >= argc)
            {
                options.ok = false;
                options.errorName = "MissingArgument";
                return options;
            }
            const string argument = argv[i + 1];
            ++i;

            if (flag == "--policy")
            {
                const string name = lower(argument);
                if (name != "lru" && name != "lfu")
                {
                    options.ok = false;
                    options.errorName = "BadPolicy";
                    return options;
                }
                options.policy = name;
            }
            else if (flag == "--capacity")
            {
                uint64_t number = 0;
                if (!parseNumber(argument, number))
                {
                    options.ok = false;
                    options.errorName = "BadNumber";
                    return options;
                }
                options.capacity = (size_t)number;
            }
            else
            {
                options.dbPath = argument;
            }
            continue;
        }

        options.ok = false;
        options.errorName = "UnknownFlag";
        return options;
    }

    return options;
}

// ---------------------------------------------------------------------------
// output
// ---------------------------------------------------------------------------

void printOk() { cout << "OK" << endl; }
void printEvicted(const string &key) { cout << "EVICTED: " << key << endl; }
void printHit(const string &value) { cout << "HIT " << value << endl; }
void printMissStore(const string &value) { cout << "MISS (store) " << value << endl; }
void printNotFound() { cout << "NOT FOUND" << endl; }
void printDeleted() { cout << "DELETED" << endl; }
void printPolicy(const string &name) { cout << "POLICY " << name << endl; }
void printExpired(uint64_t count) { cout << "EXPIRED " << count << endl; }
void printTime(uint64_t seconds) { cout << "TIME " << seconds << endl; }
void printError(const string &name) { cout << "ERROR: " << name << endl; }

void printStats(const Stats &stats)
{
    cout << "HITS " << stats.hits << endl;
    cout << "MISSES " << stats.misses << endl;
    cout << "EVICTIONS " << stats.evictions << endl;
    cout << "EXPIRATIONS " << stats.expirations << endl;
    cout << "STORE_READS " << stats.storeReads << endl;
    cout << "STORE_WRITES " << stats.storeWrites << endl;
    cout << "HIT_RATE " << fixed << setprecision(2) << stats.hitRate() << endl;
}

void printHelp()
{
    cout << "SET <key> <value> [TTL <seconds>]  store a value" << endl;
    cout << "GET <key>                          look a value up" << endl;
    cout << "DEL <key>                          remove a value" << endl;
    cout << "POLICY lru|lfu                     switch eviction policy" << endl;
    cout << "STATS                              print the counters" << endl;
    cout << "SWEEP                              remove expired entries now" << endl;
    cout << "ADVANCE <seconds>                  move the fake clock forward" << endl;
    cout << "RUN <file>                         run commands from a file" << endl;
    cout << "HELP                               this list" << endl;
    cout << "EXIT                               quit" << endl;
    cout << endl;
    cout << "A value with spaces must be quoted: SET k \"hello world\"" << endl;
    cout << "Lines starting with # are ignored." << endl;
}

void printUsage()
{
    cout << "usage: cache [--policy lru|lfu] [--capacity <n>] "
         << "[--db <path>] [--fake-clock]" << endl;
}
