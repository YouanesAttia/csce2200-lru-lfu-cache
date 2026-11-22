/*
    Tests for the command parser and the startup flags.

    These need no cache at all, which is the point of keeping parsing separate
    from everything else. If the grammar in cli.h changes, this file is what
    catches it.
*/

#include <cassert>
#include <iostream>
#include <string>
#include "../src/app/cli.h"
using namespace std;

void test_simple_commands()
{
    Command c = parseLine("GET apple");
    assert(c.type == CommandType::Get);
    assert(c.key == "apple");

    c = parseLine("DEL apple");
    assert(c.type == CommandType::Del);
    assert(c.key == "apple");

    assert(parseLine("STATS").type == CommandType::Stats);
    assert(parseLine("SWEEP").type == CommandType::Sweep);
    assert(parseLine("HELP").type == CommandType::Help);
    assert(parseLine("EXIT").type == CommandType::Exit);

    cout << "[SUCCESS] test_simple_commands" << endl;
}

void test_case_and_spacing()
{
    // command words are not case sensitive, keys are
    assert(parseLine("get apple").type == CommandType::Get);
    assert(parseLine("GeT apple").type == CommandType::Get);
    assert(parseLine("   GET    apple   ").key == "apple");
    assert(parseLine("GET Apple").key == "Apple");

    cout << "[SUCCESS] test_case_and_spacing" << endl;
}

void test_set_with_and_without_ttl()
{
    Command c = parseLine("SET apple red");
    assert(c.type == CommandType::Set);
    assert(c.key == "apple");
    assert(c.value == "red");
    assert(c.hasTtl == false); // no TTL means it never expires

    c = parseLine("SET apple red TTL 30");
    assert(c.type == CommandType::Set);
    assert(c.hasTtl == true);
    assert(c.ttl == 30);

    c = parseLine("SET apple red ttl 30"); // TTL keyword is case insensitive too
    assert(c.hasTtl == true);

    c = parseLine("SET apple red TTL 0"); // a zero TTL is allowed
    assert(c.hasTtl == true);
    assert(c.ttl == 0);

    cout << "[SUCCESS] test_set_with_and_without_ttl" << endl;
}

void test_quoted_values()
{
    Command c = parseLine("SET greeting \"hello world\"");
    assert(c.type == CommandType::Set);
    assert(c.value == "hello world");

    c = parseLine("SET greeting \"hello world\" TTL 5");
    assert(c.value == "hello world");
    assert(c.ttl == 5);

    c = parseLine("SET empty \"\""); // an empty value is allowed
    assert(c.type == CommandType::Set);
    assert(c.value == "");

    c = parseLine("SET broken \"unclosed");
    assert(c.type == CommandType::Invalid);
    assert(c.errorName == "UnclosedQuote");

    cout << "[SUCCESS] test_quoted_values" << endl;
}

void test_policy_and_advance()
{
    Command c = parseLine("POLICY lfu");
    assert(c.type == CommandType::Policy);
    assert(c.policy == "lfu");

    assert(parseLine("POLICY LRU").policy == "lru"); // normalised to lowercase
    assert(parseLine("POLICY fifo").type == CommandType::Invalid);
    assert(parseLine("POLICY fifo").errorName == "BadPolicy");

    c = parseLine("ADVANCE 60");
    assert(c.type == CommandType::Advance);
    assert(c.seconds == 60);

    assert(parseLine("ADVANCE later").errorName == "BadNumber");

    cout << "[SUCCESS] test_policy_and_advance" << endl;
}

void test_blank_and_comments()
{
    assert(parseLine("").type == CommandType::Empty);
    assert(parseLine("      ").type == CommandType::Empty);
    assert(parseLine("# this is a comment").type == CommandType::Empty);
    assert(parseLine("   # indented comment").type == CommandType::Empty);

    // A comment can be longer than any real command, so it has to be spotted
    // before the line is split into tokens. This one has more words than the
    // tokeniser allows, and must still come back as Empty rather than an error.
    assert(parseLine("# capacity is 2, so this should evict something").type
           == CommandType::Empty);

    cout << "[SUCCESS] test_blank_and_comments" << endl;
}

void test_bad_input()
{
    assert(parseLine("FLY apple").errorName == "UnknownCommand");
    assert(parseLine("GET").errorName == "MissingArgument");
    assert(parseLine("GET a b").errorName == "TooManyArguments");
    assert(parseLine("SET apple").errorName == "MissingArgument");
    assert(parseLine("SET a b TTL").errorName == "TooManyArguments");
    assert(parseLine("SET a b XTL 5").errorName == "UnknownCommand");
    assert(parseLine("SET a b TTL soon").errorName == "BadNumber");
    assert(parseLine("STATS now").errorName == "TooManyArguments");

    cout << "[SUCCESS] test_bad_input" << endl;
}

void test_startup_flags()
{
    // defaults when nothing is passed
    char *none[] = {(char *)"cache"};
    Options o = parseArgs(1, none);
    assert(o.ok == true);
    assert(o.policy == "lru");
    assert(o.fakeClock == false);

    char *full[] = {(char *)"cache", (char *)"--policy", (char *)"lfu",
                    (char *)"--capacity", (char *)"500",
                    (char *)"--db", (char *)"data/test.db",
                    (char *)"--fake-clock"};
    o = parseArgs(8, full);
    assert(o.ok == true);
    assert(o.policy == "lfu");
    assert(o.capacity == 500);
    assert(o.dbPath == "data/test.db");
    assert(o.fakeClock == true);

    char *badFlag[] = {(char *)"cache", (char *)"--colour"};
    assert(parseArgs(2, badFlag).ok == false);

    char *noValue[] = {(char *)"cache", (char *)"--capacity"};
    o = parseArgs(2, noValue);
    assert(o.ok == false);
    assert(o.errorName == "MissingArgument");

    char *badNumber[] = {(char *)"cache", (char *)"--capacity", (char *)"lots"};
    assert(parseArgs(3, badNumber).errorName == "BadNumber");

    cout << "[SUCCESS] test_startup_flags" << endl;
}

int main()
{
    test_simple_commands();
    test_case_and_spacing();
    test_set_with_and_without_ttl();
    test_quoted_values();
    test_policy_and_advance();
    test_blank_and_comments();
    test_bad_input();
    test_startup_flags();
    cout << "ALL TESTS PASSED!" << endl;
    return 0;
}
