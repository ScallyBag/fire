/*
-----------------------------------------------------------------------------
This source file is part of the Havoc chess engine
Copyright (c) 2020 Minniesoft
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

/* adapted and modified for use in fire via clang core cpp guidelines:
vars moved to inner scope
member function naming
removed template arguments
use 'default' for class destructor init
argc made const
removed redundant inline specifiers
initialized template vars
*/

#pragma once

#ifndef OPTIONS_H
#define OPTIONS_H

#include <string>
#include <sstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <fstream>
#include <vector>

#include "evaluate.h"
#include "search.h"
#include "util/misc.h"

class options {
	std::mutex m;
	std::map<std::string, std::string> opts;
	void load_args(int argc, char* argv[]);

public:
	options() = delete;
	options(const options&) = delete;
	options(const options&&) = delete;
	options& operator=(const options&) = delete;
	options& operator=(const options&&) = delete;

	options(const int argc, char* argv[]) { load_args(argc, argv); }
	~options() = default;

	template<typename T>
	T value(const char* s) {
		std::unique_lock<std::mutex> lock(m);
		std::istringstream ss{ opts[std::string{s}] };
		T v{};
		ss >> v;
		return v;
	}

	template<typename T>
	void set(const std::string key, const T value) {
		std::unique_lock lock(m);
		
		if (std::string vs = std::to_string(value); opts.find(key) == opts.end()) {
			opts.emplace(key, vs);
		}
		else opts[key] = vs;
	}

	bool read_param_file(std::string& filename);
	bool save_param_file(std::string& filename);
	void set_engine_params();
};

inline void options::load_args(int argc, char* argv[]) {

	auto matches = [](std::string& s1, const char* s2) { return strcmp(s1.c_str(), s2) == 0; };
	auto set = [this](std::string& k, std::string& v) { this->opts.emplace(k.substr(1), v); };

	for (int j = 1; j < argc - 1; j += 2) {
		std::string key = argv[j];
		if (std::string val = argv[j + 1]; matches(key, "-threads")) set(key, val);
		else if (matches(key, "-book")) set(key, val);
		else if (matches(key, "-hash")) set(key, val);
		else if (matches(key, "-tune")) set(key, val);
		else if (matches(key, "-bench")) set(key, val);
		else if (matches(key, "-param")) set(key, val);
	}
}

inline bool options::read_param_file(std::string& filename) {
	std::string line;

	if (filename.empty()) {
		filename = value<std::string>("param");
		if (filename.empty()) {
			filename = "engine.conf";
		}
	}
	else acout() << "info string...reading param file " << filename << std::endl;

	std::ifstream param_file(filename);
	auto set = [this](std::string& k, std::string& v) { this->opts.emplace(k, v); };

	while (std::getline(param_file, line)) {

	// assumed format "param-tag:param-value"
    std::vector<std::string> tokens = util::split(line, ':');

		if (tokens.size() != 2) {
			acout() << "info string...skipping invalid line" << line << std::endl;
			continue;
		}

		set(tokens[0], tokens[1]);
		//acout() << "stored param: " << tokens[0] << " value: " << tokens[1] << std::endl;
	}

	set_engine_params();
	return true;
}

inline bool options::save_param_file(std::string& filename) {

	if (filename.empty()) {
		filename = value<std::string>("param");
		if (filename.empty()) {
			filename = "engine.conf"; // default
		}
	}

	std::ofstream param_file(filename, std::ofstream::out);

	for (const auto& p : opts) {
		std::string line = p.first + ":" + p.second + "\n";
		param_file << line;
		//acout() << "saved " << line << " into engine.conf " << std::endl;
	}
	param_file.close();
	return true;
}

inline void options::set_engine_params() {
	auto matches = [](const std::string& s1, const char* s2) { return strcmp(s1.c_str(), s2) == 0; };

	for (const auto& p : opts) {
		if (matches(p.first, "razor_margin")) {search::razor_margin = value<int>("razor_margin"); acout() << "info string " << p.first << " = " << p.second << std::endl;}
		else if (matches(p.first, "futility_value_0")) {search::futility_value_0 = value<int>("futility_value_0"); acout() << "info string " << p.first << " = " << p.second << std::endl;}
		else if (matches(p.first, "futility_value_1")) {search::futility_value_1 = value<int>("futility_value_1"); acout() << "info string " << p.first << " = " << p.second << std::endl;}
		else if (matches(p.first, "futility_value_2")) {search::futility_value_2 = value<int>("futility_value_2"); acout() << "info string " << p.first << " = " << p.second << std::endl;}
		else if (matches(p.first, "futility_value_3")) {search::futility_value_3 = value<int>("futility_value_3"); acout() << "info string " << p.first << " = " << p.second << std::endl;}
		else if (matches(p.first, "futility_value_4")) {search::futility_value_4 = value<int>("futility_value_4"); acout() << "info string " << p.first << " = " << p.second << std::endl;}
		else if (matches(p.first, "futility_value_5")) {search::futility_value_5 = value<int>("futility_value_5"); acout() << "info string " << p.first << " = " << p.second << std::endl;}
		else if (matches(p.first, "futility_value_6")) {search::futility_value_6 = value<int>("futility_value_6"); acout() << "info string " << p.first << " = " << p.second << std::endl;}
		else if (matches(p.first, "futility_margin_ext_base")) {search::futility_margin_ext_base = value<int>("futility_margin_ext_base"); acout() << "info string " << p.first << " = " << p.second << std::endl;}
		else if (matches(p.first, "futility_margin_ext_mult")) {search::futility_margin_ext_mult = value<int>("futility_margin_ext_mult"); acout() << "info string " << p.first << " = " << p.second << std::endl;}
			
	}
}

extern std::unique_ptr<options> opts;

#endif
