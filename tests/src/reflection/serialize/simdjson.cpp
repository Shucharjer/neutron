#if __has_include(<simdjson.h>)
    #include <cstdint>
    #include <optional>
    #include <string>
    #include <string_view>
    #include <vector>
    #include <simdjson.h>
    #include <neutron/reflection.hpp>
    #include "require.hpp"

using namespace neutron;

struct student {
    std::string name;
    std::uint8_t age;
    std::uint32_t id;
};

enum class grade : std::uint8_t { freshman, sophomore, junior, senior };

struct course {
    std::string title;
    std::vector<double> scores;
};

struct enrollment {
    student who;
    course main_course;
    grade year;
};

// Parenthesized aggregate initialization makes this one constructible from a
// std::string_view, which is what simdjson's string-like path looks for. It has
// to be read as an object all the same.
struct label {
    std::string_view text;
    int weight;
};

// Aggregates holding a std::optional cannot be counted by the aggregate
// reflection path, so this one describes its members explicitly.
struct note {
    std::string title;
    std::optional<std::string> body;

    REFL_MEMBERS(note, title, body)
};

// NOLINTBEGIN
int main() {
    simdjson::ondemand::parser parser;

    // deserialize
    {
        simdjson::padded_string text{ std::string_view{
            R"({"name": "Alice", "age": 16, "id": 20260816})" } };

        simdjson::ondemand::document doc;
        require_or_return(
            parser.iterate(text).get(doc) == simdjson::SUCCESS, 1);

        student alice{};
        require_or_return(deserialize(doc, alice) == simdjson::SUCCESS, 1);

        require_or_return(alice.name == "Alice", 1);
        require_or_return(alice.age == 16, 1);
        require_or_return(alice.id == 20260816, 1);
    }

    // deserialize, returning the object. keys may come in any order
    {
        simdjson::padded_string text{ std::string_view{
            R"({"id": 20260816, "name": "Alice", "age": 16})" } };

        simdjson::ondemand::document doc;
        require_or_return(
            parser.iterate(text).get(doc) == simdjson::SUCCESS, 1);

        auto alice = deserialize<student>(doc);
        require_or_return(alice.error() == simdjson::SUCCESS, 1);

        require_or_return(alice.value_unsafe().name == "Alice", 1);
        require_or_return(alice.value_unsafe().age == 16, 1);
        require_or_return(alice.value_unsafe().id == 20260816, 1);
    }

    // deserialize, reporting the error of the first member that failed
    {
        simdjson::padded_string text{ std::string_view{
            R"({"name": "Alice", "id": 20260816})" } };

        simdjson::ondemand::document doc;
        require_or_return(
            parser.iterate(text).get(doc) == simdjson::SUCCESS, 1);

        student alice{};
        require_or_return(
            deserialize(doc, alice) == simdjson::NO_SUCH_FIELD, 1);
    }

    // serialize
    {
        student alice{ .name = "Alice", .age = 16, .id = 20260816 };

        std::string text;
        require_or_return(serialize(text, alice) == simdjson::SUCCESS, 1);
        require_or_return(
            text == R"({"name":"Alice","age":16,"id":20260816})", 1);

        auto another = serialize(alice);
        require_or_return(another.error() == simdjson::SUCCESS, 1);
        require_or_return(another.value_unsafe() == text, 1);
    }

    // serialize into a builder
    {
        student alice{ .name = "Alice", .age = 16, .id = 20260816 };

        simdjson::builder::string_builder builder;
        builder.start_array();
        serialize(builder, alice);
        builder.append_comma();
        serialize(builder, alice);
        builder.end_array();

        std::string_view text;
        require_or_return(builder.view().get(text) == simdjson::SUCCESS, 1);
        require_or_return(
            text == R"([{"name":"Alice","age":16,"id":20260816},)"
                    R"({"name":"Alice","age":16,"id":20260816}])",
            1);
    }

    // round trip: nested aggregates, containers and enums
    {
        enrollment expected{
            .who         = { .name = "Bob", .age = 17, .id = 20260817 },
            .main_course = { .title = "Math", .scores = { 90.5, 78.25 } },
            .year        = grade::junior
        };

        std::string text;
        require_or_return(serialize(text, expected) == simdjson::SUCCESS, 1);

        simdjson::padded_string padded{ text };
        simdjson::ondemand::document doc;
        require_or_return(
            parser.iterate(padded).get(doc) == simdjson::SUCCESS, 1);

        enrollment actual{};
        require_or_return(deserialize(doc, actual) == simdjson::SUCCESS, 1);

        require_or_return(actual.who.name == expected.who.name, 1);
        require_or_return(actual.who.age == expected.who.age, 1);
        require_or_return(actual.who.id == expected.who.id, 1);
        require_or_return(
            actual.main_course.title == expected.main_course.title, 1);
        require_or_return(
            actual.main_course.scores == expected.main_course.scores, 1);
        require_or_return(actual.year == expected.year, 1);
    }

    // aggregates that are constructible from a string_view are still objects
    {
        static_assert(simdjson::concepts::constructible_from_string_view<label>);

        simdjson::padded_string text{ std::string_view{
            R"({"text": "high", "weight": 3})" } };

        simdjson::ondemand::document doc;
        require_or_return(
            parser.iterate(text).get(doc) == simdjson::SUCCESS, 1);

        label actual{};
        require_or_return(deserialize(doc, actual) == simdjson::SUCCESS, 1);
        require_or_return(actual.text == "high", 1);
        require_or_return(actual.weight == 3, 1);

        std::string out;
        require_or_return(serialize(out, actual) == simdjson::SUCCESS, 1);
        require_or_return(out == R"({"text":"high","weight":3})", 1);
    }

    // optional members: written as null, read back from null
    {
        note empty{ .title = "empty", .body = std::nullopt };

        std::string text;
        require_or_return(serialize(text, empty) == simdjson::SUCCESS, 1);
        require_or_return(text == R"({"title":"empty","body":null})", 1);

        simdjson::padded_string padded{ text };
        simdjson::ondemand::document doc;
        require_or_return(
            parser.iterate(padded).get(doc) == simdjson::SUCCESS, 1);

        note actual{ .title = {}, .body = "overwritten" };
        require_or_return(deserialize(doc, actual) == simdjson::SUCCESS, 1);
        require_or_return(actual.title == "empty", 1);
        require_or_return(!actual.body.has_value(), 1);
    }

    // optional members: a missing key is not an error
    {
        simdjson::padded_string text{
            std::string_view{ R"({"title": "no body here"})" }
        };

        simdjson::ondemand::document doc;
        require_or_return(
            parser.iterate(text).get(doc) == simdjson::SUCCESS, 1);

        note actual{ .title = {}, .body = "overwritten" };
        require_or_return(deserialize(doc, actual) == simdjson::SUCCESS, 1);
        require_or_return(actual.title == "no body here", 1);
        require_or_return(!actual.body.has_value(), 1);
    }

    return 0;
}
// NOLINTEND
#else
int main() { return 0; }
#endif
