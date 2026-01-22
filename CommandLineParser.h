/*
 *  File: CommandLineParser.h
 *  Copyright (c) 2023 Florian Porrmann
 *
 *  MIT License
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 *
 */

#pragma once

#include <algorithm>
#include <deque>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#else
#include <libgen.h>
#endif

/**
 * TODO:
 *  - Currently, when an option is required there is no way to implement direct exit options like version
 * **/

class CommandLineOption
{
public:
	enum class HasValue
	{
		Yes,
		No
	};

	enum class Required
	{
		Yes,
		No
	};

	enum class Separator
	{
		Yes,
		No
	};

public:
	CommandLineOption(const std::string& arg, const std::string& argAlt, const std::string& desc,
					  const std::string& defaultValue, const HasValue& hasValue, const Required& required, const Separator& separator) :
		m_arg(arg),
		m_argAlt(argAlt),
		m_desc(desc),
		m_default(defaultValue),
		m_required(required == Required::Yes),
		m_hasValue(hasValue == HasValue::Yes),
		m_isSeparator(separator == Separator::Yes)
	{
	}

	CommandLineOption(const char* pArg, const char* pArgAlt, const char* pDesc, const char* pDefaultValue,
					  const HasValue& hasValue, const Required& required, const Separator& separator) :
		CommandLineOption(std::string(pArg), std::string(pArgAlt), std::string(pDesc), std::string(pDefaultValue), hasValue, required, separator)
	{
	}

	// If a default value is set, the option has to have a value
	// When calling the constructor with a plain char* as default parameter, e.g., "DEFAULT"
	// not the std::string constructor would be used, but the one for hasValue, because converting
	// from char* to bool is a standard conversion, while char* to std::string is a user-defined conversion
	// See: https://stackoverflow.com/a/26414524
	// Therefore, a char* based constructor is available.
	CommandLineOption(const std::string& arg, const std::string& argAlt, const std::string& desc, const char* pDefault, const Required& required = Required::No) :
		CommandLineOption(arg, argAlt, desc, std::string(pDefault), required)
	{
	}

	CommandLineOption(const std::string& arg, const std::string& argAlt, const std::string& desc, const std::string& defaultValue, const Required& required = Required::No) :
		CommandLineOption(arg, argAlt, desc, defaultValue, HasValue::Yes, required, Separator::No)
	{
	}

	CommandLineOption(const std::string& arg, const std::string& argAlt, const std::string& desc, const HasValue& hasValue = HasValue::Yes, const Required& required = Required::No) :
		CommandLineOption(arg, argAlt, desc, "", hasValue, required, Separator::No)
	{
	}

	bool check(const std::string& arg)
	{
		// Do not expect the same option to be selected twice ...
		// This is required to prevent set parameters from being
		// overritten by following checks against different parameters
		if (m_set)
			return false;

		std::stringstream ss(m_argAlt);
		std::string argAltArg = "";
		if (!ss.eof())
			ss >> argAltArg;

		if (!m_arg.empty())
			m_set = m_arg == arg;

		if (!m_set && !argAltArg.empty())
			m_set = argAltArg == arg;

		return m_set;
	}

	bool isSet() const
	{
		// In case a default value has been set (default not empty) return true
		return (m_set || !(m_default.empty()));
	}

	void setValue(const std::string& value)
	{
		m_value = value;
	}

	const std::string& getValue() const
	{
		if (m_set)
			return m_value;
		else
			return m_default;
	}

	bool isRequired() const
	{
		return m_required;
	}

	void setRequired(const bool& required)
	{
		m_required = required;
	}

	bool hasValue() const
	{
		return m_hasValue;
	}

	void setSpaceAdd(const size_t& spaceAdd)
	{
		m_addSpace = spaceAdd;
	}

	const std::string& getArg() const
	{
		return m_arg;
	}

	const std::string& getArgAlt() const
	{
		return m_argAlt;
	}

	void setDefault(const std::string& defaultValue)
	{
		m_default = defaultValue;
	}

	const std::string& getDefault() const
	{
		return m_default;
	}

	friend std::ostream& operator<<(std::ostream& os, const CommandLineOption& clo)
	{
		// Windows cmd default width is 80
		const size_t maxLineLen = 80;

		const size_t spaceArgDesc = 4;

		if (clo.m_isSeparator)
			os << std::endl;
		else
		{
			std::string argStr(clo.m_arg + ", " + clo.m_argAlt);
			os << std::left << std::setfill(' ') << std::setw(clo.m_addSpace) << argStr;
			os << std::setfill(' ') << std::setw(spaceArgDesc) << "";

			std::string desc = clo.m_desc;

			if (clo.m_required)
				desc.append(" (required)");

			if (!(clo.m_default.empty()))
			{
				desc.append(" DEFAULT: ");
				desc.append(clo.m_default);
			}

			while (desc.length() + spaceArgDesc + clo.m_addSpace > maxLineLen)
			{
				size_t spacePos = desc.find_last_of(' ', maxLineLen - (spaceArgDesc + clo.m_addSpace));
				os << desc.substr(0, spacePos) << std::endl;
				os << std::setfill(' ') << std::setw(spaceArgDesc + clo.m_addSpace) << "";
				desc = desc.substr(spacePos + 1);
			}

			os << desc << std::endl;
		}

		return os;
	}

	bool operator==(const CommandLineOption& rhs) const
	{
		if (this == &rhs)
			return true;

		return m_arg == rhs.m_arg && m_argAlt == rhs.m_argAlt && m_desc == rhs.m_desc;
	}

	size_t getArgsLength() const
	{
		if (m_isSeparator) return 0;

		return std::string(m_arg + ", " + m_argAlt).size();
	}

private:
	std::string m_arg;
	std::string m_argAlt;
	std::string m_desc;
	std::string m_value = "";
	std::string m_default;
	bool m_set = false;
	bool m_required;
	bool m_hasValue;
	bool m_isSeparator;
	size_t m_addSpace = 0;
};

using CLO = CommandLineOption;

class CommandLineParser
{
	using CommandLineOptions = std::deque<CommandLineOption>;

public:
	CommandLineParser(const int argc, char** argv, const std::string& programName = "", const std::string& programVersion = "") :
		m_options(),
		m_argc(argc),
		m_argv(argv),
		m_programName(programName),
		m_programVersion(programVersion)
	{
	}

	CommandLineParser(const CommandLineParser&)            = delete; // disable copy constructor
	CommandLineParser& operator=(const CommandLineParser&) = delete; //  disable assignment constructor

	void addVersionOption()
	{
		m_options.push_back(m_verOpt);
	}

	void addVersionOption(const CommandLineOption& versionOpt)
	{
		m_verOpt = versionOpt;
		addVersionOption();
	}

	void addOption(const CommandLineOption& opt)
	{
		m_options.push_back(opt);
	}

	void addSeparator()
	{
		m_options.push_back(CommandLineOption("", "", "", "", CLO::HasValue::No, CLO::Required::No, CLO::Separator::Yes));
	}

	void addHelpOption()
	{
		m_options.push_front(m_helpOpt);
	}

	void parse(const bool& requireMatch = true)
	{
		bool anyMatch       = false;
		bool allRequiredSet = true;

		for (int i = 1; i < m_argc; i++)
		{
			std::string str = m_argv[i];

			for (CommandLineOption& option : m_options)
			{
				if (option.check(str))
				{
					if (option.hasValue())
					{
						i++;

						if (i >= m_argc)
						{
							std::cerr << "ERROR: Option (" << option.getArg() << " / " << option.getArgAlt() << ") requires a value, but none was provided, exiting ..." << std::endl;
							exit(-1);
						}

						option.setValue(m_argv[i]);
					}

					anyMatch = true;
				}
			}
		}

		if (isSet(m_helpOpt) || (!anyMatch && requireMatch))
		{
			printHelp();
			exit(0);
		}

		if (isSet(m_verOpt))
		{
			printVersion();
			exit(0);
		}

		for (CommandLineOption& option : m_options)
		{
			if (option.isRequired() && !option.isSet())
			{
				std::cerr << "ERROR: Required option (" << option.getArg() << " / " << option.getArgAlt() << ") not set, exiting ..." << std::endl;
				allRequiredSet = false;
			}
		}

		if (!allRequiredSet)
			exit(-1);
	}

	bool isSet(const CommandLineOption& opt) const
	{
		CommandLineOptions::const_iterator result = std::find(m_options.begin(), m_options.end(), opt);

		if (result == m_options.end())
			return false;
		else
			return result->isSet();
	}

	std::string getValue(const CommandLineOption& opt) const
	{
		CommandLineOptions::const_iterator result = std::find(m_options.begin(), m_options.end(), opt);

		if (result == m_options.end())
			return "";
		else
			return result->getValue();
	}

	std::vector<std::string> getValueList(const CommandLineOption& opt, const std::string delim = ",") const
	{
		CommandLineOptions::const_iterator result = std::find(m_options.begin(), m_options.end(), opt);

		if (result == m_options.end())
			return std::vector<std::string>();
		else
			return splitString(result->getValue(), delim);
	}

private:
	void printHelp()
	{
#ifdef _WIN32
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char pFileName[_MAX_FNAME];
		char ext[_MAX_EXT];
		_splitpath_s(m_argv[0], drive, dir, pFileName, ext);
#else
		char* pFileName = basename(m_argv[0]);
#endif

		updateAddSpaces();
		std::cout << "Usage: " << pFileName << " option" << std::endl
				  << std::endl;

		for (const CommandLineOption& option : m_options)
			std::cout << option;
	}

	void printVersion()
	{
		if (!m_programName.empty())
			std::cout << m_programName;

		if (!m_programVersion.empty())
		{
			if (!m_programName.empty())
				std::cout << " - ";

			std::cout << m_programVersion;
		}

		std::cout << std::endl;
	}

	void updateAddSpaces()
	{
		// Find the element with the largest argument length and use that length as the max length
		size_t maxLen = std::max_element(m_options.begin(), m_options.end(), [](CommandLineOption a, CommandLineOption b) { return a.getArgsLength() < b.getArgsLength(); })->getArgsLength();

		for (CommandLineOption& option : m_options)
			option.setSpaceAdd(maxLen);
	}

	static std::vector<std::string> splitString(const std::string& s, const std::string& delimiter = " ")
	{
		std::vector<std::string> split;
		std::string item;
		std::istringstream stream(s);

		char delim = ',';

		if (!delimiter.empty())
			delim = delimiter.at(0);

		while (std::getline(stream, item, delim))
			split.push_back(item);

		return split;
	}

private:
	CommandLineOptions m_options;
	int m_argc;
	char** m_argv;
	std::string m_programName;
	std::string m_programVersion;
	CommandLineOption m_helpOpt = CommandLineOption("-h", "--help", "Displays Help", CLO::HasValue::No);
	CommandLineOption m_verOpt  = CommandLineOption("-v", "--version", "Print the version", CLO::HasValue::No);
};
