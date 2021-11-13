
#define _CRT_SECURE_NO_WARNINGS

#ifdef _DEBUG 
#else
#include "mimalloc-new-delete.h"
#endif

#include <iostream>
#include <string>
#include <ctime>

#include "simdjson.h"
#include "claujson.h"

int main(int argc, char* argv[])
{
    const int thr_num = 8;
    int _ = clock();  
    
    claujson::UserType ut;

    {
        simdjson::dom::parser test;

        auto x = test.load(argv[1]);

        if (x.error() != simdjson::error_code::SUCCESS) {
            std::cout << x.error() << "\n";

            return -1;
        }

        const auto& tape = test.raw_tape();
        const auto& string_buf = test.raw_string_buf();

        std::vector<int64_t> start(thr_num + 1, 0);
        std::vector<int> key;
        int64_t length;
        int a = clock();

        std::cout << a - _ << "ms\n";
        {
            uint32_t string_length;
            size_t tape_idx = 0;
            uint64_t tape_val = tape[tape_idx];
            uint8_t type = uint8_t(tape_val >> 56);
            uint64_t payload;
            tape_idx++;
            size_t how_many = 0;
            if (type == 'r') {
                how_many = size_t(tape_val & simdjson::internal::JSON_VALUE_MASK);
                length = how_many;

                key = std::vector<int>(how_many, 0);
            }
            else {
                // Error: no starting root node?
                return -1;
            }

            start[0] = 1;
            for (int i = 1; i < thr_num; ++i) {
                start[i] = how_many / thr_num * i;
            }


            std::vector<int> _stack;
            std::vector<int> _stack2;

            int count = 1;
            for (; tape_idx < how_many; tape_idx++) {
                if (count < 8 && tape_idx == start[count]) {
                    count++;
                }
                else if (count < thr_num && tape_idx == start[count] + 1) {
                    start[count] = tape_idx;
                }

                //os << tape_idx << " : ";
                tape_val = tape[tape_idx];
                payload = tape_val & simdjson::internal::JSON_VALUE_MASK;
                type = uint8_t(tape_val >> 56);

                if (!_stack.empty() && _stack.back() == 1) { // if it is object,
                    _stack2.back() += 1; // next
                    _stack2.back() &= 1; // % 2
                }

                switch (type) {
                case '"': // we have a string
                    //os << "string \"";
                   // std::memcpy(&string_length, string_buf.get() + payload, sizeof(uint32_t));
                   // os << internal::escape_json_string(std::string_view(
                   //     reinterpret_cast<const char*>(string_buf.get() + payload + sizeof(uint32_t)),
                   //     string_length
                   // ));
                   // os << '"';
                  //  os << '\n';
                    if (!_stack.empty() && _stack.back() == 1) { // if it is object,
                        if (_stack2.back() == 1) {
                            key[tape_idx] = 1;
                        }
                    }

                    break;
                case 'l': // we have a long int
                    if (tape_idx + 1 >= how_many) {
                        return false;
                    }
                    //  os << "integer " << static_cast<int64_t>(tape[++tape_idx]) << "\n";
                    ++tape_idx;

                    break;
                case 'u': // we have a long uint
                    if (tape_idx + 1 >= how_many) {
                        return false;
                    }
                    //  os << "unsigned integer " << tape[++tape_idx] << "\n";
                    ++tape_idx;
                    break;
                case 'd': // we have a double
                  //  os << "float ";
                    if (tape_idx + 1 >= how_many) {
                        return false;
                    }

                    // double answer;
                    // std::memcpy(&answer, &tape[++tape_idx], sizeof(answer));
                   //  os << answer << '\n';
                    ++tape_idx;
                    break;
                case 'n': // we have a null
                   // os << "null\n";
                    break;
                case 't': // we have a true
                   // os << "true\n";
                    break;
                case 'f': // we have a false
                  //  os << "false\n";
                    break;
                case '{': // we have an object
                 //   os << "{\t// pointing to next tape location " << uint32_t(payload)
                 //       << " (first node after the scope), "
                  //      << " saturated count "
                   //     << ((payload >> 32) & internal::JSON_COUNT_MASK) << "\n";

                    _stack.push_back(1);
                    _stack2.push_back(0);
                    break;
                case '}': // we end an object
                  //  os << "}\t// pointing to previous tape location " << uint32_t(payload)
                  //      << " (start of the scope)\n";
                    _stack.pop_back();
                    _stack2.pop_back();
                    break;
                case '[': // we start an array
                  //  os << "[\t// pointing to next tape location " << uint32_t(payload)
                  //      << " (first node after the scope), "
                  //      << " saturated count "
                   //     << ((payload >> 32) & internal::JSON_COUNT_MASK) << "\n";
                    _stack.push_back(0);
                    break;
                case ']': // we end an array
                 //   os << "]\t// pointing to previous tape location " << uint32_t(payload)
                  //      << " (start of the scope)\n";
                    _stack.pop_back();
                    break;
                case 'r': // we start and end with the root node
                  // should we be hitting the root node?
                    break;
                default:

                    return -3;
                }
            }

        }

        int b = clock();

        std::cout << b - a << "ms\n";

        start[thr_num] = length - 1;

        claujson::LoadData::parse(ut, string_buf, tape, length, key, start, thr_num); // 0 : use all thread. - notyet..

        int c = clock();
        std::cout << c - b << "ms\n";
    }
    int c = clock();
    std::cout << c - _ << "ms\n";

  //  claujson::LoadData::_save(std::cout, &ut);

	return 0;
}
