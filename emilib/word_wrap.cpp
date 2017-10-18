// By Emil Ernerfeldt 2015-2016
// LICENSE:
//   This software is dual-licensed to the public domain and under the following
//   license: you are granted a perpetual, irrevocable license to copy, modify,
//   publish, and distribute this file as you see fit.

#include "word_wrap.hpp"

namespace emilib {

std::vector<std::string> naive_word_wrap(
	const std::string& text,
	float              max_width,
	const CalcWidth&   calc_width)
{
	std::vector<std::string> result;
	std::string line;

	size_t cursor = 0;
	while (cursor < text.size()) {
		auto space = text.find(' ', cursor);
		std::string word;
		if (space == std::string::npos) {
			word = text.substr(cursor);
		} else {
			word = text.substr(cursor, space - cursor);
		}

		if (line.empty()) {
			line = word;
		} else {
			// Can we fit another word?
			auto line_and_word = line + " " + word;
			if (calc_width(line_and_word) <= max_width) {
				line.swap(line_and_word);
			} else {
				result.emplace_back(std::move(line));
				line = word;
			}
		}

		if (space == std::string::npos) {
			break;
		}

		cursor = space + 1;
	}

	if (!line.empty()) {
		result.push_back(line);
	}

	return result;
}

std::vector<int> find_all_of_pattern_in(const std::string& pattern, const std::string& text)
{
	std::vector<int> results;
	size_t pos = 0;
	for (;;) {
		auto hit = text.find(pattern, pos);
		if (hit == std::string::npos) {
			break;
		}
		results.push_back(static_cast<int>(hit));
		pos = hit + 1;
	}
	return results;
}

int closest_to(const std::vector<int>& values, int target)
{
	CHECK_F(!values.empty());
	int closest = values[0];
	for (auto value : values) {
		if (std::abs(target - value) < std::abs(closest - value)) {
			closest = value;
		}
	}
	return closest;
}

void word_wrap_line(
	std::vector<std::string>* io_result,
	const std::string&        text,
	float                     max_width,
	const CalcWidth&          calc_width)
{
	if (calc_width(text) <= max_width) {
		io_result->push_back(text);
		return;
	}

	const auto kPatterns = std::vector<std::string>{
		". ",
		"! ",
		"? ",
		"… ",
		"; ",
		", ",
		" ",
	};

	for (const auto& pattern : kPatterns) {
		const auto breaks = find_all_of_pattern_in(pattern, text);
		if (!breaks.empty()) {
			size_t pos = closest_to(breaks, static_cast<int>(text.size() / 2));
			const auto first = text.substr(0, pos + pattern.size() - 1);
			const auto second = text.substr(pos + pattern.size());

			if (calc_width(first)  <= max_width
			 && calc_width(second) <= max_width) {
				io_result->push_back(first);
				io_result->push_back(second);
				return;
			}
		}
	}

	for (const auto& line : naive_word_wrap(text, max_width, calc_width)) {
		io_result->push_back(line);
	}
}

std::vector<std::string> word_wrap(
	const std::string& text,
	float              max_width,
	const CalcWidth&   calc_width)
{
	std::vector<std::string> result;

	size_t start = 0;
	for (;;) {
		size_t newline = text.find('\n', start);
		if (newline == std::string::npos) {
			break;
		}
		word_wrap_line(&result, text.substr(start, newline - start), max_width, calc_width);
		start = newline + 1;
	}
	word_wrap_line(&result, text.substr(start), max_width, calc_width);
	return result;
}

// ----------------------------------------------------------------------------

void test_wrap(const std::string& break_where, const std::string& text,
               const std::vector<std::string>& expected_output)
{
	ERROR_CONTEXT("Wrap text", text.c_str());

	const float max_width = break_where.size();
	const CalcWidth calc_width = [](const std::string& t) { return t.size(); };

	const auto actual_lines = word_wrap(text, max_width, calc_width);

	CHECK_EQ_F(expected_output.size(), actual_lines.size());
	for (size_t i = 0; i < actual_lines.size(); ++i) {
		ERROR_CONTEXT("Line number (zero indexed)", i);
		ERROR_CONTEXT("Expected line", expected_output[i].c_str());
		ERROR_CONTEXT("Actual line", actual_lines[i].c_str());
		CHECK_F(expected_output[i] == actual_lines[i]);
	}
}

void unit_test_word_wrap()
{
	test_wrap(
		"                    |",
		"This sentence should wrap.",
		{
		"This sentence",
		"should wrap.",
		}
	);

	test_wrap(
		"             |",
		"This sentence should wrap two times.",
		{
		"This sentence",
		"should wrap",
		"two times.",
		}
	);
}

} // namespace emilib
