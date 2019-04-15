#include <iostream>

#include <vector>
#include <unordered_set>
#include <unordered_map>

#include <functional>
#include <algorithm>

namespace console {

    std::string _ERROR = "\033[31m";
    std::string _DEFAULT = "\033[0m";
    std::string _BOLD = "\033[1m";

    // @formatter:off
    std::string USAGE = "find utility v.1.0.0\n"
                        "Help:  ./find --help\n"
                        "Usage: ./find PATH [-inum INUM] [-name NAME] [-size (-|=|+)SIZE] [-nlinks NLINKS] [-exec EPATH]\n"
                        "\t- PATH is an absolute path to the directory for searching\n"
                        "\t- INUM is a number of " + _BOLD + "inode" + _DEFAULT + "\n"
                        "\t- NAME is a name of the file\n"
                        "\t- SIZE is a size of the file (- for Lesser, = for Equal, + for Greater)\n"
                        "\t- NLINKS is a number of " + _BOLD + "hardlink" + _DEFAULT + "s\n"
                        "\t- EPATH is an absolute path to the file that should be executed on each found entity\n";
    // @formatter:on

}

namespace filter {

    enum class filter_type {
        INUM,
        NAME,
        SIZE_LESSER,
        SIZE_EQUAL,
        SIZE_GREATER,
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

    class filter {

        typedef std::function<bool(std::string const &, std::string const &)> predicate;

        const std::unordered_map<std::string, filter_type> _type_mapper{
            {"-inum", filter_type::INUM},
            {"-name", filter_type::NAME},
            {"-size-", filter_type::SIZE_LESSER},
            {"-size=", filter_type::SIZE_EQUAL},
            {"-size+", filter_type::SIZE_GREATER}
        };

        const std::unordered_map<filter_type, predicate> _filter_mapper{
            {filter_type::INUM, [](std::string const &path, std::string const &inum) {
                return true;
            }},
            {filter_type::NAME, [](std::string const &path, std::string const &name) {
                return true;
            }},
            {filter_type::SIZE_LESSER, [](std::string const &path, std::string const &name) {
                return true;
            }},
            {filter_type::SIZE_EQUAL, [](std::string const &path, std::string const &name) {
                return true;
            }},
            {filter_type::SIZE_GREATER, [](std::string const &path, std::string const &name) {
                return true;
            }},
        };

        const std::unordered_set<filter_type> _integer_values_types{
            filter_type::INUM,
            filter_type::SIZE_LESSER,
            filter_type::SIZE_EQUAL,
            filter_type::SIZE_GREATER
        };

        typedef std::pair<filter_type, std::string> filter_atom;
        std::vector<filter_atom> _filter_chain;

    public:

        struct filter_exception : public std::runtime_error {

            filter_exception(std::string const &message) : runtime_error(message) {}

        };

        filter() = default;
        filter(filter const &other) = default;

        void ensure_valid_value(filter_type const &type, std::string const &value) {
            if (_integer_values_types.count(type) != 0) {
                try {
                    std::stoi(value);
                } catch (std::logic_error const &e) {
                    throw filter_exception(e.what());
                }
            }
        }

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
            return std::all_of(_filter_chain.begin(), _filter_chain.end(), [&, this, path](filter_atom const &atom) {
                return _filter_mapper.at(atom.first)(path, atom.second);
            });
        }

    };

}

int main(int argc, char *argv[]) {

    std::cout << console::USAGE;

}
