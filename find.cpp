#include <iostream>
#include <string>
#include <cstring>

#include <vector>
#include <unordered_set>
#include <unordered_map>

#include <functional>
#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

namespace console {

    std::string _ERROR = "\033[31m";
    std::string _DEFAULT = "\033[0m";
    std::string _BOLD = "\033[1m";

    std::string REPORT_HELP = "use " + _DEFAULT + "./find -help" + _ERROR + " to view help reference";
    // @formatter:off
    std::string USAGE = "find utility v.1.0.0\n"
                        "Help:  ./find -help\n"
                        "Usage: ./find PATH [-inum INUM] [-name NAME] [-size (-|=|+)SIZE] [-nlinks NLINKS] [-exec EPATH]\n"
                        "\t- PATH is an absolute path to the directory for searching\n"
                        "\t- INUM is a number of " + _BOLD + "inode" + _DEFAULT + "\n"
                        "\t- NAME is a name of the file\n"
                        "\t- SIZE is a size of the file (- for Lesser, = for Equal, + for Greater)\n"
                        "\t- NLINKS is a number of " + _BOLD + "hardlinks" + _DEFAULT + "\n"
                        "\t- EPATH is an absolute path to the file that should be executed on each found entity\n";
    // @formatter:on

    int report(std::string const &message, int err = 0) {
        std::cerr << _ERROR << message;
        if (err != 0) {
            std::cerr << std::strerror(errno);
        }
        std::cerr << std::endl << _DEFAULT;
        return 0;
    }

}

namespace files {

    class full_stat {

        std::string _file_name;
        struct stat _file_stat;

    public:

        full_stat(std::string const &path) : _file_name(path) {
            if (stat(path.c_str(), &_file_stat) != 0) {
                throw std::logic_error("kek");
            }
        }

        std::string const &get_name() const {
            return _file_name;
        }
        struct stat const &get_stat() const {
            return _file_stat;
        }

    };

    bool file_exists(std::string const &path) {
        return access(path.c_str(), F_OK) != -1;
    }

}

namespace filter {

    enum class filter_type {
        INUM,
        NAME,
        SIZE_LESSER,
        SIZE_EQUAL,
        SIZE_GREATER,
        NLINKS,
    };

}

namespace std {

    template<>
    struct hash<filter::filter_type> {
        std::size_t operator()(filter::filter_type const &type) const {
            return static_cast<typename std::underlying_type_t<filter::filter_type>>(type);
        }
    };

}

namespace filter {

    struct filter_exception : public std::runtime_error {

        filter_exception(std::string const &message) : runtime_error(message) {}

    };

    class filter {

        typedef std::function<bool(files::full_stat const &, std::string const &)> predicate;

        const std::unordered_map<std::string, filter_type> _type_mapper{
            {"-inum", filter_type::INUM},
            {"-name", filter_type::NAME},
            {"-size-", filter_type::SIZE_LESSER},
            {"-size=", filter_type::SIZE_EQUAL},
            {"-size+", filter_type::SIZE_GREATER},
            {"-nlinks", filter_type::NLINKS}
        };

        const std::unordered_map<filter_type, predicate> _filter_mapper{
            {filter_type::INUM, [](files::full_stat const &file_stat, std::string const &inum) {
                return file_stat.get_stat().st_ino == std::stoi(inum);
            }},
            {filter_type::NAME, [](files::full_stat const &file_stat, std::string const &name) {
                return file_stat.get_name() == name;
            }},
            {filter_type::SIZE_LESSER, [](files::full_stat const &file_stat, std::string const &size) {
                return file_stat.get_stat().st_size < std::stoi(size);
            }},
            {filter_type::SIZE_EQUAL, [](files::full_stat const &file_stat, std::string const &size) {
                return file_stat.get_stat().st_size == std::stoi(size);
            }},
            {filter_type::SIZE_GREATER, [](files::full_stat const &file_stat, std::string const &size) {
                return file_stat.get_stat().st_size > std::stoi(size);
            }},
            {filter_type::NLINKS, [](files::full_stat const &file_stat, std::string const &nlinks) {
                return file_stat.get_stat().st_nlink == std::stoi(nlinks);
            }}
        };

        const std::unordered_set<filter_type> _integer_values_types{
            filter_type::INUM,
            filter_type::SIZE_LESSER,
            filter_type::SIZE_EQUAL,
            filter_type::SIZE_GREATER,
            filter_type::NLINKS
        };

        typedef std::pair<filter_type, std::string> filter_atom;
        std::vector<filter_atom> _filter_chain;

        void ensure_valid_value(filter_type const &type, std::string const &value) {
            if (_integer_values_types.count(type) != 0) {
                try {
                    std::stoi(value);
                } catch (std::logic_error const &e) {
                    throw filter_exception(e.what());
                }
            }
        }

    public:

        filter() = default;
        filter(filter const &other) = default;

        void add_filter(std::string type, std::string value) {
            if (type == "-size") {
                type.push_back(value[0]);
                value = value.substr(1);
            }
            if (_type_mapper.count(type) == 0) {
                throw filter_exception("Unknown filter argument '" + type + "'");
            }
            ensure_valid_value(_type_mapper.at(type), value);
            _filter_chain.push_back({_type_mapper.at(type), value});
        }

        bool apply(std::string const &path) const {
            files::full_stat file_stat(path);
            return std::all_of(_filter_chain.begin(), _filter_chain.end(), [&, this, path](filter_atom const &atom) {
                return _filter_mapper.at(atom.first)(file_stat, atom.second);
            });
        }

    };

}

int main(int argc, char *argv[]) {

    if (argc <= 1) {
        console::report("At least one argument expected, " + console::REPORT_HELP);
        return 0;
    }

    if (std::string(argv[1]) == "-help") {
        std::cout << console::USAGE;
    } else {
        std::string path(argv[1]);
        if (!files::file_exists(path)) {
            return console::report("Specified path does not exist");
        }

        filter::filter config;
        int arg_index = 2;
        while (arg_index < argc) {
            std::string arg(argv[arg_index++]);
            if (arg[0] != '-') {
                return console::report("Unexpected token '" + arg + "', " + console::REPORT_HELP);
            }
            if (arg_index >= argc) {
                return console::report("Value for token '" + arg + "' not specified, " + console::REPORT_HELP);
            }
            try {
                config.add_filter(arg, argv[arg_index++]);
            } catch (filter::filter_exception const &e) {
                return console::report(std::string(e.what()) + ", " + console::REPORT_HELP, errno);
            }
        }

        std::cout << config.apply(path) << std::endl;
    }

}
